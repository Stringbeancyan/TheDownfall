#include "Character/DownfallCharacterMovementComponent.h"

#include "Character/DownfallCharacter.h"
#include "Combat/DownfallHealthComponent.h"

UDownfallCharacterMovementComponent::UDownfallCharacterMovementComponent()
{
	MaxWalkSpeed = 380.0f;
	MaxWalkSpeedCrouched = 180.0f;
	MaxAcceleration = 1600.0f;
	BrakingDecelerationWalking = 1600.0f;
	JumpZVelocity = 440.0f;
	AirControl = 0.2f;
	GetNavAgentPropertiesRef().bCanCrouch = true;
}

float UDownfallCharacterMovementComponent::GetMaxSpeed() const
{
	const ADownfallCharacter* Character = Cast<ADownfallCharacter>(CharacterOwner);
	if (Character && (!Character->GetHealthComponent()->IsAlive() || Character->IsSeated()))
	{
		return 0.0f;
	}
	if (bWantsToSprint && IsMovingOnGround() && !IsCrouching())
	{
		// This bound is applied identically during client prediction and authoritative server moves.
		return FMath::Clamp(SprintSpeed, MaxWalkSpeed, 800.0f);
	}
	return Super::GetMaxSpeed();
}

void UDownfallCharacterMovementComponent::UpdateFromCompressedFlags(const uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* UDownfallCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UDownfallCharacterMovementComponent* MutableThis =
			const_cast<UDownfallCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Downfall(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_Downfall::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
}

uint8 FSavedMove_Downfall::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Flags |= FLAG_Custom_0;
	}
	return Flags;
}

bool FSavedMove_Downfall::CanCombineWith(const FSavedMovePtr& NewMove,
	ACharacter* Character, const float MaxDelta) const
{
	const FSavedMove_Downfall* DownfallMove = static_cast<const FSavedMove_Downfall*>(NewMove.Get());
	return bSavedWantsToSprint == DownfallMove->bSavedWantsToSprint &&
		Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_Downfall::SetMoveFor(ACharacter* Character, const float DeltaTime,
	FVector const& NewAcceleration, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, DeltaTime, NewAcceleration, ClientData);
	if (const UDownfallCharacterMovementComponent* Movement =
		Cast<UDownfallCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		bSavedWantsToSprint = Movement->WantsToSprint();
	}
}

void FSavedMove_Downfall::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	if (UDownfallCharacterMovementComponent* Movement =
		Cast<UDownfallCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		Movement->SetWantsToSprint(bSavedWantsToSprint);
	}
}

FNetworkPredictionData_Client_Downfall::FNetworkPredictionData_Client_Downfall(
	const UCharacterMovementComponent& Movement)
	: Super(Movement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Downfall::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Downfall());
}
