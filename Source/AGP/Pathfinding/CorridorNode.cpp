#include "CorridorNode.h"

// Sets default values
ACorridorNode::ACorridorNode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bNetLoadOnClient = false;

	LocationComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Location Component"));
	SetRootComponent(LocationComponent);
}

// Called when the game starts or when spawned
void ACorridorNode::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACorridorNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FColor SphereColor = FColor::Red;

	DrawDebugSphere(GetWorld(), GetActorLocation(), 50.0f, 4, SphereColor, false, -1, 0, 5.0f);
    
	for (const ACorridorNode* ConnectedNode : ConnectedNodes)
	{
		if (ConnectedNode)
		{
			DrawDebugLine(GetWorld(), GetActorLocation(), ConnectedNode->GetActorLocation(),
				FColor::Yellow, false, -1, 0, 5.0f);
		}
	}
}

bool ACorridorNode::ShouldTickIfViewportsOnly() const
{
	return true;
}
