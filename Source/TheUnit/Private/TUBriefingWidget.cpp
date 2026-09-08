#include "TUBriefingWidget.h"

#include "TU_ArmedOperatorCharacter.h"
#include "TU_PlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
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

UTUMX50TabletComponent* UTUBriefingWidget::GetTabletState() const
{
    if (!Operator.IsValid())
    {
        return nullptr;
    }

    if (ATU_PlayerController* PC = Cast<ATU_PlayerController>(Operator->GetController()))
    {
        return PC->GetMX50Tablet();
    }
    return nullptr;
}

void UTUBriefingWidget::Configure(ATU_ArmedOperatorCharacter* InOperator, FName InMissionId, const FText& InMissionTitle)
{
    Operator = InOperator;

    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        Tablet->SetMissionContext(InMissionId, InMissionTitle);
        MissionSnapshot = Tablet->GetMissionSnapshot();
        ActivePage = Tablet->GetActivePage();
        Refresh();
        return;
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
    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        Tablet->SetMissionSnapshot(InSnapshot);
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
    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        Tablet->SetActivePage(Page);
    }
    Refresh();
}

void UTUBriefingWidget::SelectMapMarker(FName MarkerId)
{
    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        Tablet->SelectMapMarker(MarkerId);
    }
    Refresh();
}

void UTUBriefingWidget::SelectVideoFeed(FName FeedId)
{
    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        Tablet->SelectVideoFeed(FeedId);
        MissionSnapshot = Tablet->GetMissionSnapshot();
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

FText UTUBriefingWidget::GetMarkerTypeLabel(ETUMX50MapMarkerType Type) const
{
    switch (Type)
    {
        case ETUMX50MapMarkerType::Entry: return FText::FromString(TEXT("ENTRY"));
        case ETUMX50MapMarkerType::Objective: return FText::FromString(TEXT("OBJECTIVE"));
        case ETUMX50MapMarkerType::Threat: return FText::FromString(TEXT("THREAT"));
        case ETUMX50MapMarkerType::Rally: return FText::FromString(TEXT("RALLY"));
        case ETUMX50MapMarkerType::Friendly: return FText::FromString(TEXT("FRIENDLY"));
        case ETUMX50MapMarkerType::Extraction: return FText::FromString(TEXT("EXTRACTION"));
        case ETUMX50MapMarkerType::Observation: return FText::FromString(TEXT("OBSERVATION"));
        default: return FText::FromString(TEXT("MARKER"));
    }
}

FText UTUBriefingWidget::GetVideoFeedTypeLabel(ETUMX50VideoFeedType Type) const
{
    switch (Type)
    {
        case ETUMX50VideoFeedType::FPVDrone: return FText::FromString(TEXT("FPV DRONE"));
        case ETUMX50VideoFeedType::BodyCamera: return FText::FromString(TEXT("BODY CAMERA"));
        case ETUMX50VideoFeedType::FixedCamera: return FText::FromString(TEXT("FIXED CAMERA"));
        case ETUMX50VideoFeedType::Observation: return FText::FromString(TEXT("OBSERVATION"));
        default: return FText::FromString(TEXT("VIDEO"));
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

void UTUBriefingWidget::AddMapPage()
{
    UTUMX50TabletComponent* Tablet = GetTabletState();
    if (!Tablet || !RootBox.IsValid())
    {
        return;
    }

    const TArray<FTMX50MapMarker> Markers = Tablet->GetMapMarkers();
    const TArray<FVector2D> Route = Tablet->GetPlannedRoute();

    RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
    [
        SNew(STextBlock).Text(FText::FromString(FString::Printf(
            TEXT("AO: %s // %d markers // %d route points"),
            *MissionSnapshot.Area.ToString(), Markers.Num(), Route.Num())))
    ];

    if (Markers.Num() == 0)
    {
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 4.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("NO MISSION MAP OVERLAYS LOADED")))
        ];
        return;
    }

    constexpr int32 Columns = 12;
    constexpr int32 Rows = 8;
    TSharedPtr<SGridPanel> Grid;
    RootBox->AddSlot().AutoHeight().Padding(12.0f, 8.0f)
    [
        SNew(SBox)
        .WidthOverride(900.0f)
        .HeightOverride(360.0f)
        [
            SAssignNew(Grid, SGridPanel)
        ]
    ];

    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Column = 0; Column < Columns; ++Column)
        {
            Grid->AddSlot(Column, Row).Padding(1.0f)
            [
                SNew(SBorder)
                .Padding(2.0f)
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("·")))
                ]
            ];
        }
    }

    TWeakObjectPtr<UTUBriefingWidget> WeakThis(this);
    for (const FTMX50MapMarker& Marker : Markers)
    {
        if (!Marker.bVisible || Marker.MarkerId.IsNone())
        {
            continue;
        }

        const int32 Column = FMath::Clamp(FMath::FloorToInt(Marker.NormalizedPosition.X * Columns), 0, Columns - 1);
        const int32 Row = FMath::Clamp(FMath::FloorToInt(Marker.NormalizedPosition.Y * Rows), 0, Rows - 1);
        const FName MarkerId = Marker.MarkerId;
        const FString ButtonText = FString::Printf(TEXT("%s\n%s"), *GetMarkerTypeLabel(Marker.Type).ToString(), *Marker.Label.ToString());

        Grid->AddSlot(Column, Row).Padding(1.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(ButtonText))
            .OnClicked_Lambda([WeakThis, MarkerId]()
            {
                if (WeakThis.IsValid())
                {
                    WeakThis->SelectMapMarker(MarkerId);
                }
                return FReply::Handled();
            })
        ];
    }

    FTMX50MapMarker Selected;
    if (Tablet->GetSelectedMapMarker(Selected))
    {
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 8.0f, 12.0f, 2.0f)
        [
            SNew(STextBlock).Text(FText::FromString(FString::Printf(
                TEXT("SELECTED: %s // %s // FLOOR %d // (%.2f, %.2f)"),
                *GetMarkerTypeLabel(Selected.Type).ToString(),
                *Selected.Label.ToString(),
                Selected.Floor,
                Selected.NormalizedPosition.X,
                Selected.NormalizedPosition.Y)))
        ];
        if (!Selected.Details.IsEmpty())
        {
            RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
            [
                SNew(STextBlock).Text(Selected.Details)
            ];
        }
    }
}

