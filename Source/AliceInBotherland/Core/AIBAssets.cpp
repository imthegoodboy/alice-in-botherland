// Alice in Botherland - runtime asset lookup with graceful fallbacks

#include "Core/AIBAssets.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/PackageName.h"

namespace AIBAssets
{
	static TMap<FString, TWeakObjectPtr<UObject>>& Cache()
	{
		static TMap<FString, TWeakObjectPtr<UObject>> Map;
		return Map;
	}

	static TSet<FString>& Missing()
	{
		static TSet<FString> Set;
		return Set;
	}

	UObject* Load(const FString& PackagePath)
	{
		if (PackagePath.IsEmpty())
		{
			return nullptr;
		}
		if (TWeakObjectPtr<UObject>* Found = Cache().Find(PackagePath))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
		if (Missing().Contains(PackagePath))
		{
			return nullptr;
		}

		FString ObjectPath = PackagePath;
		if (!ObjectPath.Contains(TEXT(".")))
		{
			ObjectPath = PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
		}
		const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);

		UObject* Obj = FindObject<UObject>(nullptr, *ObjectPath);
		if (!Obj && FPackageName::DoesPackageExist(PackageName))
		{
			Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (Obj)
		{
			Cache().Add(PackagePath, Obj);
		}
		else
		{
			Missing().Add(PackagePath);
		}
		return Obj;
	}

	UStaticMesh* Mesh(const FString& PackagePath, UStaticMesh* Fallback)
	{
		UStaticMesh* Found = Cast<UStaticMesh>(Load(PackagePath));
		return Found ? Found : Fallback;
	}

	UMaterialInterface* Material(const FString& PackagePath, UMaterialInterface* Fallback)
	{
		UMaterialInterface* Found = Cast<UMaterialInterface>(Load(PackagePath));
		return Found ? Found : Fallback;
	}

	USoundBase* Sound(const FString& PackagePath)
	{
		return Cast<USoundBase>(Load(PackagePath));
	}

	UStaticMesh* Cube() { return Mesh(TEXT("/Engine/BasicShapes/Cube")); }
	UStaticMesh* Cylinder() { return Mesh(TEXT("/Engine/BasicShapes/Cylinder")); }
	UStaticMesh* Sphere() { return Mesh(TEXT("/Engine/BasicShapes/Sphere")); }
	UStaticMesh* Plane() { return Mesh(TEXT("/Engine/BasicShapes/Plane")); }
	UStaticMesh* Cone() { return Mesh(TEXT("/Engine/BasicShapes/Cone")); }

	UMaterialInterface* BasicMaterial()
	{
		return Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	}

	void SetColor(UMaterialInstanceDynamic* MID, const FLinearColor& Color)
	{
		if (!MID)
		{
			return;
		}
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("Tint"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}

	static UMaterialInstanceDynamic* MakeFrom(UObject* Outer, const FString& Path, const FLinearColor& Color, float Scalar, const TCHAR* ScalarName)
	{
		UMaterialInterface* Parent = Material(Path, BasicMaterial());
		if (!Parent)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
		SetColor(MID, Color);
		MID->SetScalarParameterValue(ScalarName, Scalar);
		return MID;
	}

	UMaterialInstanceDynamic* MakeEmissive(UObject* Outer, const FLinearColor& Color, float Intensity)
	{
		return MakeFrom(Outer, Paths::Materials + TEXT("M_AIB_Emissive"), Color, Intensity, TEXT("Intensity"));
	}

	UMaterialInstanceDynamic* MakeHologram(UObject* Outer, const FLinearColor& Color, float Opacity)
	{
		return MakeFrom(Outer, Paths::Materials + TEXT("M_AIB_Hologram"), Color, Opacity, TEXT("Opacity"));
	}

	UMaterialInstanceDynamic* MakeBeam(UObject* Outer, const FLinearColor& Color, float Intensity)
	{
		return MakeFrom(Outer, Paths::Materials + TEXT("M_AIB_Beam"), Color, Intensity, TEXT("Intensity"));
	}

	UMaterialInstanceDynamic* MakeTinted(UObject* Outer, const FString& MaterialPath, const FLinearColor& Tint)
	{
		UMaterialInterface* Parent = Material(MaterialPath, BasicMaterial());
		if (!Parent)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
		SetColor(MID, Tint);
		return MID;
	}
}
