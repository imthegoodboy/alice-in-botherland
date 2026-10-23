// Alice in Botherland - assembles and indexes the island's set pieces

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/AIBTypes.h"
#include "AIBWorldSubsystem.generated.h"

class AAIBSetPiece;
class AAIBOcean;
class AAIBFleet;
class AAIBNumberArena;
class AAIBHotelArena;
class AAIBRaceCourse;
class AAIBGuardianSite;
class AAIBEnvironmentDirector;
class AAIBLandingSite;

UCLASS()
class ALICEINBOTHERLAND_API UAIBWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Finds set pieces placed in the level or spawns missing ones locally. Safe to call repeatedly. */
	void EnsureWorldActors();

	/** Local camera zone (drives set piece visibility, lighting and ambience). */
	void SetLocalZone(EAIBSceneZone Zone);
	EAIBSceneZone GetLocalZone() const { return LocalZone; }

	AAIBOcean* GetOcean() const { return Ocean.Get(); }
	AAIBFleet* GetFleet() const { return Fleet.Get(); }
	AAIBLandingSite* GetLanding() const { return Landing.Get(); }
	AAIBNumberArena* GetNumberArena() const { return NumberArena.Get(); }
	AAIBHotelArena* GetHotel() const { return Hotel.Get(); }
	AAIBRaceCourse* GetRaceCourse() const { return RaceCourse.Get(); }
	AAIBGuardianSite* GetGuardian() const { return Guardian.Get(); }
	AAIBEnvironmentDirector* GetEnvironment() const { return Environment.Get(); }

	bool IsIslandMap() const;

private:
	template <typename T>
	T* FindOrSpawn(TWeakObjectPtr<T>& Slot, const FTransform& Transform);

	TWeakObjectPtr<AAIBOcean> Ocean;
	TWeakObjectPtr<AAIBFleet> Fleet;
	TWeakObjectPtr<AAIBLandingSite> Landing;
	TWeakObjectPtr<AAIBNumberArena> NumberArena;
	TWeakObjectPtr<AAIBHotelArena> Hotel;
	TWeakObjectPtr<AAIBRaceCourse> RaceCourse;
	TWeakObjectPtr<AAIBGuardianSite> Guardian;
	TWeakObjectPtr<AAIBEnvironmentDirector> Environment;

	EAIBSceneZone LocalZone = EAIBSceneZone::Lobby;
	bool bEnsured = false;
};
