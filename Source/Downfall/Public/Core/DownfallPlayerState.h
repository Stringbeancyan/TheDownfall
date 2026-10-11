#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DownfallPlayerState.generated.h"

/** Public match identity. Team changes are owned by the server, never by a client RPC. */
UCLASS()
class DOWNFALL_API ADownfallPlayerState : public APlayerState
{

	GENERATED_BODY()

public:
	ADownfallPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	UFUNCTION(BlueprintPure, Category = "Downfall|Team")
	uint8 GetTeamId() const { return TeamId; }

	/** Authority only; assigning 255 is reserved for a neutral actor. */
	void SetTeamId(uint8 NewTeamId);

private:
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Downfall|Team")
	uint8 TeamId = 1;
};