void UTUBriefingWidget::AddVideoPage()
{
    UTUMX50TabletComponent* Tablet = GetTabletState();
    if (!Tablet || !RootBox.IsValid())
    {
        return;
    }

    const TArray<FTMX50VideoFeed> Feeds = Tablet->GetVideoFeeds();
    if (Feeds.Num() == 0)
    {
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 3.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("NO REGISTERED VIDEO SOURCES")))
        ];
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 3.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("FPV, bodycam and fixed-camera providers register here when available.")))
        ];
        return;
    }

    TWeakObjectPtr<UTUBriefingWidget> WeakThis(this);
    for (const FTMX50VideoFeed& Feed : Feeds)
    {
        const FName FeedId = Feed.FeedId;
        const FString Text = FString::Printf(
            TEXT("%s // %s // %s"),
            *GetVideoFeedTypeLabel(Feed.Type).ToString(),
            *Feed.DisplayName.ToString(),
            Feed.bAvailable ? TEXT("AVAILABLE") : TEXT("OFFLINE"));

        RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
        [
            SNew(SButton)
            .Text(FText::FromString(Text))
            .IsEnabled(Feed.bAvailable)
            .OnClicked_Lambda([WeakThis, FeedId]()
            {
                if (WeakThis.IsValid())
                {
                    WeakThis->SelectVideoFeed(FeedId);
                }
                return FReply::Handled();
            })
        ];
    }

    FTMX50VideoFeed Selected;
    if (Tablet->GetSelectedVideoFeed(Selected))
    {
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 8.0f, 12.0f, 2.0f)
        [
            SNew(STextBlock).Text(FText::FromString(FString::Printf(
                TEXT("SELECTED FEED: %s // %s"),
                *Selected.DisplayName.ToString(),
                *Selected.StatusText.ToString())))
        ];
        RootBox->AddSlot().AutoHeight().Padding(12.0f, 2.0f)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("Live render-target viewport connects here when the selected provider supplies video.")))
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
            AddMapPage();
            break;

        case ETUMX50Page::Intel:
            AddLine(FText::FromString(FString::Printf(TEXT("Threat assessment: %s"), *MissionSnapshot.ThreatSummary.ToString())));
            AddLine(FText::FromString(TEXT("Intel attachments remain a mission-data integration boundary: imagery, documents, target photos, ROE notes and evidence.")));
            break;

        case ETUMX50Page::Team:
            AddLine(MissionSnapshot.TeamSummary);
            AddLine(FText::FromString(TEXT("Co-op integration boundary: callsigns, roles, readiness, health/status and leader launch authority.")));
            break;

        case ETUMX50Page::Drone:
            AddVideoPage();
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

    if (UTUMX50TabletComponent* Tablet = GetTabletState())
    {
        MissionSnapshot = Tablet->GetMissionSnapshot();
        ActivePage = Tablet->GetActivePage();
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
