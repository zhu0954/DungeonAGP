// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "PlayerCharacter.h"
#include "DrawDebugHelpers.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Components/BoxComponent.h"
#include "Perception/PawnSensingComponent.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// DO NOTHING IF NOT ON THE SERVER
	if (GetLocalRole() != ROLE_Authority) return;

	GetHidingSpots();
	
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	if (PathfindingSubsystem)
	{
		CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find the PathfindingSubsystem"))
	}
	if (PawnSensingComponent)
	{
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &AEnemyCharacter::OnSensedPawn);
	}

	if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
	{
		HealthComp->SetMaxHealth(1.0f);
	}
	
	
}

void AEnemyCharacter::MoveAlongPath()
{
	if (CurrentPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No path available for movement."));
		return;
	}

	// Get the next target location in the path
	FVector NextLocation = CurrentPath[CurrentPath.Num() - 1];
	FVector MovementDirection = (NextLocation - GetActorLocation()).GetSafeNormal();

	// Debug log the movement direction
	UE_LOG(LogTemp, Display, TEXT("Moving towards: %s"), *NextLocation.ToString());

	// Perform a line trace from the enemy to check if above solid ground
	FVector Start = GetActorLocation() + FVector(0, 0, 50.0f);
	FVector End = GetActorLocation() - FVector(0, 0, 1000.0f);

	FHitResult HitResult;
	bool bOnSolidGround = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	// Debug visualization and logging of the line trace
	DrawDebugLine(GetWorld(), Start, End, bOnSolidGround ? FColor::Green : FColor::Red, false, 1.0f, 0, 5.0f);
	UE_LOG(LogTemp, Display, TEXT("On solid ground: %s"), bOnSolidGround ? TEXT("Yes") : TEXT("No"));

	if (bOnSolidGround && HitResult.bBlockingHit)
	{
		LastKnownGoodLocation = GetActorLocation();
		AddMovementInput(MovementDirection);

		// Check if close enough to the current target in the path
		if (FVector::Distance(GetActorLocation(), NextLocation) < PathfindingError)
		{
			CurrentPath.Pop();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not on solid ground. Returning to last known good location."));
		CurrentPath.Empty();
		FVector ReturnDirection = (LastKnownGoodLocation - GetActorLocation()).GetSafeNormal();
		AddMovementInput(ReturnDirection);

		if (FVector::Distance(GetActorLocation(), LastKnownGoodLocation) < PathfindingError)
		{
			CurrentPath.Empty();
			FindNewPath();
		}
	}
}

void AEnemyCharacter::FindNewPath()
{
	if (CurrentState == EEnemyState::Patrol)
	{
		CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
	}
	

	// Validate and remove points that aren't above solid ground
	CurrentPath.RemoveAll([this](const FVector& Location) {
		return !IsLocationAboveSolidGround(Location);
	});
}


void AEnemyCharacter::TickPatrol()
{
	if (CurrentPath.IsEmpty())
	{
		FindNewPath();
	}

	MoveAlongPath();
}



//add all hiding spots in the world to HidingSpots array
void AEnemyCharacter::GetHidingSpots()
{
	for(TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* CheckActor = *It;

		//check if actor has hideableobject tag
		if(CheckActor->ActorHasTag("HideableObject"))
		{
			HidingSpots.Add(CheckActor);
			UE_LOG(LogTemp, Error, TEXT("hiding spots: %d"), HidingSpots.Num());
		}
	}
}

//check if enemy is near (< 300.0f) a hiding spot
bool AEnemyCharacter::IsEnemyNearHidingSpot()
{
	if(HidingSpots.IsEmpty())
	{
		GetHidingSpots();
	}

	for(AActor* HidingSpot : HidingSpots)
	{
		if(HidingSpot)
		{
			float Distance = FVector::Distance(GetActorLocation(), HidingSpot->GetActorLocation());

			if(Distance < 200.0f)
			{
				UE_LOG(LogTemp, Error, TEXT("Enemy near a hiding spot"));
				return true;
			}
		}
	}
	return false;
}

//get nearest hiding spot to enemy
AActor* AEnemyCharacter::GetNearestHidingSpot()
{
	UE_LOG(LogTemp, Error, TEXT("Getting nearest hiding spot"));

	if(HidingSpots.IsEmpty())
	{
		GetHidingSpots();
	}

	NearestHidingSpot = nullptr;
	float NearestDistance = MAX_FLT;

	for(AActor* HidingSpot : HidingSpots)
	{
		if(HidingSpot)
		{
			float Distance = FVector::Distance(GetActorLocation(), HidingSpot->GetActorLocation());
			{
				if(Distance < NearestDistance)
				{
					NearestDistance = Distance;
					NearestHidingSpot = HidingSpot;
				}
			}
		}
	}
	return NearestHidingSpot;
}

//check if spot has been examined
bool AEnemyCharacter::IsHidingSpotExamined(AActor* Spot)
{
	if(CheckedHidingSpots.Contains(Spot))
	{
		UE_LOG(LogTemp, Error, TEXT("Hiding spot already examined"));
		return true;
	}
	return false;
}

//go to hiding spot (KINDA BROKEN ENEMY DOESN'T GO TO SPOT BUT IS NEAR IT AND JUST STARES AT IT BUT IT WORKS FOR THIS BEHAVIOUR)
void AEnemyCharacter::GoToHidingSpot()
{
	UE_LOG(LogTemp, Display, TEXT("Enemy attempting to move to hiding spot."));

	if (!NearestHidingSpot)
	{
		NearestHidingSpot = GetNearestHidingSpot();
		if (!NearestHidingSpot)
		{
			UE_LOG(LogTemp, Error, TEXT("No hiding spot found!"));
			return;
		}
	}

	// Use the location of the box collider directly
	UBoxComponent* BoxCollider = Cast<UBoxComponent>(NearestHidingSpot->GetComponentByClass(UBoxComponent::StaticClass()));
	if (BoxCollider)
	{
		FVector SpotLocation = BoxCollider->GetComponentLocation();
		SpotLocation.Z -= 96;  // Adjust if needed to ensure it's at ground level
		FVector MovementDirection = (SpotLocation - GetActorLocation()).GetSafeNormal();
        
		AddMovementInput(MovementDirection);

		// Check if close enough to the hiding spot collider
		float DistanceToSpot = FVector::Distance(GetActorLocation(), SpotLocation);
		if (DistanceToSpot < PathfindingError)
		{
			AtSpot = true;
			UE_LOG(LogTemp, Display, TEXT("Enemy reached the hiding spot."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No BoxCollider found for the hiding spot."));
	}
}



//check if player in hiding spot
bool AEnemyCharacter::IsPlayerHiding(AActor* CurrentSpot)
{
	APlayerCharacter* Player = FindPlayer();

	if(Player)
	{
		float Distance = FVector::Distance(Player->GetActorLocation(), CurrentSpot->GetActorLocation());

		if(Distance < 200.0f)
		{
			UE_LOG(LogTemp, Error, TEXT("PLAYER HIDING"));
			return true;
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("PLAYER NOT HIDING"));
		}
	}
	return false;
}

void AEnemyCharacter::TickExamine(float DeltaTime)
{
	UE_LOG(LogTemp, Display, TEXT("Enemy is in Examine State."));

	if (!AtSpot)
	{
		UE_LOG(LogTemp, Display, TEXT("Enemy moving to hiding spot."));
		GoToHidingSpot();
	}
	else
	{
		// Increment the timer by DeltaTime
		ExamineTimer += DeltaTime;
		UE_LOG(LogTemp, Display, TEXT("Examine timer: %f"), ExamineTimer);

		if (ExamineTimer >= 5.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Examination complete. Transitioning to Hiding mode."));

			CheckedHidingSpots.Add(NearestHidingSpot);
			NearestHidingSpot = nullptr;

			// Reset examine variables
			ExamineTimer = 0.0f;
			AtSpot = false;

			// Transition to Hiding mode
			CurrentState = EEnemyState::Hiding;
		}
	}
}


void AEnemyCharacter::TickHiding()
{
	UE_LOG(LogTemp, Display, TEXT("Enemy is in Hiding State."));

	// Move towards the hiding spot
	GoToHidingSpot();

	// Once the enemy reaches the hiding spot, it will remain there
	if (AtSpot)
	{
		UE_LOG(LogTemp, Display, TEXT("Enemy is now hiding."));
		// Optionally you can add logic here to stop the enemy from further movement
	}
}

void AEnemyCharacter::OnSensedPawn(APawn* SensedActor)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(SensedActor))
	{
		SensedCharacter = Player;
		//UE_LOG(LogTemp, Display, TEXT("Sensed Player"))
	}
}

void AEnemyCharacter::UpdateSight()
{
	if (!SensedCharacter) return;
	if (PawnSensingComponent)
	{
		if (!PawnSensingComponent->HasLineOfSightTo(SensedCharacter))
		{
			SensedCharacter = nullptr;
			//UE_LOG(LogTemp, Display, TEXT("Lost Player"))
		}
	}
}


// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() != ROLE_Authority) return;  // Only execute on server

	UpdateSight();
    
	switch(CurrentState)
	{
	case EEnemyState::Patrol:
		UE_LOG(LogTemp, Display, TEXT("Enemy is in Patrol State."));
		TickPatrol();

		if (IsEnemyNearHidingSpot())
		{
			UE_LOG(LogTemp, Display, TEXT("Enemy found a nearby hiding spot."));
			GetNearestHidingSpot();

			if (NearestHidingSpot && !IsHidingSpotExamined(NearestHidingSpot))
			{
				UE_LOG(LogTemp, Display, TEXT("Enemy approaching hiding spot for examination."));
				GoToHidingSpot();
				CurrentState = EEnemyState::Examine;
			}
		}

		if (SensedCharacter)
		{
			UE_LOG(LogTemp, Display, TEXT("Enemy senses a character."));
			CurrentState = EEnemyState::Examine;  // Transition to Examine instead of Engage
			CurrentPath.Empty();
		}
		break;
        
	case EEnemyState::Examine:
		TickExamine(DeltaTime);  // Pass DeltaTime to TickExamine
		break;

	case EEnemyState::Hiding:
		UE_LOG(LogTemp, Display, TEXT("Enemy is in Hiding State."));
		TickHiding();
		CurrentPath.Empty();
		break;
	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

APlayerCharacter* AEnemyCharacter::FindPlayer() const
{
	APlayerCharacter* Player = nullptr;
	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		Player = *It;
		break;
	}
	if (!Player)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find the Player Character in the world."))
	}
	return Player;
}

// Implementation of the line trace function
bool AEnemyCharacter::IsLocationAboveSolidGround(const FVector& Location) const
{
	FVector Start = Location + FVector(0, 0, 100.0f); // Start a bit above the location
	FVector End = Location - FVector(0, 0, 1000.0f); // Trace downward

	FHitResult HitResult;

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility  // You could use a custom channel if needed
	);

	// Draw a debug line to visualize the trace
	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 1.0f);

	// Return true if the trace hits something (indicating solid ground)
	return bHit;
}
