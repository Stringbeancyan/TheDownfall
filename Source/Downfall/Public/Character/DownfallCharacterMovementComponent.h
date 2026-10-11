#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DownfallCharacterMovementComponent.generated.h"

/** Sprint is a saved-move intention; clients never send speed, position corrections, or stamina. */
UCLASS()
class DOWNFALL_API UDownfallCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UDownfallCharacterMovementComponent();
	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	void SetWantsToSprint(bool bNewSprint) { bWantsToSprint = bNewSprint; }
	bool WantsToSprint() const { return bWantsToSprint; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Downfall|Movement", meta = (ClampMin = "100.0", ClampMax = "800.0"))
	float SprintSpeed = 630.0f;

	bool bWantsToSprint = false;
};

class FSavedMove_Downfall final : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character,
		float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float DeltaTime, FVector const& NewAcceleration,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;

private:
	bool bSavedWantsToSprint = false;
};

class FNetworkPredictionData_Client_Downfall final : public FNetworkPredictionData_Client_Character
{
public:
	using Super = FNetworkPredictionData_Client_Character;
	explicit FNetworkPredictionData_Client_Downfall(const UCharacterMovementComponent& Movement);
	virtual FSavedMovePtr AllocateNewMove() override;
};
