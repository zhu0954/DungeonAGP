#include "PlayerCharacter.h"

#include "EnemyCharacter.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HealthComponent.h"
#include "PlayerCharacterHUD.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;
	
}

void APlayerCharacter::UpdateHealthBar(float HealthPercent)
{
	if (PlayerHUD && IsLocallyControlled())
	{
		PlayerHUD->SetHealthBar(HealthPercent);
	}
}

void APlayerCharacter::UpdateAmmoUI(int32 RoundsRemaining, int32 MagazineSize)
{
	if (PlayerHUD && IsLocallyControlled())
	{
		PlayerHUD->SetAmmoText(RoundsRemaining, MagazineSize);
	}
}

void APlayerCharacter::DrawUI()
{
	if (IsLocallyControlled() && PlayerHUDClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerHUD = CreateWidget<UPlayerCharacterHUD>(PlayerController, PlayerHUDClass);
			if (PlayerHUD)
			{
				PlayerHUD->AddToPlayerScreen();
			}
		}
	}
	UpdateHealthBar(1.0f);
	UpdateRemainingEnemiesText();
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	DrawUI();

	// Disable movement initially
	bCanMove = false;

	// Set a timer to enable movement after 20 seconds
	GetWorld()->GetTimerManager().SetTimer(
		MovementEnableTimerHandle,
		this,
		&APlayerCharacter::EnableMovement,
		20.0f,  // Delay in seconds
		false   // Do not loop
	);
}

void APlayerCharacter::EnableMovement()
{
	bCanMove = true;
	GetWorld()->GetTimerManager().ClearTimer(MovementEnableTimerHandle);
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (PlayerHUD)
	{
		PlayerHUD->RemoveFromParent();
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateRemainingEnemiesText();
	Timer -= DeltaTime;
	if(Timer <= 0.0f)
	{
		if (PlayerHUD && IsLocallyControlled())
		{
			PlayerHUD->DeleteTimerText();
		}
		Timer = 0.0f;
	}
	UpdateTimerText();
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		Input->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &APlayerCharacter::FireWeapon);
		Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABaseCharacter::Reload);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!bCanMove) return; // Only allow movement if bCanMove is true

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FVector ForwardVector = GetActorForwardVector();
	AddMovementInput(ForwardVector, MovementVector.X);
	const FVector RightVector = GetActorRightVector();
	AddMovementInput(RightVector, MovementVector.Y);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void APlayerCharacter::FireWeapon(const FInputActionValue& Value)
{
	FVector CameraPosition;
	FRotator CameraRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(CameraPosition, CameraRotation);
	const FVector CameraForward = UKismetMathLibrary::GetForwardVector(CameraRotation);
	if (BulletStartPosition)
	{
		Fire(BulletStartPosition->GetComponentLocation() + 10000.0f * CameraForward);
	}
}

void APlayerCharacter::UpdateRemainingEnemiesText()
{
	int32 TotalEnemies = 0;
	for(TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		TotalEnemies++;
	}

	//UpdateRemainingEnemiesText(TotalEnemies);
	if (PlayerHUD && IsLocallyControlled())
	{
		PlayerHUD->SetRemainingEnemiesText(TotalEnemies);
	}
	UE_LOG(LogTemp, Error, TEXT("TotalEnemies: %d"), TotalEnemies);
}

void APlayerCharacter::UpdateTimerText()
{
	if (PlayerHUD && IsLocallyControlled())
	{
		PlayerHUD->SetTimerText(Timer);
	}
	UE_LOG(LogTemp, Error, TEXT("Timer: %d"), Timer);
}
