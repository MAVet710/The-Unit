#include "TUArmoryWidget.h"

#include "TUEquipmentDefinition.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_ModularOperatorCharacter.h"
#include "TUMeleeLoadoutComponent.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutComponent.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    FString GearSlotLabel(ETUEquipmentSlot Slot)
    {
        switch (Slot)
        {
            case ETUEquipmentSlot::Headwear: return TEXT("HEADWEAR");
            case ETUEquipmentSlot::Headset: return TEXT("HEADSET");
            case ETUEquipmentSlot::Eyewear: return TEXT("EYEWEAR");
            case ETUEquipmentSlot::Facewear: return TEXT("FACEWEAR");
            case ETUEquipmentSlot::NVG: return TEXT("NVG");
            case ETUEquipmentSlot::TorsoArmor: return TEXT("TORSO ARMOR");
            case ETUEquipmentSlot::ChestRig: return TEXT("CHEST RIG");
            case ETUEquipmentSlot::Backpack: return TEXT("BACKPACK");
            case ETUEquipmentSlot::Belt: return TEXT("BATTLE BELT");
            case ETUEquipmentSlot::LeftHip: return TEXT("LEFT HIP");
            case ETUEquipmentSlot::RightHip: return TEXT("RIGHT HIP");
            case ETUEquipmentSlot::Gloves: return TEXT("GLOVES");
            case ETUEquipmentSlot::KneePads: return TEXT("KNEE PADS");
            case ETUEquipmentSlot::Footwear: return TEXT("FOOTWEAR");
            default: return TEXT("ACCESSORY");
        }
    }
}

TSharedRef<SWidget> UTUArmoryWidget::RebuildWidget()
{
    TSharedRef<SWidget> Widget =
        SNew(SBorder)
        .Padding(16.0f)
        [
            SNew(SBox)
            .WidthOverride(680.0f)
            .MaxDesiredHeight(820.0f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SAssignNew(RootBox, SVerticalBox)
                ]
            ]
        ];

    RebuildContent();
    return Widget;
}

void UTUArmoryWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RootBox.Reset();
}

void UTUArmoryWidget::SetOperator(ATU_ArmedOperatorCharacter* InOperator)
{
    Operator = InOperator;
    Refresh();
}

void UTUArmoryWidget::SetViewMode(ETUArmoryViewMode NewViewMode)
{
    ViewMode = NewViewMode;
    Refresh();
}

void UTUArmoryWidget::Refresh()
{
    RebuildContent();
}

void UTUArmoryWidget::PersistSelections()
{
    if (!Operator.IsValid())
    {
        return;
    }

    if (UGameInstance* GameInstance = Operator->GetGameInstance())
    {
        if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
        {
            Lifecycle->CaptureOperatorLoadout(Operator.Get());
            Lifecycle->SaveProfile();
        }
    }
}

void UTUArmoryWidget::RebuildContent()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    RootBox->ClearChildren();

    FString Title = TEXT("THE UNIT // ARMORY");
    FString Subtitle = TEXT("Configure operator loadout. Changes apply and save immediately at this command-center station.");
    if (ViewMode == ETUArmoryViewMode::Weapons)
    {
        Title = TEXT("THE UNIT // ARMORY & WEAPON BENCH");
        Subtitle = TEXT("Select primary and secondary weapons. Attachment bench integration uses the same weapon loadout data.");
    }
    else if (ViewMode == ETUArmoryViewMode::Gear)
    {
        Title = TEXT("THE UNIT // CAGE");
        Subtitle = TEXT("Configure melee, mission equipment and modular operator gear. Tactical slot changes apply and save immediately.");
    }

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 8.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Title))
    ];

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 12.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Subtitle))
    ];

    if (!Operator.IsValid())
    {
        RootBox->AddSlot().AutoHeight().Padding(4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("No operator is assigned to this station.")))
        ];
        return;
    }

    if (ViewMode == ETUArmoryViewMode::Full || ViewMode == ETUArmoryViewMode::Weapons)
    {
        AddPrimaryChoices();
        AddSecondaryChoices();
    }

    if (ViewMode == ETUArmoryViewMode::Full || ViewMode == ETUArmoryViewMode::Gear)
    {
        AddMeleeChoices();
        AddEquipmentChoices();
        AddTacticalGearChoices();
    }

    AddSummary();
    AddStashManifest();

    TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 16.0f, 4.0f, 4.0f)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("LEAVE STATION")))
        .OnClicked_Lambda([WeakThis]()
        {
            if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
            {
                WeakThis->PersistSelections();
                WeakThis->Operator->CloseArmory();
            }
            return FReply::Handled();
        })
    ];
}

