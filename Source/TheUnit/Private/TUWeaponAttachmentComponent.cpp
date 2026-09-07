#include "TUWeaponAttachmentComponent.h"

#include "TUWeaponLoadoutData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

UTUWeaponAttachmentComponent::UTUWeaponAttachmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTUWeaponAttachmentComponent::InitializeVisualRoot(USceneComponent* InVisualRoot)
{
    VisualRoot = InVisualRoot;
    for (const TPair<ETUWeaponAttachmentSlot, TObjectPtr<UTUWeaponAttachmentDefinition>>& Pair : EquippedAttachments)
    {
        RebuildVisual(Pair.Key);
    }
}

bool UTUWeaponAttachmentComponent::EquipAttachment(UTUWeaponAttachmentDefinition* Definition)
{
    if (!Definition)
    {
        return false;
    }

    EquippedAttachments.Add(Definition->Slot, Definition);
    RebuildVisual(Definition->Slot);
    return true;
}

bool UTUWeaponAttachmentComponent::UnequipAttachment(ETUWeaponAttachmentSlot Slot)
{
    if (!EquippedAttachments.Contains(Slot))
    {
        return false;
    }

    EquippedAttachments.Remove(Slot);
    DestroyVisual(Slot);
    return true;
}

void UTUWeaponAttachmentComponent::ClearAttachments()
{
    TArray<ETUWeaponAttachmentSlot> Slots;
    EquippedAttachments.GetKeys(Slots);
    for (const ETUWeaponAttachmentSlot Slot : Slots)
    {
        DestroyVisual(Slot);
    }
    EquippedAttachments.Empty();
}

bool UTUWeaponAttachmentComponent::ApplyLoadout(UTUWeaponLoadoutData* Loadout)
{
    if (!Loadout)
    {
        return false;
    }

    ClearAttachments();
    bool bAppliedAny = false;
    for (UTUWeaponAttachmentDefinition* Definition : Loadout->Attachments)
    {
        if (Definition)
        {
            bAppliedAny |= EquipAttachment(Definition);
        }
    }
    return bAppliedAny || Loadout->Attachments.Num() == 0;
}

UTUWeaponAttachmentDefinition* UTUWeaponAttachmentComponent::GetAttachment(ETUWeaponAttachmentSlot Slot) const
{
    if (const TObjectPtr<UTUWeaponAttachmentDefinition>* Found = EquippedAttachments.Find(Slot))
    {
        return Found->Get();
    }
    return nullptr;
}

float UTUWeaponAttachmentComponent::GetRecoilMultiplier() const
{
    float Multiplier = 1.0f;
    for (const TPair<ETUWeaponAttachmentSlot, TObjectPtr<UTUWeaponAttachmentDefinition>>& Pair : EquippedAttachments)
    {
        if (Pair.Value)
        {
            Multiplier *= FMath::Clamp(Pair.Value->RecoilMultiplier, 0.1f, 3.0f);
        }
    }
    return FMath::Clamp(Multiplier, 0.25f, 4.0f);
}

float UTUWeaponAttachmentComponent::GetSpreadMultiplier() const
{
    float Multiplier = 1.0f;
    for (const TPair<ETUWeaponAttachmentSlot, TObjectPtr<UTUWeaponAttachmentDefinition>>& Pair : EquippedAttachments)
    {
        if (Pair.Value)
        {
            Multiplier *= FMath::Clamp(Pair.Value->SpreadMultiplier, 0.1f, 3.0f);
        }
    }
    return FMath::Clamp(Multiplier, 0.25f, 4.0f);
}

float UTUWeaponAttachmentComponent::GetAttachmentWeightKg() const
{
    float Total = 0.0f;
    for (const TPair<ETUWeaponAttachmentSlot, TObjectPtr<UTUWeaponAttachmentDefinition>>& Pair : EquippedAttachments)
    {
        if (Pair.Value)
        {
            Total += FMath::Max(0.0f, Pair.Value->WeightKg);
        }
    }
    return Total;
}

UStaticMeshComponent* UTUWeaponAttachmentComponent::GetAttachmentVisual(ETUWeaponAttachmentSlot Slot) const
{
    if (const TObjectPtr<UStaticMeshComponent>* Found = AttachmentVisuals.Find(Slot))
    {
        return Found->Get();
    }
    return nullptr;
}

void UTUWeaponAttachmentComponent::RebuildVisual(ETUWeaponAttachmentSlot Slot)
{
    DestroyVisual(Slot);

    if (!VisualRoot)
    {
        return;
    }

    UTUWeaponAttachmentDefinition* Definition = GetAttachment(Slot);
    if (!Definition || Definition->Mesh.IsNull())
    {
        return;
    }

    UStaticMesh* Mesh = Definition->Mesh.LoadSynchronous();
    if (!Mesh || !GetOwner())
    {
        return;
    }

    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetOwner());
    if (!Component)
    {
        return;
    }

    GetOwner()->AddInstanceComponent(Component);
    Component->SetupAttachment(VisualRoot, Definition->AttachSocket);
    Component->SetRelativeTransform(Definition->RelativeTransform);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false);
    Component->SetOnlyOwnerSee(true);
    Component->SetCastShadow(false);
    Component->RegisterComponent();
    AttachmentVisuals.Add(Slot, Component);
}

void UTUWeaponAttachmentComponent::DestroyVisual(ETUWeaponAttachmentSlot Slot)
{
    if (TObjectPtr<UStaticMeshComponent>* Found = AttachmentVisuals.Find(Slot))
    {
        if (IsValid(Found->Get()))
        {
            Found->Get()->DestroyComponent();
        }
        AttachmentVisuals.Remove(Slot);
    }
}
