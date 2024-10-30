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

