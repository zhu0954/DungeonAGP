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

    TArray<FVector> RoomAndCorridorLocations;
    TArray<TArray<int32>> RoomGrid;  // Declare RoomGrid
    RoomGrid.SetNum(GridSizeX);
    for (int32 i = 0; i < GridSizeX; ++i)
    {
        RoomGrid[i].SetNumZeroed(GridSizeY);
    }

    // Place rooms and add their positions
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            FVector SpawnLocation = FVector(X * RoomSize, Y * RoomSize, 0);
            FRotator SpawnRotation = FRotator::ZeroRotator;
            FActorSpawnParameters SpawnParams;

            if (RoomTypes.Num() == 5 && FMath::RandRange(0, 100) < 50)
            {
                int32 RandomRoomIndex = FMath::RandRange(0, RoomTypes.Num() - 1);
                TSubclassOf<AActor> SelectedRoomClass = RoomTypes[RandomRoomIndex];

                if (SelectedRoomClass)
                {
                    GetWorld()->SpawnActor<AActor>(SelectedRoomClass, SpawnLocation, SpawnRotation, SpawnParams);
                    RoomAndCorridorLocations.Add(SpawnLocation);
                    RoomGrid[X][Y] = 1; // Mark this cell as occupied
                }
            }
        }
    }

    // Place corridors and add their positions to RoomAndCorridorLocations
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            if (RoomGrid[X][Y]) // Only if there’s a gap
            {
                if (X < GridSizeX - 2 && !RoomGrid[X + 1][Y] && RoomGrid[X + 2][Y])
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector((X + 2) * RoomSize, Y * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                    RoomAndCorridorLocations.Add((RoomA + RoomB) / 2);
                }
                if (Y < GridSizeY - 2 && !RoomGrid[X][Y + 1] && RoomGrid[X][Y + 2])
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector(X * RoomSize, (Y + 2) * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                    RoomAndCorridorLocations.Add((RoomA + RoomB) / 2);
                }
            }
        }
    }

    // Update pathfinding nodes based on room and corridor locations
    if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
    {
        PathfindingSubsystem->UpdatePathfindingNodes(RoomAndCorridorLocations, GridSizeX, GridSizeY, RoomSize);
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

