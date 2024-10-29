#include "DungeonGenerator.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStatics.h"

ADungeonGenerator::ADungeonGenerator()
{
    PrimaryActorTick.bCanEverTick = true;
    GridSizeX = 10;
    GridSizeY = 10;
    RoomSize = 500.0f;
    RandomSeed = 0;
}

void ADungeonGenerator::BeginPlay()
{
    Super::BeginPlay();
}

void ADungeonGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ADungeonGenerator::GenerateDungeon()
{
    FMath::RandInit(RandomSeed);
    ClearDungeon();

    TArray<FVector> RoomLocations;
    TArray<FVector> CorridorLocations;

    // Initialize the room grid
    TArray<TArray<int32>> RoomGrid;
    RoomGrid.SetNum(GridSizeX);
    for (int32 i = 0; i < GridSizeX; ++i)
    {
        RoomGrid[i].SetNumZeroed(GridSizeY);
    }

    // Place rooms
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            FVector SpawnLocation = FVector(X * RoomSize, Y * RoomSize, 0);
            FRotator SpawnRotation = FRotator::ZeroRotator;
            FActorSpawnParameters SpawnParams;

            bool bCanPlaceRoom = true;
            if (RoomGrid[X][Y] == 0)
            {
                // Ensure no adjacent rooms are of the same type
                TArray<int32> NeighborRoomTypes;
                if (X > 0) NeighborRoomTypes.Add(RoomGrid[X - 1][Y]);
                if (Y > 0) NeighborRoomTypes.Add(RoomGrid[X][Y - 1]);
                if (X < GridSizeX - 1) NeighborRoomTypes.Add(RoomGrid[X + 1][Y]);
                if (Y < GridSizeY - 1) NeighborRoomTypes.Add(RoomGrid[X][Y + 1]);

                int32 RandomRoomIndex = FMath::RandRange(0, RoomTypes.Num() - 1);
                for (int32 NeighborType : NeighborRoomTypes)
                {
                    if (NeighborType == RandomRoomIndex + 1)
                    {
                        bCanPlaceRoom = false;
                        break;
                    }
                }

                if (bCanPlaceRoom && RoomTypes.Num() > 0 && FMath::RandRange(0, 100) < 50)
                {
                    TSubclassOf<AActor> SelectedRoomClass = RoomTypes[RandomRoomIndex];
                    if (SelectedRoomClass)
                    {
                        GetWorld()->SpawnActor<AActor>(SelectedRoomClass, SpawnLocation, SpawnRotation, SpawnParams);
                        RoomLocations.Add(SpawnLocation);
                        RoomGrid[X][Y] = RandomRoomIndex + 1;
                    }
                }
            }
        }
    }

    // Place corridors and add nodes for each endpoint
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            if (RoomGrid[X][Y])
            {
                // Check horizontally
                if (X < GridSizeX - 2 && !RoomGrid[X + 1][Y] && RoomGrid[X + 2][Y])
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector((X + 2) * RoomSize, Y * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                    
                    // Add nodes for corridor ends
                    CorridorLocations.Add((RoomA + FVector(RoomSize / 2, 0, 0)));
                    CorridorLocations.Add((RoomB - FVector(RoomSize / 2, 0, 0)));
                }
                // Check vertically
                if (Y < GridSizeY - 2 && !RoomGrid[X][Y + 1] && RoomGrid[X][Y + 2])
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector(X * RoomSize, (Y + 2) * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                    
                    // Add nodes for corridor ends
                    CorridorLocations.Add((RoomA + FVector(0, RoomSize / 2, 0)));
                    CorridorLocations.Add((RoomB - FVector(0, RoomSize / 2, 0)));
                }
            }
        }
    }

    // Combine room and corridor locations into a single array for pathfinding
    TArray<FVector> AllNodeLocations = RoomLocations;
    AllNodeLocations.Append(CorridorLocations);

    // Update pathfinding nodes with both room and corridor locations
    if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
    {
        PathfindingSubsystem->UpdatePathfindingNodes(AllNodeLocations, GridSizeX, GridSizeY, RoomSize);
    }
}

void ADungeonGenerator::ClearDungeon()
{
    // Find and destroy all the previously spawned actors of the classes in RoomTypes and corridors
    for (TSubclassOf<AActor> RoomType : RoomTypes)
    {
        if (RoomType)
        {
            TArray<AActor*> ActorsToClear;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), RoomType, ActorsToClear);

            for (AActor* Actor : ActorsToClear)
            {
                Actor->Destroy();
            }
        }
    }

    // Destroy all corridors
    if (CorridorClass)
    {
        TArray<AActor*> CorridorsToClear;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), CorridorClass, CorridorsToClear);

        for (AActor* Corridor : CorridorsToClear)
        {
            Corridor->Destroy();
        }
    }
}

void ADungeonGenerator::CreateCorridorBetweenRooms(FVector RoomA, FVector RoomB)
{
    // Calculate the midpoint between the two rooms to place the corridor
    FVector CorridorLocation = (RoomA + RoomB) / 2;

    // Spawn a corridor actor at the midpoint
    if (CorridorClass)
    {
        FRotator CorridorRotation = FRotator::ZeroRotator;

        // Adjust rotation based on whether the rooms are connected horizontally or vertically
        if (FMath::Abs(RoomA.X - RoomB.X) > 0) // Horizontal corridor
        {
            CorridorRotation = FRotator(0, 90, 0); // Rotate the corridor
        }

        GetWorld()->SpawnActor<AActor>(CorridorClass, CorridorLocation, CorridorRotation);
    }
}
