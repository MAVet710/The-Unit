#include "TUMX50TabletComponent.h"

UTUMX50TabletComponent::UTUMX50TabletComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTUMX50TabletComponent::SetMissionContext(FName MissionId, const FText& MissionTitle)
{
    if (!MissionId.IsNone())
    {
        MissionSnapshot.MissionId = MissionId;
    }
    if (!MissionTitle.IsEmpty())
    {
        MissionSnapshot.MissionTitle = MissionTitle;
    }
}

void UTUMX50TabletComponent::SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot)
{
    MissionSnapshot = InSnapshot;
}

ETUMX50Page UTUMX50TabletComponent::CyclePage(int32 Direction)
{
    constexpr int32 PageCount = static_cast<int32>(ETUMX50Page::Loadout) + 1;
    int32 Index = static_cast<int32>(ActivePage);
    const int32 Step = Direction < 0 ? -1 : 1;
    Index = (Index + Step + PageCount) % PageCount;
    ActivePage = static_cast<ETUMX50Page>(Index);
    return ActivePage;
}
