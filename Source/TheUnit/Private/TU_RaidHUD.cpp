#include "TU_RaidHUD.h"
#include "TUBetaUserSettings.h"
#include "TU_GameState.h"
#include "TU_PlayerState.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TU_CommandCenterStation.h"
#include "TU_InteractableBase.h"
#include "TUWorldItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

float ATU_RaidHUD::DrawReadableText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale)
{
    if (!Canvas || !GEngine) return 0.f;
    UFont* Font = GEngine->GetMediumFont();
    float Width = 0.f, Height = 0.f;
    Canvas->StrLen(Font, Text, Width, Height);
    const float AvailableWidth = FMath::Max(1.f, Canvas->ClipX - X - 24.f);
    const float FitScale = FMath::Min(Scale, AvailableWidth / FMath::Max(Width, 1.f));
    // Canvas HUD is composed after tone mapping; this plate retains contrast on any scene.
    DrawRect(FLinearColor(.008f, .012f, .018f, .88f), X - 6.f, Y - 4.f, Width * FitScale + 12.f, Height * FitScale + 8.f);
    FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
    Item.Scale = FVector2D(FitScale, FitScale);
    Item.EnableShadow(FLinearColor::Black, FVector2D(1.f, 1.f));
    Canvas->DrawItem(Item);
    return Height * FitScale;
}

void ATU_RaidHUD::DrawHUD()
{
    auto KeyName=[](FName Id){const auto* S=UTUBetaUserSettings::Get();return S?S->EffectiveKey(Id).GetDisplayName().ToString():Id.ToString();};
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const float Left=24.f;
    float Row=30.f;
    auto Line=[&](const FString& Text, FLinearColor Color=FLinearColor::White)
    {
        Row += FMath::Max(22.f, DrawReadableText(Text,Color,Left,Row) + 10.f);
    };
    const ATU_ArmedOperatorCharacter* Operator=Cast<ATU_ArmedOperatorCharacter>(PlayerOwner->GetPawn());
    if (Operator && Operator->GetCurrentWeapon())
    {
        const ATU_WeaponBase* Weapon=Operator->GetCurrentWeapon();
        const FMagazineState Ammo=Weapon->GetMagazineState();
        Line(FString::Printf(TEXT("%s  |  %d in magazine  |  Chamber %s"),
            *Weapon->GetWeaponDefinition().DisplayName.ToString(),Ammo.RoundsInMagazine,Ammo.bRoundChambered?TEXT("loaded"):TEXT("empty")));
        const FTUWeaponActionState Action=Weapon->GetActionState();
        if (Action.bActive) Line(TEXT("Reloading"),FLinearColor(1.f,.8f,.2f));
        else if (Action.Phase!=ETUWeaponActionPhase::Ready) Line(TEXT("Reload interrupted - R to resume"),FLinearColor(1.f,.8f,.2f));
        else if (Weapon->IsMuzzleObstructed()) Line(TEXT("Weapon obstructed"),FLinearColor(1.f,.3f,.2f));
        else if (!Operator->IsWeaponRaised()) Line(TEXT("Weapon lowered - H to change ready position"));
    }
    const ATU_GameState* Raid=GetWorld()->GetGameState<ATU_GameState>();
    const ATU_PlayerState* PS=PlayerOwner->GetPlayerState<ATU_PlayerState>();
    if (Raid && PS && Raid->RaidId.IsValid())
    {
        const int32 Remaining=FMath::Max(0,FMath::CeilToInt(Raid->RaidEndTime-Raid->RaidElapsedTime));
        Line(FString::Printf(TEXT("%s  |  %02d:%02d remaining"),Raid->bTraining?TEXT("TRAINING - disposable kit"):TEXT("LIVE RAID"),Remaining/60,Remaining%60));
        const FTURaidParticipantState* Participant=Raid->Participants.FindByPredicate([&](const auto& P){return P.PlayerId==PS->PersistentPlayerId;});
        if (Participant)
        {
            if (Participant->bSavePending) Line(TEXT("Securing your result. Please wait; your kit is locked."),FLinearColor(1.f,.8f,.2f));
            else if (Participant->bExtracting) Line(FString::Printf(TEXT("EXTRACTING - %.1f seconds. Stay inside the exit."),FMath::Max(0.f,Participant->ExtractionEndTime-Raid->RaidElapsedTime)),FLinearColor(.3f,1.f,.4f));
            else if (Participant->Outcome==ETURaidPlayerOutcome::Extracted) Line(TEXT("Extracted. Teammates may still be in the raid."),FLinearColor(.3f,1.f,.4f));
            else if (Participant->Outcome==ETURaidPlayerOutcome::Dead) Line(TEXT("Operator lost. Deployed equipment is lost."),FLinearColor(1.f,.3f,.2f));
            for (const FTUTaskProgress& Task:Participant->Tasks)
            {
                FString Label=Task.Definition.TaskId.ToString().Replace(TEXT("_"),TEXT(" "));
                int32 Required=Task.Definition.RequiredCount;
                if(!Task.Definition.Steps.IsEmpty()) { Required=0; for(const auto& Step:Task.Definition.Steps) Required+=Step.RequiredCount; }
                Line(FString::Printf(TEXT("%s: %d / %d%s"),*Label,Task.CommittedCount+Task.PendingCount,Required,Task.bCompleted?TEXT("  COMPLETE"):TEXT("")));
            }
            Line(TEXT("Tasks are optional for ordinary extraction."),FLinearColor(.7f,.8f,.85f));
        }
    }
    if (Operator && !Operator->IsCombatDisabled())
    {
        FVector View; FRotator Rotation; PlayerOwner->GetPlayerViewPoint(View,Rotation);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(RaidHUDInteraction),false,Operator); FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,View,View+Rotation.Vector()*400.f,ECC_Visibility,Params))
        {
            FString Prompt;
            if(const auto* Station=Cast<ATU_CommandCenterStation>(Hit.GetActor())) Prompt=KeyName(TEXT("Interact"))+TEXT(" - ")+Station->GetStationLabel().ToString();
            else if(const auto* Object=Cast<ATU_InteractableBase>(Hit.GetActor())) Prompt=KeyName(TEXT("Interact"))+TEXT(" - ")+Object->TargetId.ToString();
            else if(Cast<ATUWorldItem>(Hit.GetActor())) Prompt=KeyName(TEXT("Interact"))+TEXT(" - Pick up magazine");
            if(!Prompt.IsEmpty()) DrawReadableText(Prompt,FLinearColor::White,Canvas->ClipX*.35f,Canvas->ClipY*.65f);
        }
    }
    const FString Hints=FString::Printf(TEXT("%s Reload   %s Drop reload   %s Inspect   %s Cycle   %s Ready   %s Interact   Esc Menu"),*KeyName(TEXT("Reload")),*KeyName(TEXT("EmergencyReload")),*KeyName(TEXT("InspectWeapon")),*KeyName(TEXT("CycleAction")),*KeyName(TEXT("CycleReady")),*KeyName(TEXT("Interact")));
    DrawReadableText(Hints,FLinearColor(.88f,.92f,.96f),Left,Canvas->ClipY-38.f,.9f);
}
