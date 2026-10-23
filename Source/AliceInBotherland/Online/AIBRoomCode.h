// Alice in Botherland - room codes: a host address packed into a short code friends can type

#pragma once

#include "CoreMinimal.h"

/**
 * A room code is the host's IPv4 address (and port, when it isn't the default 7777) written in Crockford base32,
 * e.g. "6C2M-9QZ". No matchmaking server is needed: the code decodes straight back into an address to connect to.
 * Codes made from the LAN address work on the same network; codes made from the public address work over the
 * internet when the host forwards UDP 7777.
 */
namespace AIBRoomCode
{
	constexpr int32 DefaultPort = 7777;

	/** "192.168.1.20" (+ port) -> "XXXX-XXX[-XXXX]". Empty if the address isn't IPv4. */
	FString FromAddress(const FString& IPv4, int32 Port = DefaultPort);

	/** Code (any case, with or without dashes/spaces; O/I/L read as 0/1/1) -> "a.b.c.d:port". False if it isn't a room code. */
	bool ToAddress(const FString& Code, FString& OutAddress);

	/** True for 10.x, 172.16-31.x, 192.168.x, 169.254.x and loopback. */
	bool IsPrivateAddress(const FString& IPv4);

	/** Asks a public echo service for this machine's internet address on a worker thread; calls back on the game thread ("" on failure). */
	void FetchPublicAddress(TFunction<void(const FString&)> OnDone);
}
