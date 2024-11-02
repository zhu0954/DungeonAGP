// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCharacterHUD.generated.h"

/**
 * 
 */
UCLASS()
class AGP_API UPlayerCharacterHUD : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetHealthBar(float HealthPercent);
	void SetAmmoText(int32 RoundsRemaining, int32 MagazineSize);
	void SetRemainingEnemiesText(int32 RemainingEnemies);

protected:

	UPROPERTY(EditAnywhere, meta=(BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(EditAnywhere, meta=(BindWidget))
	class UTextBlock* AmmoText;

	UPROPERTY(EditAnywhere, meta=(BindWidget))
	class UTextBlock* RemainingEnemiesText;
	
};
