#include "TUBriefingWidget.h"

#include "TU_ArmedOperatorCharacter.h"
#include "TU_PlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UTUBriefingWidget::RebuildWidget()
{
    TSharedRef<SWidget> Widget =
        SNew(SBorder)
        .Padding(18.0f)
        [
            SNew(SBox)
            .WidthOverride(980.0f)
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

    if (Operator.IsValid())
    {
        if (ATU_PlayerController* PC = Cast<ATU_PlayerController>(Operator->GetController()))
        {
            if (UTUMX50TabletComponent* Tablet = PC->GetMX50Tablet())
            {
                Tablet->SetMissionContext(InMissionId, InMissionTitle);
                MissionSnapshot = Tablet->GetMissionSnapshot();
                ActivePage = Tablet->GetActivePage();
                Refresh();
                return;
            }
        }
    }

    if (!InMissionId.IsNone())
    {
        MissionSnapshot.MissionId = InMissionId;
    }
    if (!InMissionTitle.IsEmpty())
    {
        MissionSnapshot.MissionTitle = InMissionTitle;
    }
    Refresh();
}

void UTUBriefingWidget::SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot)
{
    MissionSnapshot = InSnapshot;

    if (Operator.IsValid())
    {
        if (ATU_PlayerController* PC = Cast<ATU_PlayerController>(Operator->GetController()))
        {
            if (UTUMX50TabletComponent* Tablet = PC->GetMX50Tablet())
            {
                Tablet->SetMissionSnapshot(InSnapshot);
            }
        }
    }

    Refresh();
}

void UTUBriefingWidget::Refresh()
{
    RebuildContent();
}

void UTUBriefingWidget::SetPage(ETUMX50Page Page)
{
    if (ActivePage == Page)
    {
        return;
    }

    ActivePage = Page;

    if (Operator.IsValid())
    {
        if (ATU_PlayerController* PC = Cast<ATU_PlayerController>(Operator->GetController()))
        {
            if (UTUMX50TabletComponent* Tablet = PC->GetMX50Tablet())
            {
                Tablet->SetActivePage(Page);
            }
        }
    }

    Refresh();
}

FText UTUBriefingWidget::GetPageLabel(ETUMX50Page Page) const
{
    switch (Page)
    {
        case ETUMX50Page::Mission: return FText::FromString(TEXT("MISSION"));
        case ETUMX50Page::Map: return FText::FromString(TEXT("MAP"));
        case ETUMX50Page::Intel: return FText::FromString(TEXT("INTEL"));
        case ETUMX50Page::Team: return FText::FromString(TEXT("TEAM"));
        case ETUMX50Page::Drone: return FText::FromString(TEXT("DRONE / VIDEO"));
        case ETUMX50Page::Loadout: return FText::FromString(TEXT("LOADOUT"));
        default: return FText::FromString(TEXT("MX50"));
    }
}

void UTUBriefingWidget::AddNavigation()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    TWeakObjectPtr<UTUBriefingWidget> WeakThis(this);
    TSharedPtr<SHorizontalBox> Navigation;
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 10.0f)
    [
        SAssignNew(Navigation, SHorizontalBox)
    ];

    const ETUMX50Page Pages[] = {
        ETUMX50Page::Mission,
        ETUMX50Page::Map,
        ETUMX50Page::Intel,
        ETUMX50Page::Team,
        ETUMX50Page::Drone,
        ETUMX50Page::Loadout
    };

    for (ETUMX50Page Page : Pages)
    {
        Navigation->AddSlot().AutoWidth().Padding(2.0f)
        [
            SNew(SButton)
            .Text(GetPageLabel(Page))
            .OnClicked_Lambda([WeakThis, Page]()
            {
                if (WeakThis.IsValid())
                {
                    WeakThis->SetPage(Page);
                }
                return FReply::Handled();
            })
        ];
    }
}

