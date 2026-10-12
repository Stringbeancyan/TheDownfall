#include "Presentation/DownfallModelComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Combat/DownfallHealthComponent.h"

UDownfallModelComponent::UDownfallModelComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.f / 30.f;
}

void UDownfallModelComponent::OnRegister()
{
    Super::OnRegister();
    if (!GetOwner() || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    if (Parts.IsEmpty()) BuildModel();
    PreviousLocation = GetOwner()->GetActorLocation();
}

void UDownfallModelComponent::OnUnregister()
{
    for (UStaticMeshComponent* ModelPart : Parts)
        if (IsValid(ModelPart)) ModelPart->DestroyComponent();
    Parts.Reset();
    MovingParts.Reset();
    RestLocations.Reset();
    Super::OnUnregister();
}

UStaticMeshComponent* UDownfallModelComponent::Part(const TCHAR* Name, const TCHAR* Shape,
    FVector Position, FVector Dimensions, FLinearColor Color, float Metallic, FRotator Rotation)
{
    UStaticMeshComponent* ModelPart = NewObject<UStaticMeshComponent>(GetOwner(),
        MakeUniqueObjectName(GetOwner(), UStaticMeshComponent::StaticClass(), FName(Name)), RF_Transient);
    ModelPart->SetupAttachment(this);
    ModelPart->SetMobility(EComponentMobility::Movable);
    const FString MeshPath = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape);
    ModelPart->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *MeshPath));
    ModelPart->SetRelativeLocation(Position);
    ModelPart->SetRelativeRotation(Rotation);
    ModelPart->SetRelativeScale3D(Dimensions / 100.f);
    ModelPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ModelPart->SetGenerateOverlapEvents(false);
    ModelPart->SetCanEverAffectNavigation(false);
    // Generated with the scene. BasicShapeMaterial remains a visible fallback.
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Downfall/Generated/Materials/M_ModelPaint.M_ModelPaint"));
    if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Base)
    {
        UMaterialInstanceDynamic* Surface = UMaterialInstanceDynamic::Create(Base, ModelPart);
        Surface->SetVectorParameterValue(TEXT("BaseColor"), Color);
        Surface->SetScalarParameterValue(TEXT("Metallic"), Metallic);
        Surface->SetScalarParameterValue(TEXT("Roughness"), Metallic > .5f ? .32f : .72f);
        ModelPart->SetMaterial(0, Surface);
    }
    Parts.Add(ModelPart);
    ModelPart->RegisterComponent();
    return ModelPart;
}

