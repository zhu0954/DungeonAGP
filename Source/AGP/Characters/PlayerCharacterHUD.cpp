// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterHUD.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerCharacterHUD::SetHealthBar(float HealthPercent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}
}

void UPlayerCharacterHUD::SetAmmoText(int32 RoundsRemaining, int32 MagazineSize)
{
	if (AmmoText)
	{
		AmmoText->SetText(FText::FromString(FString::FromInt(RoundsRemaining) + " / " + FString::FromInt(MagazineSize)));
	}
}

void UPlayerCharacterHUD::SetRemainingEnemiesText(int32 RemainingEnemies)
{
	if (RemainingEnemiesText)
	{
		RemainingEnemiesText->SetText(FText::FromString(FString::FromInt(RemainingEnemies) + " enemies left." ));
	}
}

void UPlayerCharacterHUD::SetTimerText(int32 Timer)
{
	if(TimerText)
	{
		TimerText->SetText(FText::FromString(FString::FromInt(Timer) + " seconds until you can seek!" ));
	}
}

void UPlayerCharacterHUD::DeleteTimerText()
{
	if(TimerText)
	{
		TimerText->SetVisibility(ESlateVisibility::Hidden);
	}
}
