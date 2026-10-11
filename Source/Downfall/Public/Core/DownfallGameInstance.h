#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "DownfallGameInstance.generated.h"

/** Lifecycle anchor for later online-session and party services. No online backend is fabricated. */
UCLASS()
class DOWNFALL_API UDownfallGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category = "Downfall|Network")
	FString GetLastNetworkError() const { return LastNetworkError; }

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType, const FString& ErrorString);

	UPROPERTY(Transient)
	FString LastNetworkError;

	FDelegateHandle NetworkFailureHandle;
};
