#include "TUOperatorLoadoutComponent.h"

#include "TU_AK105.h"
#include "TU_G34CM.h"
#include "TU_M110.h"
#include "TU_RGRFive7.h"
#include "TU_TacticalRifle.h"

UTUOperatorLoadoutComponent::UTUOperatorLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    FTUOperatorWeaponEntry TU556;
    TU556.ItemId = TEXT("PRIMARY_TU556");
    TU556.DisplayName = FText::FromString(TEXT("TU-556"));
    TU556.WeaponClass = ATU_TacticalRifle::StaticClass();
    TU556.WeightKg = 3.2f;

    FTUOperatorWeaponEntry AK105;
    AK105.ItemId = TEXT("PRIMARY_AK105");
    AK105.DisplayName = FText::FromString(TEXT("AK-105"));
    AK105.WeaponClass = ATU_AK105::StaticClass();
    AK105.WeightKg = 3.4f;

    FTUOperatorWeaponEntry M110;
    M110.ItemId = TEXT("PRIMARY_M110");
    M110.DisplayName = FText::FromString(TEXT("M110 DMR"));
    M110.WeaponClass = ATU_M110::StaticClass();
    M110.WeightKg = 5.4f;

    PrimaryItems = {TU556, AK105, M110};
    SelectedPrimaryId = TU556.ItemId;

    FTUOperatorWeaponEntry G34;
    G34.ItemId = TEXT("SECONDARY_G34CM");
    G34.DisplayName = FText::FromString(TEXT("G34 CM"));
    G34.WeaponClass = ATU_G34CM::StaticClass();
    G34.WeightKg = 0.85f;

    FTUOperatorWeaponEntry Five7;
    Five7.ItemId = TEXT("SECONDARY_RGRFIVE7");
    Five7.DisplayName = FText::FromString(TEXT("RGR Five7"));
    Five7.WeaponClass = ATU_RGRFive7::StaticClass();
    Five7.WeightKg = 0.75f;

    SecondaryItems = {G34, Five7};
    SelectedSecondaryId = G34.ItemId;

    FTUOperatorEquipmentEntry EmptyEquipment;
    EmptyEquipment.ItemId = TEXT("EQUIPMENT_EMPTY");
    EmptyEquipment.DisplayName = FText::FromString(TEXT("None"));
    EmptyEquipment.WeightKg = 0.0f;
    EquipmentItems = {EmptyEquipment};
    SelectedEquipmentId = EmptyEquipment.ItemId;
}

void UTUOperatorLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureValidSelections();
}

bool UTUOperatorLoadoutComponent::SelectPrimaryById(FName ItemId)
{
    if (FindWeaponIndex(PrimaryItems, ItemId) == INDEX_NONE)
    {
        return false;
    }
    SelectedPrimaryId = ItemId;
    return true;
}

bool UTUOperatorLoadoutComponent::SelectSecondaryById(FName ItemId)
{
    if (FindWeaponIndex(SecondaryItems, ItemId) == INDEX_NONE)
    {
        return false;
    }
    SelectedSecondaryId = ItemId;
    return true;
}

bool UTUOperatorLoadoutComponent::SelectEquipmentById(FName ItemId)
{
    if (FindEquipmentIndex(ItemId) == INDEX_NONE)
    {
        return false;
    }
    SelectedEquipmentId = ItemId;
    return true;
}

TSubclassOf<ATU_WeaponBase> UTUOperatorLoadoutComponent::GetSelectedPrimaryClass() const
{
    const int32 Index = FindWeaponIndex(PrimaryItems, SelectedPrimaryId);
    return Index != INDEX_NONE ? PrimaryItems[Index].WeaponClass : nullptr;
}

TSubclassOf<ATU_WeaponBase> UTUOperatorLoadoutComponent::GetSelectedSecondaryClass() const
{
    const int32 Index = FindWeaponIndex(SecondaryItems, SelectedSecondaryId);
    return Index != INDEX_NONE ? SecondaryItems[Index].WeaponClass : nullptr;
}

float UTUOperatorLoadoutComponent::GetSelectedPrimaryWeightKg() const
{
    const int32 Index = FindWeaponIndex(PrimaryItems, SelectedPrimaryId);
    return Index != INDEX_NONE ? PrimaryItems[Index].WeightKg : 0.0f;
}

float UTUOperatorLoadoutComponent::GetSelectedSecondaryWeightKg() const
{
    const int32 Index = FindWeaponIndex(SecondaryItems, SelectedSecondaryId);
    return Index != INDEX_NONE ? SecondaryItems[Index].WeightKg : 0.0f;
}

