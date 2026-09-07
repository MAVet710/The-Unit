#include "TUBriefingWidget.h"

#include "TU_ArmedOperatorCharacter.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UTUBriefingWidget::RebuildWidget()
{
    TSharedRef<SWidget> Widget =
        SNew(SBorder)
        .Padding(20.0f)
        [
            SNew(SBox)
            .WidthOverride(760.0f)
            [
                SAssignNew(RootBox, SVerticalBox)
            ]
        ];

    RebuildContent();
    return Widget;
}

void UTUBriefingWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RootBox.Reset();
}

void UTUBriefingWidget::Configure(ATU_ArmedOperatorCharacter* InOperator, FName InMissionId, const FText& InMissionTitle)
{
    Operator = InOperator;
    MissionId = InMissionId.IsNone() ? FName(TEXT("OP_KILLHOUSE")) : InMissionId;
    MissionTitle = InMissionTitle.IsEmpty() ? FText::FromString(TEXT("Operation Briefing")) : InMissionTitle;
    Refresh();
}

void UTUBriefingWidget::Refresh()
{
    RebuildContent();
}

void UTUBriefingWidget::RebuildContent()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    RootBox->ClearChildren();
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 8.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("MX50 TACTICAL TABLET // THE UNIT")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("SECURE OPERATIONS // MISSION PACKAGE")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 8.0f, 4.0f, 2.0f)
    [
        SNew(STextBlock).Text(MissionTitle)
    ];
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 2.0f, 4.0f, 12.0f)
    [
        SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Mission ID: %s"), *MissionId.ToString())))
    ];

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("MISSION INTEL")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("• Review mission objective, entry plan and known threats")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("• Review map, route, imagery and team assignments")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("• Confirm primary / secondary / melee / equipment loadout")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("• Test-fire and configure weapons before deployment")))
    ];

    if (Operator.IsValid())
    {
        RootBox->AddSlot().AutoHeight().Padding(4.0f, 14.0f, 4.0f, 4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Selected carried loadout: %.2f kg"), Operator->GetSelectedLoadoutWeightKg())))
        ];
    }

    TWeakObjectPtr<UTUBriefingWidget> WeakThis(this);
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 18.0f, 4.0f, 4.0f)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("CLOSE MX50")))
        .OnClicked_Lambda([WeakThis]()
        {
            if (WeakThis.IsValid() && WeakThis->Operator.IsValid())
            {
                WeakThis->Operator->CloseBriefing();
            }
            return FReply::Handled();
        })
    ];
}
