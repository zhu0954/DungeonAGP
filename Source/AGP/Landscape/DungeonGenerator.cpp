#include "DungeonGenerator.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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
    // Set the random seed for consistent randomization
    FMath::RandInit(RandomSeed);

    // Clear the previously generated dungeon
    ClearDungeon();

    // Track room locations 
    TArray<FVector> RoomLocations;
    TArray<TArray<int32>> RoomGrid;  // Store room type at each grid cell
    RoomGrid.SetNum(GridSizeX);
    for (int32 i = 0; i < GridSizeX; ++i)
    {
        RoomGrid[i].SetNumZeroed(GridSizeY);
    }

     // Place rooms with a 20% chance for each of the 5 room types, ensuring no two adjacent rooms are the same
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            // Randomly decide whether to spawn a room
            if (FMath::RandRange(0, 100) < 50) // Ensure we spawn a room in each valid location
            {
                FVector SpawnLocation = FVector(X * RoomSize, Y * RoomSize, 0);
                FRotator SpawnRotation = FRotator::ZeroRotator;

                if (RoomTypes.Num() == 5) // Ensure there are exactly 5 room types available
                {
                    // Determine the valid room types to avoid adjacent duplication
                    TArray<int32> ValidRoomIndices = { 0, 1, 2, 3, 4 };

                    // Check neighboring rooms to exclude invalid room types
                    if (X > 0 && RoomGrid[X - 1][Y] != -1) // Check room to the left
                    {
                        ValidRoomIndices.Remove(RoomGrid[X - 1][Y]);
                    }
                    if (Y > 0 && RoomGrid[X][Y - 1] != -1) // Check room above
                    {
                        ValidRoomIndices.Remove(RoomGrid[X][Y - 1]);
                    }

                    // Randomly select a valid room type
                    if (ValidRoomIndices.Num() > 0)
                    {
                        int32 RandomRoomIndex = ValidRoomIndices[FMath::RandRange(0, ValidRoomIndices.Num() - 1)];
                        TSubclassOf<AActor> SelectedRoomClass = RoomTypes[RandomRoomIndex];

                        // Spawn the selected room actor
                        if (SelectedRoomClass)
                        {
                            GetWorld()->SpawnActor<AActor>(SelectedRoomClass, SpawnLocation, SpawnRotation);
                            RoomLocations.Add(SpawnLocation);
                            RoomGrid[X][Y] = RandomRoomIndex; // Store room type on the grid
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("RoomTypes array must have exactly 5 elements!"));
                }
            }
        }
    }

    // Connect rooms with corridors in gaps
    for (int32 X = 0; X < GridSizeX; X++)
    {
        for (int32 Y = 0; Y < GridSizeY; Y++)
        {
            if (RoomGrid[X][Y])
            {
                // Check for gaps horizontally, ensuring the next two cells are within bounds
                if (X < GridSizeX - 2 && !RoomGrid[X + 1][Y] && RoomGrid[X + 2][Y]) // Horizontal corridor
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector((X + 2) * RoomSize, Y * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                }

                // Check for gaps vertically, ensuring the next two cells are within bounds
                if (Y < GridSizeY - 2 && !RoomGrid[X][Y + 1] && RoomGrid[X][Y + 2]) // Vertical corridor
                {
                    FVector RoomA = FVector(X * RoomSize, Y * RoomSize, 0);
                    FVector RoomB = FVector(X * RoomSize, (Y + 2) * RoomSize, 0);
                    CreateCorridorBetweenRooms(RoomA, RoomB);
                }
            }
        }
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

