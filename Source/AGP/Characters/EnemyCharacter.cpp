// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "PlayerCharacter.h"
#include "DrawDebugHelpers.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
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
	
	
}

void AEnemyCharacter::MoveAlongPath()
{
	if (CurrentPath.IsEmpty()) return;

	// Get the next target location in the path
	FVector NextLocation = CurrentPath[CurrentPath.Num() - 1];
	FVector MovementDirection = NextLocation - GetActorLocation();
	MovementDirection.Normalize();

	// Perform a line trace from the enemy to the ground
	FVector Start = GetActorLocation() + FVector(0, 0, 50.0f);  // A bit above the actor
	FVector End = GetActorLocation() - FVector(0, 0, 1000.0f);  // Trace down

	FHitResult HitResult;
	bool bOnSolidGround = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	// Debug visualization of line trace
	DrawDebugLine(GetWorld(), Start, End, bOnSolidGround ? FColor::Green : FColor::Red, false, 1.0f, 0, 5.0f);

	if (bOnSolidGround && HitResult.bBlockingHit)
	{
		// Update the last known good location
		LastKnownGoodLocation = GetActorLocation();

		// Move towards the target location
		AddMovementInput(MovementDirection);

		// Check if the enemy is close enough to the current target in the path
		if (FVector::Distance(GetActorLocation(), NextLocation) < PathfindingError)
		{
			CurrentPath.Pop();
		}
	}
	else
	{
		// If not on solid ground, stop moving and clear the current path
		CurrentPath.Empty();

		// Move back to the last known good location
		FVector ReturnDirection = LastKnownGoodLocation - GetActorLocation();
		ReturnDirection.Normalize();
		AddMovementInput(ReturnDirection);

		// If close enough to the last known good location, try to find a new path
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
	else if (CurrentState == EEnemyState::Engage && SensedCharacter)
	{
		CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(), SensedCharacter->GetActorLocation());
	}
	else if (CurrentState == EEnemyState::Evade && SensedCharacter)
	{
		CurrentPath = PathfindingSubsystem->GetPathAway(GetActorLocation(), SensedCharacter->GetActorLocation());
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

void AEnemyCharacter::TickEngage()
{
	if (!SensedCharacter) return;

	if (CurrentPath.IsEmpty())
	{
		FindNewPath();
	}

	MoveAlongPath();
}

void AEnemyCharacter::TickEvade()
{
	if (!SensedCharacter) return;

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

			if(Distance < 300.0f)
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
	if(CurrentPath.IsEmpty())
	{
		//UBoxComponent* BoxCollider = Cast<UBoxComponent>(NearestHidingSpot->GetComponentByClass(UBoxComponent::StaticClass()));

		//FVector Spot = BoxCollider->GetComponentLocation();
		//Spot.Z -= 96;
		CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(), NearestHidingSpot->GetActorLocation());
		//CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(), Spot);
	}
	MoveAlongPath();
	AtSpot = true;
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

void AEnemyCharacter::TickExamine()
{
	if(!AtSpot)
	{
		GoToHidingSpot();
	}

	//don't sense player if they're hiding
	if(IsPlayerHiding(NearestHidingSpot))
	{
		SensedCharacter = nullptr;
	}

	//add spot to checked after examine is finished
	if(ExamineTimer >= 5.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Timer: %f"), ExamineTimer);

		CheckedHidingSpots.Add(NearestHidingSpot);
		NearestHidingSpot = nullptr;
		CurrentPath.Empty();
	}
}

void AEnemyCharacter::TickHiding()
{
	GoToHidingSpot();
	UE_LOG(LogTemp, Display, TEXT("Enemy is now hiding"));
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

	// DO NOTHING UNLESS IT IS ON THE SERVER
	if (GetLocalRole() != ROLE_Authority) return;
	
	UpdateSight();
	
	switch(CurrentState)
	{
	case EEnemyState::Patrol:
		TickPatrol();

		if(IsEnemyNearHidingSpot())
		{
			GetNearestHidingSpot();

			if(NearestHidingSpot && !IsHidingSpotExamined(NearestHidingSpot))
			{
				GoToHidingSpot();
				CurrentState = EEnemyState::Examine;
			}
		}
		
		if (SensedCharacter)
		{
			if (HealthComponent->GetCurrentHealthPercentage() >= 0.4f)
			{
				CurrentState = EEnemyState::Engage;
			} else
			{
				CurrentState = EEnemyState::Evade;
			}
			CurrentPath.Empty();
		}
		break;
	case EEnemyState::Engage:
		TickEngage();
		if (HealthComponent->GetCurrentHealthPercentage() < 0.4f)
		{
			CurrentPath.Empty();
			CurrentState = EEnemyState::Evade;
		} else if (!SensedCharacter)
		{
			CurrentState = EEnemyState::Patrol;
		}
		break;
	case EEnemyState::Evade:
		TickEvade();
		if (HealthComponent->GetCurrentHealthPercentage() >= 0.4f)
		{
			CurrentPath.Empty();
			CurrentState = EEnemyState::Engage;
		} else if (!SensedCharacter)
		{
			CurrentState = EEnemyState::Patrol;
		}
		break;
	case EEnemyState::Examine:
		if(AtSpot)
		{
			ExamineTimer += DeltaTime;
			TickExamine();

			if(ExamineTimer >= 5.0f)
			{
				ExamineTimer = 0.0f;
				AtSpot = false;
				CurrentPath.Empty();
				if(HealthComponent->GetCurrentHealthPercentage() < 0.4f)
				{
					CurrentState = EEnemyState::Evade;
				}
				CurrentState = EEnemyState::Patrol;
			}
		}
		break;
	case EEnemyState::Hiding:
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

