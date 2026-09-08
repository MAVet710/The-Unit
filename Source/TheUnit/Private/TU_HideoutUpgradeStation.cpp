#include "TU_HideoutUpgradeStation.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutCommandCenter.h"
#include "TU_HideoutCommandCenterDecorator.h"
#include "TU_ArmedOperatorCharacter.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

ATU_HideoutUpgradeStation::ATU_HideoutUpgradeStation()
{
    StationLabel = FText::FromString(TEXT("HIDEOUT UPGRADE"));
}

void ATU_HideoutUpgradeStation::ConfigureUpgradeStation(ETUHideoutModuleType InModuleType, const FText& InLabel)
{
    ModuleType = InModuleType;
    StationLabel = InLabel;
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
        return false;
    }

    if (ATU_HideoutCommandCenterDecorator* Layer = Hideout->GetHideoutLayer())
    {
        Layer->RefreshFromProgression();
    }

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
