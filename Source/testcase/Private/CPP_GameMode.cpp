// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_GameMode.h"

ACPP_GameMode::ACPP_GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TRAIN/Core/Characters/BP_Player"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}