void UTUArmoryWidget::AddSectionHeader(const FString& Label)
{
    if (!RootBox.IsValid())
    {
        return;
    }

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 12.0f, 4.0f, 4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Label))
    ];
}

void UTUArmoryWidget::AddPrimaryChoices()
{
    AddSectionHeader(TEXT("PRIMARY"));
    UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout();
    if (!Loadout)
    {
        return;
    }

    const FName Selected = Loadout->GetSelectedPrimaryId();
    for (const FTUOperatorWeaponEntry& Entry : Loadout->GetPrimaryItems())
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"), bSelected ? TEXT("[SELECTED] ") : TEXT(""), *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    if (WeakThis->Operator->SelectPrimaryById(ItemId))
                    {
                        WeakThis->PersistSelections();
                    }
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddSecondaryChoices()
{
    AddSectionHeader(TEXT("SECONDARY"));
    UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout();
    if (!Loadout)
    {
        return;
    }

    const FName Selected = Loadout->GetSelectedSecondaryId();
    for (const FTUOperatorWeaponEntry& Entry : Loadout->GetSecondaryItems())
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"), bSelected ? TEXT("[SELECTED] ") : TEXT(""), *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    if (WeakThis->Operator->SelectSecondaryById(ItemId))
                    {
                        WeakThis->PersistSelections();
                    }
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddMeleeChoices()
{
    AddSectionHeader(TEXT("MELEE"));
    UTUMeleeLoadoutComponent* Loadout = Operator->GetMeleeLoadout();
    if (!Loadout)
    {
        return;
    }

    const FName Selected = Loadout->GetSelectedItemId();
    for (const FTUMeleeEquipmentEntry& Entry : Loadout->GetAvailableItems())
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"), bSelected ? TEXT("[SELECTED] ") : TEXT(""), *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    if (WeakThis->Operator->SelectMeleeById(ItemId))
                    {
                        WeakThis->PersistSelections();
                    }
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddEquipmentChoices()
{
    AddSectionHeader(TEXT("MISSION EQUIPMENT"));
    UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout();
    if (!Loadout)
    {
        return;
    }

    const FName Selected = Loadout->GetSelectedEquipmentId();
    for (const FTUOperatorEquipmentEntry& Entry : Loadout->GetEquipmentItems())
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"), bSelected ? TEXT("[SELECTED] ") : TEXT(""), *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    if (WeakThis->Operator->SelectEquipmentById(ItemId))
                    {
                        WeakThis->PersistSelections();
                    }
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddTacticalGearChoices()
{
    AddSectionHeader(TEXT("TACTICAL GEAR / UNIFORM SYSTEM"));

    ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator.Get());
    if (!Modular)
    {
        RootBox->AddSlot().AutoHeight().Padding(4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("Current operator pawn does not expose the modular equipment system.")))
        ];
        return;
    }

    const TArray<UTUEquipmentDefinition*> Gear = Modular->GetAvailableGear();
    if (Gear.IsEmpty())
    {
        RootBox->AddSlot().AutoHeight().Padding(4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("No authored gear definitions are available yet. Import the SWAT/helmet assets and assign equipment definitions to the operator catalog.")))
        ];
        return;
    }

    for (UTUEquipmentDefinition* Definition : Gear)
    {
        if (!Definition)
        {
            continue;
        }

        const FName ItemId = Definition->ItemId;
        const ETUEquipmentSlot EquipmentSlot = Definition->Slot;
        const bool bSelected = Modular->GetEquippedGearId(EquipmentSlot) == ItemId;
        const FString Label = FString::Printf(
            TEXT("%s%s // %s  |  %.2f kg%s"),
            bSelected ? TEXT("[EQUIPPED] ") : TEXT(""),
            *GearSlotLabel(EquipmentSlot),
            *Definition->DisplayName.ToString(),
            Definition->WeightKg,
            Definition->bProvidesBallisticProtection ? TEXT("  |  PROTECTIVE") : TEXT(""));

        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([WeakThis, ItemId, EquipmentSlot, bSelected]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    if (ATU_ModularOperatorCharacter* CurrentModular = Cast<ATU_ModularOperatorCharacter>(WeakThis->Operator.Get()))
                    {
                        const bool bChanged = bSelected
                            ? CurrentModular->UnequipGearSlot(EquipmentSlot)
                            : CurrentModular->EquipGearById(ItemId);
                        if (bChanged)
                        {
                            WeakThis->PersistSelections();
                        }
                    }
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddStashManifest()
{
    AddSectionHeader(TEXT("PERSISTENT STASH / RAID RISK"));
    if (!Operator.IsValid() || !Operator->GetGameInstance()) return;
    UTUHideoutLifecycleSubsystem* Life = Operator->GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>();
    if (!Life) return;
    const FGuid PlayerId = Life->GetLocalPlayerId();
    const FTUItemLedger Stash = Life->GetPlayerStash(PlayerId);
    const FTUItemLedger Carried = Operator->ExportItemLedger();
    const FString Header = FString::Printf(
        TEXT("HQ SAFE: %d weapons | %d magazines | %d items | %d loose rounds    RAID KIT: %d weapons | %d magazines | %d items | %d loose rounds"),
        Stash.Weapons.Num(), Stash.Magazines.Num(), Stash.Items.Num(), Stash.LooseCartridges.Num(),
        Carried.Weapons.Num(), Carried.Magazines.Num(), Carried.Items.Num(), Carried.LooseCartridges.Num());
    RootBox->AddSlot().AutoHeight().Padding(4.0f,4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Header))
    ];
    RootBox->AddSlot().AutoHeight().Padding(4.0f,2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("Only the RAID KIT above is escrowed on deployment. Unselected HQ inventory remains safe.")))
    ];
    for (const FWeaponInstanceState& Weapon : Stash.Weapons)
    {
        const bool bCarried = Carried.Weapons.ContainsByPredicate([&](const FWeaponInstanceState& W){ return W.InstanceId == Weapon.InstanceId; });
        const FString Row = FString::Printf(TEXT("%s %s | %s | chamber %s"),
            bCarried ? TEXT("[RAID]") : TEXT("[HQ]"),
            *Weapon.DefinitionId.ToString(), *Weapon.InstanceId.ToString(EGuidFormats::Short),
            Weapon.ChamberAmmoId.IsNone() ? TEXT("empty") : *Weapon.ChamberAmmoId.ToString());
        RootBox->AddSlot().AutoHeight().Padding(12.0f,1.0f)[SNew(STextBlock).Text(FText::FromString(Row))];
    }
    for (const FTUMagazineInstance& Magazine : Stash.Magazines)
    {
        const bool bCarried = Carried.Magazines.ContainsByPredicate([&](const FTUMagazineInstance& M){ return M.InstanceId == Magazine.InstanceId; });
        const FString Row = FString::Printf(TEXT("%s MAG %s | %d rounds"),
            bCarried ? TEXT("[RAID]") : TEXT("[HQ]"), *Magazine.InstanceId.ToString(EGuidFormats::Short), Magazine.Cartridges.Num());
        RootBox->AddSlot().AutoHeight().Padding(12.0f,1.0f)[SNew(STextBlock).Text(FText::FromString(Row))];
    }
}
void UTUArmoryWidget::AddSummary()
{
    AddSectionHeader(TEXT("LOADOUT SUMMARY"));
    const FString ActiveSlot = Operator->GetActiveWeaponSlot() == ETUOperatorWeaponSlot::Primary ? TEXT("PRIMARY") : TEXT("SECONDARY");

    float CarriedWeight = Operator->GetSelectedLoadoutWeightKg();
    int32 GearCount = 0;
    if (ATU_ModularOperatorCharacter* Modular = Cast<ATU_ModularOperatorCharacter>(Operator.Get()))
    {
        CarriedWeight = Modular->GetTotalCombatLoadoutWeightKg();
        if (UTUOperatorEquipmentComponent* Equipment = Modular->GetOperatorEquipment())
        {
            GearCount = Equipment->GetEquippedItemCount();
        }
    }

    const FString Summary = FString::Printf(
        TEXT("Active weapon: %s   |   Total carried weight: %.2f kg   |   Equipped tactical items: %d"),
        *ActiveSlot,
        CarriedWeight,
        GearCount);

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Summary))
    ];
}