void UTUBriefingWidget::AddCurrentPageContent()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    auto AddLine = [this](const FText& Text, float LeftPadding = 12.0f)
    {
        RootBox->AddSlot().AutoHeight().Padding(LeftPadding, 3.0f)
        [
            SNew(STextBlock).Text(Text)
        ];
    };

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 8.0f)
    [
        SNew(STextBlock).Text(GetPageLabel(ActivePage))
    ];

    switch (ActivePage)
    {
        case ETUMX50Page::Mission:
            AddLine(FText::FromString(FString::Printf(TEXT("Operation: %s"), *MissionSnapshot.MissionTitle.ToString())));
            AddLine(FText::FromString(FString::Printf(TEXT("Mission ID: %s"), *MissionSnapshot.MissionId.ToString())));
            AddLine(FText::FromString(FString::Printf(TEXT("Area: %s"), *MissionSnapshot.Area.ToString())));
            AddLine(FText::FromString(FString::Printf(TEXT("Objective: %s"), *MissionSnapshot.Objective.ToString())));
            AddLine(FText::FromString(MissionSnapshot.bDeploymentAuthorized
                ? TEXT("Deployment status: AUTHORIZED")
                : TEXT("Deployment status: NOT YET AUTHORIZED")));
            break;

        case ETUMX50Page::Map:
            AddLine(FText::FromString(FString::Printf(TEXT("AO: %s"), *MissionSnapshot.Area.ToString())));
            AddLine(FText::FromString(TEXT("Tactical map layer reserved for mission geometry / floor plans.")));
            AddLine(FText::FromString(TEXT("Planned overlays: entry points, objectives, rally points, known threats and squad marks.")));
            AddLine(FText::FromString(TEXT("No fabricated map data is shown until the mission service supplies it.")));
            break;

        case ETUMX50Page::Intel:
            AddLine(FText::FromString(FString::Printf(TEXT("Threat assessment: %s"), *MissionSnapshot.ThreatSummary.ToString())));
            AddLine(FText::FromString(TEXT("Future intel package: imagery, documents, target photos, ROE notes and collected evidence.")));
            break;

        case ETUMX50Page::Team:
            AddLine(MissionSnapshot.TeamSummary);
            AddLine(FText::FromString(TEXT("Future co-op layer: callsigns, roles, readiness, health/status and leader launch authority.")));
            break;

        case ETUMX50Page::Drone:
            AddLine(FText::FromString(MissionSnapshot.bDroneFeedAvailable
                ? TEXT("FPV / VIDEO LINK: AVAILABLE")
                : TEXT("FPV / VIDEO LINK: NO ACTIVE FEED")));
            AddLine(FText::FromString(TEXT("This page is the integration boundary for FPV drone video, body cameras and remote observation feeds.")));
            AddLine(FText::FromString(TEXT("The current tablet branch does not depend on the separate FPV PR.")));
            break;

        case ETUMX50Page::Loadout:
            if (Operator.IsValid())
            {
                AddLine(FText::FromString(FString::Printf(TEXT("Selected carried loadout: %.2f kg"), Operator->GetSelectedLoadoutWeightKg())));
                AddLine(FText::FromString(FString::Printf(TEXT("Melee: %s"), *Operator->GetSelectedMeleeId().ToString())));
                AddLine(FText::FromString(Operator->GetActiveWeaponSlot() == ETUOperatorWeaponSlot::Primary
                    ? TEXT("Active weapon slot: PRIMARY")
                    : TEXT("Active weapon slot: SECONDARY")));
            }
            else
            {
                AddLine(FText::FromString(TEXT("Operator loadout unavailable.")));
            }
            AddLine(FText::FromString(TEXT("Armor, helmet, carrier, belt, pack and gadget status connect here when the modular equipment branch lands.")));
            break;
    }
}

void UTUBriefingWidget::RebuildContent()
{
    if (!RootBox.IsValid())
    {
        return;
    }

    RootBox->ClearChildren();
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 6.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("MX50 TACTICAL TABLET // THE UNIT")))
    ];
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 4.0f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("CHEST-RIG MISSION / INTEL SYSTEM")))
    ];

    AddNavigation();
    AddCurrentPageContent();

    TWeakObjectPtr<UTUBriefingWidget> WeakThis(this);
    RootBox->AddSlot().AutoHeight().Padding(4.0f, 18.0f, 4.0f, 4.0f)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("STOW MX50")))
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
