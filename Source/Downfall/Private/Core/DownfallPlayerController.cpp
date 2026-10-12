#include "Core/DownfallPlayerController.h"

#include "Character/DownfallCharacter.h"
#include "Character/DownfallCharacterMovementComponent.h"
#include "Combat/DownfallCombatComponent.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

void ADownfallPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void ADownfallPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
			&ADownfallPlayerController::ToggleMouseCapture);
	}
}

void ADownfallPlayerController::ToggleMouseCapture()
{
	// Multiplayer keeps simulating while local input capture is released.
	bMenusCaptureMouse = !bMenusCaptureMouse;
	bShowMouseCursor = bMenusCaptureMouse;
	SetIgnoreLookInput(bMenusCaptureMouse);
	SetIgnoreMoveInput(bMenusCaptureMouse);
	if (bMenusCaptureMouse)
	{
		if (ADownfallCharacter* ControlledCharacter = Cast<ADownfallCharacter>(GetPawn()))
		{
			ControlledCharacter->GetCombatComponent()->StopFire();
			if (UDownfallCharacterMovementComponent* Movement =
				Cast<UDownfallCharacterMovementComponent>(ControlledCharacter->GetCharacterMovement()))
			{
				Movement->SetWantsToSprint(false);
			}
		}
		SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}
