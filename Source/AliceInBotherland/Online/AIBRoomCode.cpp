// Alice in Botherland - room codes: a host address packed into a short code friends can type

#include "Online/AIBRoomCode.h"
#include "Async/Async.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"

namespace AIBRoomCode
{
	namespace
	{
		const TCHAR* Alphabet = TEXT("0123456789ABCDEFGHJKMNPQRSTVWXYZ");

		bool ParseIPv4(const FString& Text, uint32& Out)
		{
			TArray<FString> Parts;
			Text.TrimStartAndEnd().ParseIntoArray(Parts, TEXT("."));
			if (Parts.Num() != 4)
			{
				return false;
			}
			Out = 0;
			for (const FString& Part : Parts)
			{
				if (Part.IsEmpty() || !Part.IsNumeric())
				{
					return false;
				}
				const int32 Value = FCString::Atoi(*Part);
				if (Value < 0 || Value > 255)
				{
					return false;
				}
				Out = (Out << 8) | uint32(Value);
			}
			return true;
		}

		FString Encode(uint64 Value, int32 Digits)
		{
			FString Result;
			for (int32 Index = 0; Index < Digits; ++Index)
			{
				Result.InsertAt(0, Alphabet[Value & 31]);
				Value >>= 5;
			}
			return Result;
		}

		int32 DigitValue(TCHAR Char)
		{
			Char = FChar::ToUpper(Char);
			if (Char == TEXT('O')) Char = TEXT('0');
			if (Char == TEXT('I') || Char == TEXT('L')) Char = TEXT('1');
			for (int32 Index = 0; Index < 32; ++Index)
			{
				if (Alphabet[Index] == Char)
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}
	}

	FString FromAddress(const FString& IPv4, int32 Port)
	{
		uint32 Address = 0;
		if (!ParseIPv4(IPv4, Address))
		{
			return FString();
		}
		const FString Body = Encode(Address, 7);
		FString Code = Body.Left(4) + TEXT("-") + Body.Mid(4);
		if (Port > 0 && Port != DefaultPort)
		{
			Code += TEXT("-") + Encode(uint64(Port), 4);
		}
		return Code;
	}

	bool ToAddress(const FString& Code, FString& OutAddress)
	{
		FString Digits;
		for (TCHAR Char : Code)
		{
			if (Char == TEXT('-') || FChar::IsWhitespace(Char))
			{
				continue;
			}
			if (DigitValue(Char) == INDEX_NONE)
			{
				return false;
			}
			Digits.AppendChar(Char);
		}
		if (Digits.Len() != 7 && Digits.Len() != 11)
		{
			return false;
		}
		uint64 Address = 0;
		for (int32 Index = 0; Index < 7; ++Index)
		{
			Address = (Address << 5) | uint64(DigitValue(Digits[Index]));
		}
		if (Address > 0xFFFFFFFFull)
		{
			return false;
		}
		int32 Port = DefaultPort;
		if (Digits.Len() == 11)
		{
			Port = 0;
			for (int32 Index = 7; Index < 11; ++Index)
			{
				Port = (Port << 5) | DigitValue(Digits[Index]);
			}
			if (Port <= 0 || Port > 65535)
			{
				return false;
			}
		}
		OutAddress = FString::Printf(TEXT("%u.%u.%u.%u:%d"), uint32(Address >> 24) & 255, uint32(Address >> 16) & 255,
			uint32(Address >> 8) & 255, uint32(Address) & 255, Port);
		return true;
	}

	bool IsPrivateAddress(const FString& IPv4)
	{
		uint32 Address = 0;
		if (!ParseIPv4(IPv4, Address))
		{
			return false;
		}
		const uint32 A = Address >> 24;
		const uint32 B = (Address >> 16) & 255;
		return A == 10 || A == 127 || (A == 172 && B >= 16 && B <= 31) || (A == 192 && B == 168) || (A == 169 && B == 254);
	}

	void FetchPublicAddress(TFunction<void(const FString&)> OnDone)
	{
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [OnDone = MoveTemp(OnDone)]()
		{
			FString Result;
			ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			// Plain HTTP to an IP echo service: the reply body is just the address.
			TSharedPtr<FInternetAddr> Server;
			if (Sockets)
			{
				FAddressInfoResult Info = Sockets->GetAddressInfo(TEXT("api.ipify.org"), TEXT("80"), EAddressInfoFlags::Default, NAME_None, ESocketType::SOCKTYPE_Streaming);
				if (Info.ReturnCode == SE_NO_ERROR && Info.Results.Num() > 0)
				{
					Server = Info.Results[0].Address;
				}
			}
			if (Server.IsValid())
			{
				FSocket* Socket = Sockets->CreateSocket(NAME_Stream, TEXT("AIBPublicAddress"), Server->GetProtocolType());
				if (Socket)
				{
					Socket->SetNonBlocking(false);
					if (Socket->Connect(*Server))
					{
						const FTCHARToUTF8 Request(TEXT("GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n"));
						int32 Sent = 0;
						Socket->Send((const uint8*)Request.Get(), Request.Length(), Sent);
						TArray<uint8> Reply;
						uint8 Buffer[1024];
						int32 Read = 0;
						Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(5));
						while (Reply.Num() < 8192 && Socket->Recv(Buffer, sizeof(Buffer), Read) && Read > 0)
						{
							Reply.Append(Buffer, Read);
						}
						Reply.Add(0);
						const FString Text = UTF8_TO_TCHAR((const char*)Reply.GetData());
						const int32 BodyStart = Text.Find(TEXT("\r\n\r\n"));
						if (BodyStart != INDEX_NONE)
						{
							FString Body = Text.Mid(BodyStart + 4).TrimStartAndEnd();
							uint32 Dummy = 0;
							if (ParseIPv4(Body, Dummy))
							{
								Result = Body;
							}
						}
					}
					Socket->Close();
					Sockets->DestroySocket(Socket);
				}
			}
			AsyncTask(ENamedThreads::GameThread, [OnDone, Result]() { OnDone(Result); });
		});
	}
}
