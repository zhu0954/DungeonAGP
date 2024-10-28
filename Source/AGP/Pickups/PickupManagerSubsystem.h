#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PickupManagerSubsystem.generated.h"

class AWeaponPickup;

UCLASS()
class AGP_API UPickupManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void PopulateSpawnLocations();
	void SpawnWeapons();

	TArray<FVector> AllRoomSpawnLocations;     // All possible spawn locations in rooms
	TArray<FVector> UsedSpawnLocations;        // Tracks spawn locations that have been used for weapon spawns
};