void UDownfallModelComponent::BuildModel()
{
    // Keep gameplay shapes and weapon proxies, replace only body placeholders.
    TInlineComponentArray<UStaticMeshComponent*> ExistingMeshes;
    GetOwner()->GetComponents(ExistingMeshes);
    for (UStaticMeshComponent* Mesh : ExistingMeshes)
    {
        const FString Name = Mesh->GetName();
        if (Name.StartsWith(TEXT("Prototype")) && !Name.Contains(TEXT("Weapon")))
            Mesh->SetVisibility(false);
    }
    const FLinearColor Steel(.16f, .18f, .18f), Dark(.022f, .028f, .031f);
    const FLinearColor Paint(.23f, .27f, .23f), Glass(.055f, .095f, .12f);
    const FLinearColor Amber(.92f, .50f, .09f);
    if (ModelKind == EDownfallModelKind::UtilityVehicle)
    {
        Part(TEXT("Undertray"), TEXT("Cube"), FVector(0,0,-13), FVector(418,160,24), Dark);
        Part(TEXT("LowerBody"), TEXT("Cube"), FVector(0,0,12), FVector(420,176,52), Paint, .6f);
        Part(TEXT("Hood"), TEXT("Cube"), FVector(137,0,40), FVector(125,169,13), Paint, .6f, FRotator(-4,0,0));
        Part(TEXT("RearDeck"), TEXT("Cube"), FVector(-164,0,43), FVector(83,169,14), Paint, .6f);
        Part(TEXT("Roof"), TEXT("Cube"), FVector(-23,0,105), FVector(186,148,9), Paint, .6f);
        Part(TEXT("Windshield"), TEXT("Cube"), FVector(86,0,78), FVector(5,144,62), Glass, .75f, FRotator(-24,0,0));
        Part(TEXT("RearWindow"), TEXT("Cube"), FVector(-128,0,77), FVector(5,144,57), Glass, .75f, FRotator(22,0,0));
        for (int32 Side : {-1, 1})
        {
            Part(TEXT("SideGlass"), TEXT("Cube"), FVector(-20,Side*77,79), FVector(184,3,40), Glass, .65f);
            Part(TEXT("Bpillar"), TEXT("Cube"), FVector(-27,Side*79,78), FVector(8,5,51), Dark);
            Part(TEXT("DoorSeam"), TEXT("Cube"), FVector(-27,Side*89,28), FVector(2,2,46), Dark);
            for (float X : {-80.f, 30.f})
                Part(TEXT("DoorHandle"), TEXT("Cube"), FVector(X,Side*91,43), FVector(18,4,3), Steel, .85f);
            Part(TEXT("Mirror"), TEXT("Cube"), FVector(64,Side*99,69), FVector(22,17,12), Paint, .6f);
            Part(TEXT("Headlamp"), TEXT("Cube"), FVector(212,Side*60,29), FVector(4,35,14), FLinearColor(.85f,.83f,.68f), .65f);
            Part(TEXT("TailLamp"), TEXT("Cube"), FVector(-212,Side*65,29), FVector(4,26,14), FLinearColor(.4f,.025f,.015f));
            Part(TEXT("Sill"), TEXT("Cube"), FVector(0,Side*92,-7), FVector(233,8,10), Dark);
        }
        Part(TEXT("Grille"), TEXT("Cube"), FVector(213,0,19), FVector(5,73,25), Dark);
        for (int32 Bar = -3; Bar <= 3; ++Bar)
            Part(TEXT("GrilleFin"), TEXT("Cube"), FVector(217,Bar*10,19), FVector(3,2,23), Steel, .85f);
        for (int32 End : {-1,1})
            Part(TEXT("Bumper"), TEXT("Cube"), FVector(End*216,0,-2), FVector(9,171,14), Dark);
        for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
        {
            const FVector Pos(WheelIndex < 2 ? 140 : -140, WheelIndex % 2 ? 96 : -96, -25);
            UStaticMeshComponent* Tire = Part(TEXT("Tire"), TEXT("Cylinder"), Pos, FVector(75,75,27), Dark, 0, FRotator(0,0,90));
            MovingParts.Add(Tire);
            RestLocations.Add(Pos);
            const FVector RimPos = Pos + FVector(0, WheelIndex % 2 ? 15 : -15, 0);
            Part(TEXT("Rim"), TEXT("Cylinder"), RimPos, FVector(44,44,3), Steel, .85f, FRotator(0,0,90));
            Part(TEXT("Hub"), TEXT("Cylinder"), RimPos + FVector(0,WheelIndex%2 ? 2 : -2,0), FVector(16,16,5), Dark, .5f, FRotator(0,0,90));
        }
        return;
    }

    const bool bRobot = ModelKind == EDownfallModelKind::Sentinel;
    const FLinearColor BodyColor = bRobot ? Steel : Paint;
    Part(TEXT("Torso"), TEXT("Sphere"), FVector(0,0,3), FVector(44,52,66), Dark);
    Part(TEXT("ChestPlate"), TEXT("Cube"), FVector(16,0,10), FVector(16,47,42), BodyColor, bRobot ? .7f : .1f);
    Part(TEXT("BackPack"), TEXT("Cube"), FVector(-23,0,0), FVector(18,40,48), BodyColor);
    Part(TEXT("Belt"), TEXT("Cube"), FVector(0,0,-29), FVector(35,49,10), Dark);
    Part(TEXT("Collar"), TEXT("Cylinder"), FVector(0,0,44), FVector(22,22,12), Dark);
    Part(TEXT("Helmet"), TEXT("Sphere"), FVector(0,0,63), FVector(34,34,36), BodyColor, bRobot ? .65f : .15f);
    Part(TEXT("Visor"), TEXT("Cube"), FVector(16,0,65), FVector(6,28,12), bRobot ? Amber : Glass, .55f);
    for (int32 Side : {-1, 1})
    {
        Part(TEXT("Shoulder"), TEXT("Sphere"), FVector(0,Side*30,26), FVector(23,23,24), BodyColor, .3f);
        Part(TEXT("Forearm"), TEXT("Cube"), FVector(15,Side*28,1), FVector(30,15,17), BodyColor, .2f, FRotator(-30,0,0));
        Part(TEXT("Glove"), TEXT("Sphere"), FVector(32,Side*22,5), FVector(17,13,15), Dark);
        Part(TEXT("HipJoint"), TEXT("Sphere"), FVector(0,Side*16,-36), FVector(22,22,22), Dark);
        UStaticMeshComponent* Leg = Part(TEXT("Leg"), TEXT("Cube"), FVector(0,Side*16,-57), FVector(24,20,43), BodyColor, bRobot ? .6f : 0);
        MovingParts.Add(Leg);
        RestLocations.Add(Leg->GetRelativeLocation());
        Part(TEXT("KneeGuard"), TEXT("Sphere"), FVector(12,Side*16,-58), FVector(16,22,22), Dark);
        Part(TEXT("Boot"), TEXT("Cube"), FVector(5,Side*16,-81), FVector(35,22,20), Dark);
    }
    for (int32 Pouch = -1; Pouch <= 1; ++Pouch)
        Part(TEXT("ChestEquipment"), TEXT("Cube"), FVector(27,Pouch*14,4), FVector(9,11,22), Dark);
}

void UDownfallModelComponent::TickComponent(float DeltaSeconds, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (!GetOwner() || !GetWorld() || !GetWorld()->IsGameWorld() || Parts.IsEmpty()) return;
    if (const UDownfallHealthComponent* Health = GetOwner()->FindComponentByClass<UDownfallHealthComponent>())
    {
        if (!Health->IsAlive())
        {
            SetRelativeRotation(FRotator(0,0,80));
            SetRelativeLocation(FVector(0,0,-50));
            return;
        }
    }
    const FVector CurrentLocation = GetOwner()->GetActorLocation();
    const float Travel = FVector::Dist2D(CurrentLocation, PreviousLocation);
    PreviousLocation = CurrentLocation;
    // Distance-based cosmetics agree for driver and remote snapshots. Teleports
    // never produce a huge gait step. Animation is deliberately a kit rehearsal.
    MotionPhase = FMath::Fmod(MotionPhase + FMath::Min(Travel, 60.f) / 37.5f, 2.f * PI);
    for (int32 Index = 0; Index < MovingParts.Num(); ++Index)
    {
        if (ModelKind == EDownfallModelKind::UtilityVehicle)
            MovingParts[Index]->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(MotionPhase),0,90));
        else
        {
            const float Stride = Travel > .1f ? FMath::Sin(MotionPhase + Index * PI) : 0.f;
            MovingParts[Index]->SetRelativeLocation(RestLocations[Index] + FVector(Stride*9,0,FMath::Abs(Stride)*3));
            MovingParts[Index]->SetRelativeRotation(FRotator(Stride*18,0,0));
        }
    }
}
