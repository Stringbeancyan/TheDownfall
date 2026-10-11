#include "Core/DownfallGameMode.h"

#include "Character/DownfallCharacter.h"
#include "Combat/DownfallHealthComponent.h"
#include "Core/DownfallGameState.h"
#include "Core/DownfallPlayerController.h"
#include "Core/DownfallPlayerState.h"
#include "Presentation/DownfallHUD.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "TimerManager.h"

ADownfallGameMode::ADownfallGameMode()
{
	DefaultPawnClass = ADownfallCharacter::StaticClass();
	PlayerControllerClass = ADownfallPlayerController::StaticClass();
	PlayerStateClass = ADownfallPlayerState::StaticClass();
	GameStateClass = ADownfallGameState::StaticClass();
	HUDClass = ADownfallHUD::StaticClass();
	bUseSeamlessTravel = true;
}

void ADownfallGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	MaximumPlayers = FMath::Clamp(MaximumPlayers, 1, 4);
	if (GameSession)
	{
		GameSession->MaxPlayers = MaximumPlayers;
	}
}

void ADownfallGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaximumPlayers)
	{
		ErrorMessage = TEXT("The four-player prototype session is full.");
	}
}

void ADownfallGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (ADownfallPlayerState* State = NewPlayer->GetPlayerState<ADownfallPlayerState>())
	{
		State->SetTeamId(1);
	}
	if (ADownfallGameState* State = GetGameState<ADownfallGameState>())
	{
		State->ConfigureOperation(TEXT("Breakwater"), MaximumPlayers);
	}
}

void ADownfallGameMode::HandleCharacterDeath(ADownfallCharacter* DeadCharacter)
{
	if (!IsValid(DeadCharacter) || !HasAuthority())
	{
		return;
	}
	APlayerController* Player = Cast<APlayerController>(DeadCharacter->GetController());
	if (!Player)
	{
		return;
	}
	const TWeakObjectPtr<APlayerController> WeakPlayer(Player);
	const TWeakObjectPtr<ADownfallCharacter> WeakCharacter(DeadCharacter);
	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakPlayer, WeakCharacter]()
		{
			APlayerController* Controller = WeakPlayer.Get();
			ADownfallCharacter* Character = WeakCharacter.Get();
			if (!IsValid(Controller) || !IsValid(Character) ||
				Controller->GetPawn() != Character || Character->GetHealthComponent()->IsAlive())
			{
				return;
			}
			Controller->UnPossess();
			Character->Destroy();
			RestartPlayer(Controller);
		}), FMath::Max(RespawnDelay, 1.0f), false);
}
