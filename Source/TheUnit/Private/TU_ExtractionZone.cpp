#include "TU_ExtractionZone.h"
#include "TU_GameMode.h"
#include "TU_GameState.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
ATU_ExtractionZone::ATU_ExtractionZone()
{
    bReplicates = true;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionTrigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(220.f,220.f,150.f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
}
void ATU_ExtractionZone::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this,&ATU_ExtractionZone::HandleBeginOverlap);
    Trigger->OnComponentEndOverlap.AddDynamic(this,&ATU_ExtractionZone::HandleEndOverlap);
}
void ATU_ExtractionZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATU_ExtractionZone,bPowered);
    DOREPLIFETIME(ATU_ExtractionZone,UsedCapacity);
}
bool ATU_ExtractionZone::ContainsPawn(const APawn* Pawn) const
{
    if (!Pawn || !Trigger) return false;
    const FVector P = Trigger->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
    const FVector E = Trigger->GetUnscaledBoxExtent();
    return FMath::Abs(P.X)<=E.X && FMath::Abs(P.Y)<=E.Y && FMath::Abs(P.Z)<=E.Z;
}
bool ATU_ExtractionZone::IsEligible(APawn* Pawn,const ATU_GameMode* Mode) const
{
    if (!HasAuthority() || !Mode || !ContainsPawn(Pawn) || (bRequiresPower && !bPowered) ||
        (Capacity>0 && UsedCapacity>=Capacity) || Mode->GetRaidElapsedTime()<OpensAtSeconds ||
        (ClosesAtSeconds>0.f && Mode->GetRaidElapsedTime()>=ClosesAtSeconds)) return false;
    if (!RequiredItemId.IsNone())
    {
        const FTUItemLedger* Ledger=Mode->GetParticipantLedger(Pawn);
        if (!Ledger || !Ledger->Items.ContainsByPredicate([&](const FTUItemInstance& Item)
            { return Item.DefinitionId==RequiredItemId && Item.Location==ETUItemLocation::Carried; })) return false;
    }
    return true;
}
void ATU_ExtractionZone::CommitCapacity() { if (HasAuthority()) { ++UsedCapacity; ForceNetUpdate(); } }
void ATU_ExtractionZone::HandleBeginOverlap(UPrimitiveComponent*,AActor* Actor,UPrimitiveComponent*,int32,bool,const FHitResult&)
{
    if (ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>()) Mode->BeginExtraction(Cast<APawn>(Actor),this);
}
void ATU_ExtractionZone::HandleEndOverlap(UPrimitiveComponent*,AActor* Actor,UPrimitiveComponent*,int32)
{
    if (ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>()) Mode->CancelExtraction(Cast<APawn>(Actor));
}
bool ATU_ExtractionZone::ExtractNow(bool)
{
    // Compatibility entry still requires each participant's completed authority countdown.
    if (!HasAuthority() || !GetWorld()) return false;
    ATU_GameMode* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>();
    if (!Mode) return false;
    bool Result=false;
    for (TActorIterator<APawn> It(GetWorld());It;++It)
    {
        const FTURaidParticipantState* State=Mode->FindParticipant(*It);
        if (State && State->bExtracting && State->ExtractId==ExtractId && State->ExtractionEndTime<=Mode->GetRaidElapsedTime() && IsEligible(*It,Mode))
            Result=Mode->ResolvePlayerOutcome(*It,ETURaidPlayerOutcome::Extracted,ExtractId)||Result;
    }
    return Result;
}
bool ATU_ExtractionZone::IsExtractionPending() const
{
    const ATU_GameState* State=GetWorld()?GetWorld()->GetGameState<ATU_GameState>():nullptr;
    return State && State->Participants.ContainsByPredicate([&](const FTURaidParticipantState& P){return P.bExtracting && P.ExtractId==ExtractId;});
}
