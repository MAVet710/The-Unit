#include "TUHideoutLifecycleSubsystem.h"

#include "TUHideoutSaveGame.h"
#include "TUHideoutProgressionComponent.h"
#include "TUMeleeLoadoutComponent.h"
#include "TUMissionPackageData.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutComponent.h"
#include "TU_ModularOperatorCharacter.h"
#include "TU_ArmedOperatorCharacter.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static constexpr ETUEquipmentSlot AllGearSlots[] = {
        ETUEquipmentSlot::Headwear,
        ETUEquipmentSlot::Headset,
        ETUEquipmentSlot::Eyewear,
        ETUEquipmentSlot::Facewear,
        ETUEquipmentSlot::NVG,
        ETUEquipmentSlot::TorsoArmor,
        ETUEquipmentSlot::ChestRig,
        ETUEquipmentSlot::Backpack,
        ETUEquipmentSlot::Belt,
        ETUEquipmentSlot::LeftHip,
        ETUEquipmentSlot::RightHip,
        ETUEquipmentSlot::Gloves,
        ETUEquipmentSlot::KneePads,
        ETUEquipmentSlot::Footwear,
        ETUEquipmentSlot::Accessory
    };
}

void UTUHideoutLifecycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadProfile();
}

bool UTUHideoutLifecycleSubsystem::LoadProfile()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
    {
        Profile = Cast<UTUHideoutSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
    }

    if (!Profile)
    {
        Profile = Cast<UTUHideoutSaveGame>(UGameplayStatics::CreateSaveGameObject(UTUHideoutSaveGame::StaticClass()));
    }
    return Profile != nullptr;
}

bool UTUHideoutLifecycleSubsystem::SaveProfile()
{
    if (!Profile && !LoadProfile())
    {
        return false;
    }
    return UGameplayStatics::SaveGameToSlot(Profile, SaveSlotName, SaveUserIndex);
}

void UTUHideoutLifecycleSubsystem::ApplyHideoutState(UTUHideoutProgressionComponent* Progression) const
{
    if (!Progression || !Profile || Profile->HideoutModules.IsEmpty())
    {
        return;
    }

    for (const FTUHideoutModuleState& State : Profile->HideoutModules)
    {
        Progression->SetModuleLevel(State.Type, State.Level);
    }
}

void UTUHideoutLifecycleSubsystem::CaptureHideoutState(const UTUHideoutProgressionComponent* Progression)
{
    if (!Progression || !Profile)
    {
        return;
    }
    Profile->HideoutModules = Progression->GetModules();
}

void UTUHideoutLifecycleSubsystem::ApplyOperatorLoadout(ATU_ArmedOperatorCharacter* Operator) const
{
    if (!Operator || !Profile)
    {
        return;
    }

    if (!Profile->PrimaryId.IsNone())
    {
        Operator->SelectPrimaryById(Profile->PrimaryId);
    }
    if (!Profile->SecondaryId.IsNone())
    {
        Operator->SelectSecondaryById(Profile->SecondaryId);
    }
    if (!Profile->EquipmentId.IsNone())
    {
        Operator->SelectEquipmentById(Profile->EquipmentId);
    }
    if (!Profile->MeleeId.IsNone())
    {
        Operator->SelectMeleeById(Profile->MeleeId);
    }

    ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator);
    if (!Modular || Profile->GearBySlot.IsEmpty())
    {
        return;
    }

    if (UTUOperatorEquipmentComponent* Equipment = Modular->GetOperatorEquipment())
    {
        Equipment->ClearLoadout();
    }

    for (const ETUEquipmentSlot Slot : AllGearSlots)
    {
        if (const FName* SavedItemId = Profile->GearBySlot.Find(Slot))
        {
            if (!SavedItemId->IsNone())
            {
                Modular->EquipGearById(*SavedItemId);
            }
        }
    }
}

void UTUHideoutLifecycleSubsystem::CaptureOperatorLoadout(const ATU_ArmedOperatorCharacter* Operator)
{
    if (!Operator || !Profile)
    {
        return;
    }

    if (const UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout())
    {
        Profile->PrimaryId = Loadout->GetSelectedPrimaryId();
        Profile->SecondaryId = Loadout->GetSelectedSecondaryId();
        Profile->EquipmentId = Loadout->GetSelectedEquipmentId();
    }
    Profile->MeleeId = Operator->GetSelectedMeleeId();

    if (const ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator))
    {
        Profile->GearBySlot.Reset();
        for (const ETUEquipmentSlot Slot : AllGearSlots)
        {
            Profile->GearBySlot.Add(Slot, Modular->GetEquippedGearId(Slot));
        }
    }
}

bool UTUHideoutLifecycleSubsystem::DeployToMission(ATU_ArmedOperatorCharacter* Operator, const UTUMissionPackageData* MissionPackage)
{
    if (!Operator || !MissionPackage || MissionPackage->DestinationMap.IsNone() || !MissionPackage->Mission.bDeploymentAuthorized)
    {
        return false;
    }
    if (!Profile && !LoadProfile())
    {
        return false;
    }

    CaptureOperatorLoadout(Operator);
    Profile->ActiveMissionId = MissionPackage->Mission.MissionId;
    Profile->bMissionInProgress = true;
    SaveProfile();

    UGameplayStatics::OpenLevel(Operator, MissionPackage->DestinationMap);
    return true;
}

bool UTUHideoutLifecycleSubsystem::ReturnToHideout(bool bOperationCompleted)
{
    if (!Profile && !LoadProfile())
    {
        return false;
    }

    if (bOperationCompleted && Profile->bMissionInProgress)
    {
        ++Profile->CompletedOperations;
    }
    Profile->ActiveMissionId = NAME_None;
    Profile->bMissionInProgress = false;
    const FName ReturnMap = Profile->HideoutMapName.IsNone() ? FName(TEXT("CommandCenter")) : Profile->HideoutMapName;
    SaveProfile();

    if (!GetGameInstance())
    {
        return false;
    }
    UGameplayStatics::OpenLevel(GetGameInstance(), ReturnMap);
    return true;
}

bool UTUHideoutLifecycleSubsystem::IsMissionInProgress() const
{
    return Profile && Profile->bMissionInProgress;
}

FName UTUHideoutLifecycleSubsystem::GetActiveMissionId() const
{
    return Profile ? Profile->ActiveMissionId : NAME_None;
}

void UTUHideoutLifecycleSubsystem::SetHideoutMapName(FName MapName)
{
    if (!MapName.IsNone() && Profile)
    {
        Profile->HideoutMapName = MapName;
        SaveProfile();
    }
}
