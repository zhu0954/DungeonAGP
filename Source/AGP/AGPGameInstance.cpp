 // Fill out your copyright notice in the Description page of Project Settings.
#include "AGPGameInstance.h"
#include "Pickups/WeaponPickup.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

UClass* UAGPGameInstance::GetWeaponPickupClass() const
{
 return WeaponPickupClass.Get();
}

void UAGPGameInstance::SpawnGroundHitParticles(const FVector& SpawnLocation)
{
 if (GroundHitParticles)
 {
  UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GroundHitParticles, SpawnLocation);
 }
}

void UAGPGameInstance::PlayGunshotSoundAtLocation(const FVector& Location)
{
 if (GunshotSoundCue)
 {
  UGameplayStatics::PlaySoundAtLocation(GetWorld(), GunshotSoundCue, Location, FRotator::ZeroRotator);
 }
}

void UAGPGameInstance::PlayGunshotSound2D()
{
 if (GunshotSoundCue)
 {
  UGameplayStatics::PlaySound2D(GetWorld(), GunshotSoundCue);
 }
}


