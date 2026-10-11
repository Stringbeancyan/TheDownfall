#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "DownfallGameState.generated.h"

/** Replicated match-wide state. Actor-local objective and weather state live with their actors. */
UCLASS()
class DOWNFALL_API ADownfallGameState : public AGameState
{
	GENERATED_BODY()

public:
	ADownfallGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Downfall|Match")
	FName GetOperationId() const { return OperationId; }

	UFUNCTION(BlueprintPure, Category = "Downfall|Match")
	int32 GetPlayerCapacity() const { return PlayerCapacity; }

	void ConfigureOperation(FName NewOperationId, int32 NewPlayerCapacity);

private:
	UPROPERTY(Replicated)
	FName OperationId = TEXT("Breakwater");

	/** Slice configuration, not a claim that a higher player count has been measured. */
	UPROPERTY(Replicated)
	int32 PlayerCapacity = 4;
};
