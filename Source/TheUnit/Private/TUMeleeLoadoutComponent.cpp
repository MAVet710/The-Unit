#include "TUMeleeLoadoutComponent.h"

#include "TU_Karambit.h"
#include "TU_OTFKnife.h"

UTUMeleeLoadoutComponent::UTUMeleeLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    FTUMeleeEquipmentEntry OTF;
    OTF.ItemId = TEXT("MELEE_OTF");
    OTF.DisplayName = FText::FromString(TEXT("OTF Knife"));
    OTF.MeleeClass = ATU_OTFKnife::StaticClass();
    OTF.EquipSocket = TEXT("weapon_socket");
    OTF.WeightKg = 0.14f;

    FTUMeleeEquipmentEntry Karambit;
    Karambit.ItemId = TEXT("MELEE_Karambit");
    Karambit.DisplayName = FText::FromString(TEXT("Karambit"));
    Karambit.MeleeClass = ATU_Karambit::StaticClass();
    Karambit.EquipSocket = TEXT("weapon_socket");
    Karambit.WeightKg = 0.18f;

    AvailableItems = {OTF, Karambit};
    SelectedItemId = OTF.ItemId;
}

void UTUMeleeLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureValidSelection();
}

bool UTUMeleeLoadoutComponent::AddOrReplaceItem(const FTUMeleeEquipmentEntry& Entry)
{
    if (!Entry.IsValidEntry())
    {
        return false;
    }

    const int32 ExistingIndex = FindItemIndex(Entry.ItemId);
    if (ExistingIndex != INDEX_NONE)
    {
        AvailableItems[ExistingIndex] = Entry;
    }
    else
    {
        AvailableItems.Add(Entry);
    }

    EnsureValidSelection();
    return true;
}

bool UTUMeleeLoadoutComponent::RemoveItem(FName ItemId)
{
    const int32 Index = FindItemIndex(ItemId);
    if (Index == INDEX_NONE)
    {
        return false;
    }

    const bool bRemovedSelectedItem = SelectedItemId == ItemId;
    AvailableItems.RemoveAt(Index);
    if (bRemovedSelectedItem)
    {
        SelectedItemId = NAME_None;
    }

    EnsureValidSelection();
    return true;
}

bool UTUMeleeLoadoutComponent::SelectItemById(FName ItemId)
{
    if (FindItemIndex(ItemId) == INDEX_NONE)
    {
        return false;
    }

    SelectedItemId = ItemId;
    return true;
}

bool UTUMeleeLoadoutComponent::CycleSelection(int32 Direction)
{
    if (AvailableItems.Num() <= 1 || Direction == 0)
    {
        return false;
    }

    EnsureValidSelection();
    int32 CurrentIndex = FindItemIndex(SelectedItemId);
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }

    const int32 Step = Direction > 0 ? 1 : -1;
    const int32 NextIndex = (CurrentIndex + Step + AvailableItems.Num()) % AvailableItems.Num();
    SelectedItemId = AvailableItems[NextIndex].ItemId;
    return true;
}

void UTUMeleeLoadoutComponent::SetItems(const TArray<FTUMeleeEquipmentEntry>& NewItems, FName PreferredItemId)
{
    AvailableItems.Reset();
    SelectedItemId = NAME_None;

    for (const FTUMeleeEquipmentEntry& Entry : NewItems)
    {
        if (!Entry.IsValidEntry())
        {
            continue;
        }

        const int32 ExistingIndex = FindItemIndex(Entry.ItemId);
        if (ExistingIndex != INDEX_NONE)
        {
            AvailableItems[ExistingIndex] = Entry;
        }
        else
        {
            AvailableItems.Add(Entry);
        }
    }

    if (!PreferredItemId.IsNone() && FindItemIndex(PreferredItemId) != INDEX_NONE)
    {
        SelectedItemId = PreferredItemId;
    }

    EnsureValidSelection();
}

bool UTUMeleeLoadoutComponent::GetSelectedItem(FTUMeleeEquipmentEntry& OutItem) const
{
    const int32 Index = FindItemIndex(SelectedItemId);
    if (Index == INDEX_NONE)
    {
        return false;
    }

    OutItem = AvailableItems[Index];
    return true;
}

TSubclassOf<ATU_OTFKnife> UTUMeleeLoadoutComponent::GetSelectedMeleeClass() const
{
    FTUMeleeEquipmentEntry Entry;
    return GetSelectedItem(Entry) ? Entry.MeleeClass : nullptr;
}

float UTUMeleeLoadoutComponent::GetSelectedWeightKg() const
{
    FTUMeleeEquipmentEntry Entry;
    return GetSelectedItem(Entry) ? Entry.WeightKg : 0.0f;
}

int32 UTUMeleeLoadoutComponent::FindItemIndex(FName ItemId) const
{
    return AvailableItems.IndexOfByPredicate([ItemId](const FTUMeleeEquipmentEntry& Entry)
    {
        return Entry.ItemId == ItemId;
    });
}

void UTUMeleeLoadoutComponent::EnsureValidSelection()
{
    if (FindItemIndex(SelectedItemId) != INDEX_NONE)
    {
        return;
    }

    SelectedItemId = AvailableItems.Num() > 0 ? AvailableItems[0].ItemId : NAME_None;
}
