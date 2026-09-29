#include "TUWeaponPartsComponent.h"
#include "TUWeaponContactProfile.h"
#include "TU_WeaponBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"
namespace
{
float Smooth(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T*T*(3.f-2.f*T); }
const FVector StowOffset(-8.f, -6.f, -22.f);
}
UTUWeaponPartsComponent::UTUWeaponPartsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> R(TEXT("/Game/TheUnit/Weapons/ContactRifle/SM_TURifleContactReceiver.SM_TURifleContactReceiver"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> M(TEXT("/Game/TheUnit/Weapons/PrototypeRifle/SM_TURifleMagazine.SM_TURifleMagazine"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> A(TEXT("/Game/TheUnit/Weapons/ContactRifle/SM_TURifleAction.SM_TURifleAction"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> C(TEXT("/Game/TheUnit/Weapons/ContactRifle/SM_TURifleControl.SM_TURifleControl"));
    ReceiverAsset=R.Object;MagazineAsset=M.Object;ActionAsset=A.Object;ControlAsset=C.Object;
}
void UTUWeaponPartsComponent::BeginPlay()
{
    Super::BeginPlay();
    Weapon = Cast<ATU_WeaponBase>(GetOwner());
    if (Weapon) Weapon->OnShotFired.AddUniqueDynamic(this, &UTUWeaponPartsComponent::OnShot);
}
void UTUWeaponPartsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Weapon) Weapon->OnShotFired.RemoveDynamic(this, &UTUWeaponPartsComponent::OnShot);
    Super::EndPlay(Reason);
}
FTUWeaponPartsFrame UTUWeaponPartsComponent::Evaluate(const FTUItemLedger& Ledger,
    const FTUWeaponActionState& Action,float PhaseProgress)
{
    FTUWeaponPartsFrame R;
    if(Ledger.Weapons.Num()!=1) return R;
    const auto& W=Ledger.Weapons[0];
    auto Find=[&](const FGuid& Id,ETUItemLocation Location) {
        return Id.IsValid() && Ledger.Magazines.ContainsByPredicate([&](const auto& M){
            return M.InstanceId==Id && M.OwnerId==W.OwnerId && M.Location==Location;
        });
    };
    if(Find(W.InsertedMagazineId,ETUItemLocation::Inserted)) R.InsertedId=W.InsertedMagazineId;
    R.ActionTravelCm=W.bActionOpen?3.f:0.f;
    if(Action.WeaponId!=W.InstanceId) return R;
    const float P=FMath::Clamp(PhaseProgress,0.f,1.f),T=Smooth(P);
    auto Held=[&](const FVector& Offset) {
        R.MovingInActor.SetTranslation(Offset);
        R.HandOffset=Offset;
        R.HandInActor=TUWeaponContact::MagazineHandInActor()*R.MovingInActor;
        R.HandContactAlpha=1.f; R.MagazineGripAlpha=1.f;
    };
    R.HandInActor=TUWeaponContact::MagazineHandInActor();
    if(Action.Phase==ETUWeaponActionPhase::Begin && Action.bActive) {
        R.HandContactAlpha=Smooth(P/.65f);
        R.MagazineGripAlpha=Smooth((P-.25f)/.45f);
        if(!R.InsertedId.IsValid()) { R.HandInActor.AddToTranslation(StowOffset*T); R.MagazineGripAlpha=0.f; }
    }
    else if(Action.Phase==ETUWeaponActionPhase::Removed && Action.bActive) {
        Held(StowOffset*T);
        if(Action.Policy==ETUReloadPolicy::Retain && Find(Action.OutgoingMagazineId,ETUItemLocation::Carried)) R.MovingId=Action.OutgoingMagazineId;
        else R.MagazineGripAlpha=0.f;
    }
    else if(Action.Phase==ETUWeaponActionPhase::Acquired && Find(Action.ReplacementMagazineId,ETUItemLocation::InHand)) {
        Held(StowOffset*(1.f-T)); R.MovingId=Action.ReplacementMagazineId;
    }
    else if(Action.bActive && Action.Phase==ETUWeaponActionPhase::Inserted) {
        const bool Cycle=Action.bRequiresChamberCycle || W.ChamberAmmoId.IsNone();
        R.MagazineGripAlpha=1.f-Smooth((P-.15f)/.25f);
        if(Cycle) {
            R.HandContactAlpha=1.f;
            R.ControlGripAlpha=Smooth((P-.2f)/.5f);
            R.HandInActor.Blend(TUWeaponContact::MagazineHandInActor(),TUWeaponContact::ControlHandInActor(),R.ControlGripAlpha);
            R.ControlPress=Smooth((P-.72f)/.28f);
        } else R.HandContactAlpha=1.f-Smooth((P-.2f)/.8f);
        // Seating reaction is presentation only and never changes the ledger.
        R.SeatReaction=FMath::Sin(FMath::Clamp(P/.35f,0.f,1.f)*PI)*.30f;
    }
    else if(Action.bActive && Action.Phase==ETUWeaponActionPhase::Chambered && Action.bRequiresChamberCycle) {
        R.HandInActor=TUWeaponContact::ControlHandInActor();
        R.HandContactAlpha=1.f-T; R.ControlGripAlpha=1.f-T;
        R.ControlPress=1.f-Smooth(P/.25f);
        R.ActionTravelCm=3.f*(1.f-Smooth(P/.12f));
    }
    return R;
}
bool UTUWeaponPartsComponent::Configure()
{
    if (!Weapon) Weapon = Cast<ATU_WeaponBase>(GetOwner());
    UStaticMeshComponent* Source = Weapon ? Weapon->GetWeaponBodyMesh() : nullptr;
    const bool Match = Weapon && Weapon->GetWeaponDefinition().WeaponId == TEXT("WPN_TU556_ModularCarbine") && Source && Source->GetStaticMesh() && ReceiverAsset && MagazineAsset && ActionAsset && ControlAsset &&
        Source->GetStaticMesh()->GetPathName() == TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle");
    if (!Match) {
        if (bSupported && Source) Source->SetVisibility(true, false);
        if (VisualRoot) VisualRoot->SetVisibility(false, true);
        bSupported = false; return false;
    }
    if (!VisualRoot) {
        Weapon->OnShotFired.AddUniqueDynamic(this, &UTUWeaponPartsComponent::OnShot);
        VisualRoot = NewObject<USceneComponent>(Weapon, TEXT("RiflePartsVisualRoot"));
        Weapon->AddInstanceComponent(VisualRoot);
        VisualRoot->SetupAttachment(Weapon->GetRootComponent()); VisualRoot->RegisterComponent();
        auto Make = [&](const TCHAR* Name, UStaticMesh* Asset) {
            auto* Mesh = NewObject<UStaticMeshComponent>(Weapon, Name);
            Weapon->AddInstanceComponent(Mesh); Mesh->SetupAttachment(VisualRoot);
            Mesh->SetStaticMesh(Asset); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetGenerateOverlapEvents(false); Mesh->SetCastShadow(true); Mesh->SetVisibility(false);
            Mesh->RegisterComponent(); return Mesh;
        };
        ReceiverVisual = Make(TEXT("RifleReceiverVisual"), ReceiverAsset);
        InsertedVisual = Make(TEXT("RifleInsertedMagazineVisual"), MagazineAsset);
        MovingVisual=Make(TEXT("RifleMovingMagazineVisual"),MagazineAsset);
        ActionVisual=Make(TEXT("RifleActionVisual"),ActionAsset);
        ControlVisual=Make(TEXT("RifleControlVisual"),ControlAsset);
    }
    Source->SetVisibility(false, false); VisualRoot->SetVisibility(true, false);
    ReceiverVisual->SetVisibility(true); bSupported = true; return true;
}
void UTUWeaponPartsComponent::UpdatePresentation(float DeltaSeconds)
{
    if (!Configure()) return;
    WeaponKickCm *= FMath::Exp(-22.f * FMath::Max(0.f, DeltaSeconds));
    const float Kick = bWeaponRecoilEnabled ? WeaponKickCm : 0.f;
    VisualRoot->SetRelativeTransform(FTransform(FRotator(Kick * .45f, 0, 0), FVector(-Kick, 0, Kick * .12f)));
    const auto Action = Weapon->GetActionState();
    const auto Ledger = Weapon->ExportItemLedger();
    if (Ledger.Revision != Action.Revision) return; // Preserve the last coherent visual snapshot.
    const float Now = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const float Duration = FMath::Max(.001f, Weapon->GetPresentationPhaseDuration());
    float Progress = Action.bActive ? FMath::Clamp(1.f-(Action.PhaseEndServerTime-Now)/Duration,0.f,1.f) : 0.f;
    if (!Action.bActive && Action.ActionId == LastActionId && Action.Phase == LastPhase) Progress = LastProgress;
    LastActionId = Action.ActionId; LastPhase = Action.Phase; LastProgress = Progress;
    Frame=Evaluate(Ledger,Action,Progress);
    const float Load=Action.bActive && (Action.Phase==ETUWeaponActionPhase::Removed || Action.Phase==ETUWeaponActionPhase::Acquired)?1.5f:0.f;
    SupportLoadRoll=FMath::Lerp(SupportLoadRoll,Load,1.f-FMath::Exp(-12.f*FMath::Max(0.f,DeltaSeconds)));
    VisualRoot->SetRelativeTransform(FTransform(FRotator(Kick*.45f+Frame.SeatReaction,0,SupportLoadRoll),FVector(-Kick-Frame.SeatReaction,0,Kick*.12f)));
    const FTransform MeshInActor = FTransform(FRotator(0.f,0.f,90.f)) * Weapon->GetWeaponBodyMesh()->GetRelativeTransform();
    ReceiverVisual->SetRelativeTransform(Weapon->GetWeaponBodyMesh()->GetRelativeTransform());
    InsertedVisual->SetRelativeTransform(MeshInActor);
    ActionVisual->SetRelativeTransform(Weapon->GetWeaponBodyMesh()->GetRelativeTransform()*FTransform(FVector(-Frame.ActionTravelCm,0,0)));
    ControlVisual->SetRelativeTransform(Weapon->GetWeaponBodyMesh()->GetRelativeTransform()*FTransform(FVector(0,Frame.ControlPress*.2f,0)));
    ActionVisual->SetVisibility(true);ControlVisual->SetVisibility(true);
    MovingVisual->SetRelativeTransform(MeshInActor * Frame.MovingInActor);
    InsertedVisual->SetVisibility(Frame.InsertedId.IsValid());
    MovingVisual->SetVisibility(Frame.MovingId.IsValid() && Frame.MovingId != Frame.InsertedId);
}
FTransform UTUWeaponPartsComponent::GetVisibleActorWorld() const
{
    return bSupported && VisualRoot ? VisualRoot->GetComponentTransform() : (Weapon ? Weapon->GetActorTransform() : FTransform::Identity);
}
FTransform UTUWeaponPartsComponent::GetVisibleMeshWorld() const
{
    return bSupported && VisualRoot ? Weapon->GetWeaponBodyMesh()->GetRelativeTransform() * VisualRoot->GetComponentTransform() : (Weapon ? Weapon->GetWeaponBodyMesh()->GetComponentTransform() : FTransform::Identity);
}
void UTUWeaponPartsComponent::OnShot(FTUWeaponShotResult Result)
{
    if (Result.bFired) WeaponKickCm = FMath::Min(WeaponKickCm + .8f, 2.4f);
}
