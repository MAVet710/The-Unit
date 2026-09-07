#include "TUArmoryWidget.h"

#include "TU_ArmedOperatorCharacter.h"
#include "TUMeleeLoadoutComponent.h"
#include "TUOperatorLoadoutComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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

void UTUArmoryWidget::Refresh()
{
    RebuildContent();
}

void UTUArmoryWidget::RebuildContent()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    RootBox->ClearChildren();

    RootBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 8.0f)
    [
        SNew(STextBlock)
        .Text(FText::FromString(TEXT("THE UNIT // ARMORY")))
    ];

    RootBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 0.0f, 4.0f, 12.0f)
    [
        SNew(STextBlock)
        .Text(FText::FromString(TEXT("Configure operator loadout. Changes apply immediately while the armory is open.")))
    ];

    if (!Operator.IsValid())
    {
        RootBox->AddSlot().AutoHeight().Padding(4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("No operator is assigned to this armory.")))
        ];
        return;
    }

    AddPrimaryChoices();
    AddSecondaryChoices();
    AddMeleeChoices();
    AddEquipmentChoices();
    AddSummary();

    TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);
    RootBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 16.0f, 4.0f, 4.0f)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("CLOSE ARMORY")))
        .OnClicked_Lambda([WeakThis]()
        {
            if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
            {
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

    RootBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 12.0f, 4.0f, 4.0f)
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
    const TArray<FTUOperatorWeaponEntry> Items = Loadout->GetPrimaryItems();
    for (const FTUOperatorWeaponEntry& Entry : Items)
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"),
            bSelected ? TEXT("[SELECTED] ") : TEXT(""),
            *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);

        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(Label))
            .OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    WeakThis->Operator->SelectPrimaryById(ItemId);
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
    const TArray<FTUOperatorWeaponEntry> Items = Loadout->GetSecondaryItems();
    for (const FTUOperatorWeaponEntry& Entry : Items)
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"),
            bSelected ? TEXT("[SELECTED] ") : TEXT(""),
            *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);

        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(Label))
            .OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    WeakThis->Operator->SelectSecondaryById(ItemId);
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
    const TArray<FTUMeleeEquipmentEntry> Items = Loadout->GetAvailableItems();
    for (const FTUMeleeEquipmentEntry& Entry : Items)
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"),
            bSelected ? TEXT("[SELECTED] ") : TEXT(""),
            *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);

        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(Label))
            .OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    WeakThis->Operator->SelectMeleeById(ItemId);
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddEquipmentChoices()
{
    AddSectionHeader(TEXT("EQUIPMENT"));
    UTUOperatorLoadoutComponent* Loadout = Operator->GetOperatorLoadout();
    if (!Loadout)
    {
        return;
    }

    const FName Selected = Loadout->GetSelectedEquipmentId();
    const TArray<FTUOperatorEquipmentEntry> Items = Loadout->GetEquipmentItems();
    for (const FTUOperatorEquipmentEntry& Entry : Items)
    {
        const bool bSelected = Entry.ItemId == Selected;
        const FString Label = FString::Printf(TEXT("%s%s  |  %.2f kg"),
            bSelected ? TEXT("[SELECTED] ") : TEXT(""),
            *Entry.DisplayName.ToString(), Entry.WeightKg);
        const FName ItemId = Entry.ItemId;
        TWeakObjectPtr<UTUArmoryWidget> WeakThis(this);

        RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(Label))
            .OnClicked_Lambda([WeakThis, ItemId]()
            {
                if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
                {
                    WeakThis->Operator->SelectEquipmentById(ItemId);
                    WeakThis->Refresh();
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUArmoryWidget::AddSummary()
{
    AddSectionHeader(TEXT("LOADOUT SUMMARY"));

    const FString ActiveSlot = Operator->GetActiveWeaponSlot() == ETUOperatorWeaponSlot::Primary
        ? TEXT("PRIMARY") : TEXT("SECONDARY");
    const FString Summary = FString::Printf(TEXT("Active weapon: %s   |   Selected carried weight: %.2f kg"),
        *ActiveSlot, Operator->GetSelectedLoadoutWeightKg());

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(Summary))
    ];
}
