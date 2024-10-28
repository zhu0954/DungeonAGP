// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponComponent.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UHealthComponent;

UCLASS()
class AGP_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

	UFUNCTION(BlueprintCallable)
	bool HasWeapon();

	void EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats = FWeaponStats());
	UFUNCTION(BlueprintImplementableEvent)
	void EquipWeaponGraphical(bool bEquipWeapon);

	/**
	 * Will reload the weapon if the character has a weapon equipped.
	 */
	void Reload();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnDeath();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**
	 * A scene component to store the position that hit scan shots start from. For the enemy character this could
	 * be placed close to the guns position for example and for the player character it can be placed close to the
	 * camera position.
	 */
	UPROPERTY(VisibleAnywhere)
	USceneComponent* BulletStartPosition;

	/**
	 * A component that holds information about the health of the character. This component has functions
	 * for damaging the character and healing the character.
	 */
	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;

	/**
	 * An actor component that controls the logic for this characters equipped weapon.
	 */
	UPROPERTY(Replicated)
	UWeaponComponent* WeaponComponent = nullptr;

	/**
	 * Will fire at a specific location and handles the impact of the shot such as determining what it hit and
	 * deducting health if it hit a particular type of actor.
	 * @param FireAtLocation The location that you want to fire at.
	 */
	void Fire(const FVector& FireAtLocation);

	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent)
	void FireWeaponGraphical();

private:
	void EquipWeaponImplementation(bool bEquipWeapon, const FWeaponStats& WeaponStats = FWeaponStats());
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats = FWeaponStats());

};