float UTUOperatorLoadoutComponent::GetSelectedEquipmentWeightKg() const
{
    const int32 Index = FindEquipmentIndex(SelectedEquipmentId);
    return Index != INDEX_NONE ? EquipmentItems[Index].WeightKg : 0.0f;
}

float UTUOperatorLoadoutComponent::GetSelectedNonMeleeWeightKg() const
{
    return GetSelectedPrimaryWeightKg() + GetSelectedSecondaryWeightKg() + GetSelectedEquipmentWeightKg();
}

void UTUOperatorLoadoutComponent::SetPrimaryItems(const TArray<FTUOperatorWeaponEntry>& NewItems, FName PreferredItemId)
{
    PrimaryItems.Reset();
    for (const FTUOperatorWeaponEntry& Entry : NewItems)
    {
        if (!Entry.IsValidEntry())
        {
            continue;
        }
        const int32 Existing = FindWeaponIndex(PrimaryItems, Entry.ItemId);
        if (Existing == INDEX_NONE)
        {
            PrimaryItems.Add(Entry);
        }
        else
        {
            PrimaryItems[Existing] = Entry;
        }
    }
    SelectedPrimaryId = FindWeaponIndex(PrimaryItems, PreferredItemId) != INDEX_NONE ? PreferredItemId : NAME_None;
    EnsureValidSelections();
}

void UTUOperatorLoadoutComponent::SetSecondaryItems(const TArray<FTUOperatorWeaponEntry>& NewItems, FName PreferredItemId)
{
    SecondaryItems.Reset();
    for (const FTUOperatorWeaponEntry& Entry : NewItems)
    {
        if (!Entry.IsValidEntry())
        {
            continue;
        }
        const int32 Existing = FindWeaponIndex(SecondaryItems, Entry.ItemId);
        if (Existing == INDEX_NONE)
        {
            SecondaryItems.Add(Entry);
        }
        else
        {
            SecondaryItems[Existing] = Entry;
        }
    }
    SelectedSecondaryId = FindWeaponIndex(SecondaryItems, PreferredItemId) != INDEX_NONE ? PreferredItemId : NAME_None;
    EnsureValidSelections();
}

void UTUOperatorLoadoutComponent::SetEquipmentItems(const TArray<FTUOperatorEquipmentEntry>& NewItems, FName PreferredItemId)
{
    EquipmentItems.Reset();
    for (const FTUOperatorEquipmentEntry& Entry : NewItems)
    {
        if (!Entry.IsValidEntry())
        {
            continue;
        }
        const int32 Existing = FindEquipmentIndex(Entry.ItemId);
        if (Existing == INDEX_NONE)
        {
            EquipmentItems.Add(Entry);
        }
        else
        {
            EquipmentItems[Existing] = Entry;
        }
    }
    SelectedEquipmentId = FindEquipmentIndex(PreferredItemId) != INDEX_NONE ? PreferredItemId : NAME_None;
    EnsureValidSelections();
}

int32 UTUOperatorLoadoutComponent::FindWeaponIndex(const TArray<FTUOperatorWeaponEntry>& Items, FName ItemId) const
{
    return Items.IndexOfByPredicate([ItemId](const FTUOperatorWeaponEntry& Entry)
    {
        return Entry.ItemId == ItemId;
    });
}

int32 UTUOperatorLoadoutComponent::FindEquipmentIndex(FName ItemId) const
{
    return EquipmentItems.IndexOfByPredicate([ItemId](const FTUOperatorEquipmentEntry& Entry)
    {
        return Entry.ItemId == ItemId;
    });
}

void UTUOperatorLoadoutComponent::EnsureValidSelections()
{
    if (FindWeaponIndex(PrimaryItems, SelectedPrimaryId) == INDEX_NONE)
    {
        SelectedPrimaryId = PrimaryItems.Num() > 0 ? PrimaryItems[0].ItemId : NAME_None;
    }
    if (FindWeaponIndex(SecondaryItems, SelectedSecondaryId) == INDEX_NONE)
    {
        SelectedSecondaryId = SecondaryItems.Num() > 0 ? SecondaryItems[0].ItemId : NAME_None;
    }
    if (FindEquipmentIndex(SelectedEquipmentId) == INDEX_NONE)
    {
        SelectedEquipmentId = EquipmentItems.Num() > 0 ? EquipmentItems[0].ItemId : NAME_None;
    }
}
