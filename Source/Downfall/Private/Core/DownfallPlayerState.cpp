#include "Core/DownfallPlayerState.h"

#include "Net/UnrealNetwork.h"

ADownfallPlayerState::ADownfallPlayerState()
{
	SetNetUpdateFrequency(5.0f);
}

void ADownfallPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADownfallPlayerState, TeamId);
}

void ADownfallPlayerState::SetTeamId(const uint8 NewTeamId)
{
	if (HasAuthority())
	{
		TeamId = NewTeamId;
		ForceNetUpdate();
	}
}

void ADownfallPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);
	if (ADownfallPlayerState* DownfallState = Cast<ADownfallPlayerState>(NewPlayerState))
	{
		DownfallState->SetTeamId(TeamId);
	}
}
