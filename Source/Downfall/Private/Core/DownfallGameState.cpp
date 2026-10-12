#include "Core/DownfallGameState.h"

#include "Net/UnrealNetwork.h"

ADownfallGameState::ADownfallGameState()
{
	SetNetUpdateFrequency(5.0f);
}

void ADownfallGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADownfallGameState, OperationId);
	DOREPLIFETIME(ADownfallGameState, PlayerCapacity);
}

void ADownfallGameState::ConfigureOperation(const FName NewOperationId, const int32 NewPlayerCapacity)
{
	if (HasAuthority())
	{
		OperationId = NewOperationId;
		PlayerCapacity = FMath::Clamp(NewPlayerCapacity, 1, 4);
		ForceNetUpdate();
	}
}
