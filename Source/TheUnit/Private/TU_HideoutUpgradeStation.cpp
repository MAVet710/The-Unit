#include "TU_HideoutUpgradeStation.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutCommandCenter.h"
#include "TU_HideoutCommandCenterDecorator.h"
#include "TU_ArmedOperatorCharacter.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

ATU_HideoutUpgradeStation::ATU_HideoutUpgradeStation()
{
    StationLabel = FText::FromString(TEXT("HIDEOUT UPGRADE"));

    StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("UpgradeStatus"));
    StatusText->SetupAttachment(StationMesh);
    StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    StatusText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    StatusText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    StatusText->SetWorldSize(22.0f);
    StatusText->SetText(FText::FromString(TEXT("HIDEOUT UPGRADE")));
}

void ATU_HideoutUpgradeStation::BeginPlay()
{
    Super::BeginPlay();
    RefreshStatus();
}

void ATU_HideoutUpgradeStation::ConfigureUpgradeStation(ETUHideoutModuleType InModuleType, const FText& InLabel)
{
    ModuleType = InModuleType;
    StationLabel = InLabel;
    RefreshStatus();
}

UTUHideoutProgressionComponent* ATU_HideoutUpgradeStation::FindProgression() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    for (TActorIterator<ATU_HideoutCommandCenter> It(GetWorld()); It; ++It)
    {
        if (ATU_HideoutCommandCenter* Hideout = *It)
        {
            return Hideout->GetHideoutProgression();
        }
    }
    return nullptr;
}

void ATU_HideoutUpgradeStation::RefreshStatus()
{
    if (!StatusText)
    {
        return;
    }

    const UTUHideoutProgressionComponent* Progression = FindProgression();
    const int32 Level = Progression ? Progression->GetModuleLevel(ModuleType) : 0;
    FString ModuleName = StationLabel.ToString();
    ModuleName.ReplaceInline(TEXT(" // UPGRADE"), TEXT(""));

    const FString Action = Level >= 3 ? TEXT("MAX") : TEXT("[F] UPGRADE");
    StatusText->SetText(FText::FromString(FString::Printf(
        TEXT("%s\nLEVEL %d / 3  //  %s"),
        *ModuleName,
        Level,
        *Action)));
}

bool ATU_HideoutUpgradeStation::UseStation(ATU_ArmedOperatorCharacter* Operator)
{
    if (!Operator || !IsOperatorInRange(Operator) || !GetWorld())
    {
        return false;
    }

    ATU_HideoutCommandCenter* Hideout = nullptr;
    for (TActorIterator<ATU_HideoutCommandCenter> It(GetWorld()); It; ++It)
    {
        Hideout = *It;
        break;
    }
    if (!Hideout)
    {
        return false;
    }

    UTUHideoutProgressionComponent* Progression = Hideout->GetHideoutProgression();
    if (!Progression || !Progression->UpgradeModule(ModuleType))
    {
        RefreshStatus();
        return false;
    }

    if (ATU_HideoutCommandCenterDecorator* Layer = Hideout->GetHideoutLayer())
    {
        Layer->RefreshFromProgression();
    }
    RefreshStatus();

    if (UGameInstance* GameInstance = Operator->GetGameInstance())
    {
        if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
        {
            Lifecycle->CaptureHideoutState(Progression);
            Lifecycle->CaptureOperatorLoadout(Operator);
            Lifecycle->SaveProfile();
        }
    }
    return true;
}
