#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "DownfallGameMode.generated.h"

class ADownfallCharacter;

/** Four-player cooperative slice. Runs only on the server. */
UCLASS()
class DOWNFALL_API ADownfallGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ADownfallGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** Schedules a server respawn only while this player is still connected and dead. */
	void HandleCharacterDeath(ADownfallCharacter* DeadCharacter);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Downfall|Match", meta = (ClampMin = "1", ClampMax = "4"))
	int32 MaximumPlayers = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Downfall|Match", meta = (ClampMin = "1.0"))
	float RespawnDelay = 5.0f;
};
