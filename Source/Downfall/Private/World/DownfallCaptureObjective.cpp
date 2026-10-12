#include "World/DownfallCaptureObjective.h"
#include "Combat/DownfallHealthComponent.h"
#include "Core/DownfallPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADownfallCaptureObjective::ADownfallCaptureObjective()
{
    bReplicates = true;
    bAlwaysRelevant = true; // Small objective count; strategic state remains relevant to all players.
    SetNetUpdateFrequency(4.f);
    PrimaryActorTick.bCanEverTick = false;
    CaptureVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureVolume"));
    RootComponent = CaptureVolume;
    CaptureVolume->SetBoxExtent(FVector(550, 550, 180));
    CaptureVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CaptureVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    CaptureVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CaptureVolume->SetGenerateOverlapEvents(true);
    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(RootComponent);
    Marker->SetRelativeLocation(FVector(0, 0, 5));
    Marker->SetRelativeScale3D(FVector(3, 3, .1));
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) Marker->SetStaticMesh(Cylinder.Object);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ObjectiveLabel"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 200));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(28);
    Label->SetText(FText::FromString(TEXT("RELAY / NEUTRAL")));
}

void ADownfallCaptureObjective::BeginPlay()
{
    Super::BeginPlay();
    OnRep_CaptureState();
    if (HasAuthority())
    {
        LastCaptureServerTime = GetWorld()->GetTimeSeconds();
        GetWorldTimerManager().SetTimer(CaptureTimer, this, &ThisClass::EvaluateCapture, .25f, true);
    }
}

void ADownfallCaptureObjective::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(CaptureTimer);
    Super::EndPlay(Reason);
}

void ADownfallCaptureObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADownfallCaptureObjective, State);
}

void ADownfallCaptureObjective::EvaluateCapture()
{
    if (!HasAuthority()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    // Cap hitch catch-up: a paused server must not grant seconds of unobserved occupancy.
    const float Delta = FMath::Clamp(static_cast<float>(Now - LastCaptureServerTime), 0.f, .5f);
    LastCaptureServerTime = Now;
    TArray<AActor*> Occupants;
    CaptureVolume->GetOverlappingActors(Occupants, APawn::StaticClass());
    TSet<uint8> Teams;
    for (AActor* Actor : Occupants)
    {
        const APawn* Pawn = Cast<APawn>(Actor);
        if (!Pawn || Pawn->IsHidden() || !Pawn->IsPlayerControlled()) continue;
        const ADownfallPlayerState* Player = Pawn->GetPlayerState<ADownfallPlayerState>();
        const UDownfallHealthComponent* Health = Pawn->FindComponentByClass<UDownfallHealthComponent>();
        if (Player && Player->GetTeamId() != 255 && Health && Health->IsAlive()) Teams.Add(Player->GetTeamId());
    }
    State.bContested = Teams.Num() > 1;
    if (Teams.Num() == 1)
    {
        const uint8 Team = *Teams.CreateConstIterator();
        if (Team == State.OwnerTeam)
        {
            State.Progress = 0.f;
            State.CapturingTeam = 255;
        }
        else
        {
            if (State.CapturingTeam != Team) { State.CapturingTeam = Team; State.Progress = 0.f; }
            State.Progress = FMath::Clamp(State.Progress + Delta / FMath::Max(1.f, CaptureSeconds), 0.f, 1.f);
            if (State.Progress >= 1.f)
            {
                State.OwnerTeam = Team;
                State.CapturingTeam = 255;
                State.Progress = 0.f;
            }
        }
    }
    else if (Teams.IsEmpty())
    {
        State.Progress = FMath::Max(0.f, State.Progress - FMath::Max(0.f, DecayPerSecond) * Delta);
        if (State.Progress <= 0.f) State.CapturingTeam = 255;
    }
    // Contested capture is frozen; death/disconnect/seat changes resolve at the next server poll.
    OnRep_CaptureState();
}

void ADownfallCaptureObjective::OnRep_CaptureState()
{
    if (GetNetMode() == NM_DedicatedServer) return;
    FString Status;
    if (State.bContested) Status = TEXT("RELAY / CONTESTED");
    else if (State.CapturingTeam != 255) Status = FString::Printf(TEXT("RELAY / TEAM %d / %d%%"), State.CapturingTeam, FMath::RoundToInt(State.Progress * 100.f));
    else if (State.OwnerTeam != 255) Status = FString::Printf(TEXT("RELAY / TEAM %d"), State.OwnerTeam);
    else Status = TEXT("RELAY / NEUTRAL");
    Label->SetText(FText::FromString(Status));
    Label->SetTextRenderColor(State.bContested ? FColor::Orange : State.OwnerTeam != 255 ? FColor::Green : FColor::Cyan);
}
