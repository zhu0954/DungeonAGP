#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

UCLASS()
class AGP_API ADungeonGenerator : public AActor
{
    GENERATED_BODY()

public:
    ADungeonGenerator();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;

    // Grid size for the dungeon
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    int32 GridSizeX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    int32 GridSizeY;

    // Size of each room
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    float RoomSize;

    // Array of room blueprints to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    TArray<TSubclassOf<AActor>> RoomTypes;

    // Corridor blueprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    TSubclassOf<AActor> CorridorClass;

    // Seed for random generation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
    int32 RandomSeed;

    // Function to trigger the dungeon generation
    UFUNCTION(CallInEditor, Category = "Dungeon Generation")
    void GenerateDungeon();


private:
    void ClearDungeon();
    void CreateCorridorBetweenRooms(FVector RoomA, FVector RoomB);
};