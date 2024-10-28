#include "PickupManagerSubsystem.h"
#include "WeaponPickup.h"
#include "AGP/AGPGameInstance.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UPickupManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    PopulateSpawnLocations();
    SpawnWeapons();
}

void UPickupManagerSubsystem::PopulateSpawnLocations()
{
    AllRoomSpawnLocations.Empty();
    UsedSpawnLocations.Empty();

    if (const UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
    {
        TArray<FVector> Waypoints = PathfindingSubsystem->GetWaypointPositions();
        UE_LOG(LogTemp, Display, TEXT("Total waypoints found: %d"), Waypoints.Num());

        for (const FVector& Waypoint : Waypoints)
        {
            // Here you may want to add any logic to filter specific waypoints or adjust their position
            AllRoomSpawnLocations.Add(Waypoint);
            UE_LOG(LogTemp, Display, TEXT("Added possible spawn location: %s"), *Waypoint.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Total possible room spawn locations: %d"), AllRoomSpawnLocations.Num());
}

void UPickupManagerSubsystem::SpawnWeapons()
{
    if (AllRoomSpawnLocations.IsEmpty())
    {
        
        return;
    }

    if (const UAGPGameInstance* GameInstance = GetWorld()->GetGameInstance<UAGPGameInstance>())
    {
        // Loop through each room spawn location and spawn a weapon if it hasn't been used
        for (const FVector& SpawnLocation : AllRoomSpawnLocations)
        {
            if (UsedSpawnLocations.Contains(SpawnLocation))
            {
                UE_LOG(LogTemp, Display, TEXT("Location already used for spawning: %s"), *SpawnLocation.ToString());
                continue;
            }

            FVector AdjustedSpawnLocation = SpawnLocation;
            AdjustedSpawnLocation.Z += 50.0f;  // Adjust height to prevent clipping

            AWeaponPickup* SpawnedPickup = GetWorld()->SpawnActor<AWeaponPickup>(
                GameInstance->GetWeaponPickupClass(), AdjustedSpawnLocation, FRotator::ZeroRotator);

            if (SpawnedPickup)
            {
                UE_LOG(LogTemp, Display, TEXT("Successfully spawned weapon pickup at: %s"), *SpawnedPickup->GetActorLocation().ToString());
                UsedSpawnLocations.Add(SpawnLocation);  // Mark this location as used
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to spawn weapon pickup at location: %s"), *AdjustedSpawnLocation.ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No GameInstance found. Unable to spawn weapon pickups."));
    }
}
