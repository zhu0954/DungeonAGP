#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CorridorNode.generated.h"

UCLASS()
class AGP_API ACorridorNode : public AActor
{
	GENERATED_BODY()

	friend class UPathfindingSubsystem;
    
public:    
	// Sets default values for this actor's properties
	ACorridorNode();

	virtual bool ShouldTickIfViewportsOnly() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TArray<ACorridorNode*> ConnectedNodes;
	UPROPERTY(VisibleAnywhere)
	USceneComponent* LocationComponent;

public:    
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
