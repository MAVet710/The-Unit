#include "TUOperatorEquipmentComponent.h"

#include "TUEquipmentDefinition.h"
#include "TUOperatorLoadoutData.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"

UTUOperatorEquipmentComponent::UTUOperatorEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTUOperatorEquipmentComponent::InitializeVisualRoots(
    USkeletalMeshComponent* InThirdPersonBody,
    USkeletalMeshComponent* InFirstPersonArms)
{
    ThirdPersonBody = InThirdPersonBody;
    FirstPersonArms = InFirstPersonArms;
    RebuildVisuals();
}

bool UTUOperatorEquipmentComponent::EquipItem(UTUEquipmentDefinition* Definition)
{
    if (!Definition)
    {
        return false;
    }

    const ETUEquipmentSlot Slot = Definition->Slot;
    UnequipSlot(Slot);
    EquippedItems.Add(Slot, Definition);

    bool bVisualCreationFailed = false;

    if (ThirdPersonBody && Definition->HasThirdPersonVisual())
    {
        if (USceneComponent* Visual = CreateVisualForDefinition(Definition, false))
        {
            ThirdPersonVisuals.Add(Slot, Visual);
        }
        else
        {
            bVisualCreationFailed = true;
        }
    }

    if (FirstPersonArms && Definition->HasFirstPersonVisual())
    {
        if (USceneComponent* Visual = CreateVisualForDefinition(Definition, true))
        {
            FirstPersonVisuals.Add(Slot, Visual);
        }
        else
        {
            bVisualCreationFailed = true;
        }
    }

    if (bVisualCreationFailed)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create one or more equipment visuals for '%s'."), *Definition->GetName());
        UnequipSlot(Slot);
        return false;
    }

    return true;
}

bool UTUOperatorEquipmentComponent::UnequipSlot(ETUEquipmentSlot Slot)
{
    const bool bWasEquipped = EquippedItems.Contains(Slot);
    DestroyVisualForSlot(Slot);
    EquippedItems.Remove(Slot);
    return bWasEquipped;
}

void UTUOperatorEquipmentComponent::ClearLoadout()
{
    DestroyAllVisuals();
    EquippedItems.Empty();
}

bool UTUOperatorEquipmentComponent::ApplyLoadout(UTUOperatorLoadoutData* Loadout)
{
    ClearLoadout();

    if (!Loadout)
    {
        return true;
    }

    bool bAllSucceeded = true;
    TSet<ETUEquipmentSlot> SeenSlots;

    for (UTUEquipmentDefinition* Definition : Loadout->Items)
    {
        if (!Definition)
        {
            bAllSucceeded = false;
            continue;
        }

        if (SeenSlots.Contains(Definition->Slot))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Loadout '%s' contains duplicate slot %d. Later item '%s' replaces the earlier item."),
                *Loadout->GetName(),
                static_cast<int32>(Definition->Slot),
                *Definition->GetName());
        }
        SeenSlots.Add(Definition->Slot);

        bAllSucceeded = EquipItem(Definition) && bAllSucceeded;
    }

    return bAllSucceeded;
}

void UTUOperatorEquipmentComponent::RebuildVisuals()
{
    DestroyAllVisuals();

    TArray<TPair<ETUEquipmentSlot, TObjectPtr<UTUEquipmentDefinition>>> Snapshot;
    Snapshot.Reserve(EquippedItems.Num());
    for (const TPair<ETUEquipmentSlot, TObjectPtr<UTUEquipmentDefinition>>& Pair : EquippedItems)
    {
        Snapshot.Add(Pair);
    }

    for (const TPair<ETUEquipmentSlot, TObjectPtr<UTUEquipmentDefinition>>& Pair : Snapshot)
    {
        UTUEquipmentDefinition* Definition = Pair.Value.Get();
        if (!Definition)
        {
            continue;
        }

        if (ThirdPersonBody && Definition->HasThirdPersonVisual())
        {
            if (USceneComponent* Visual = CreateVisualForDefinition(Definition, false))
            {
                ThirdPersonVisuals.Add(Pair.Key, Visual);
            }
        }

        if (FirstPersonArms && Definition->HasFirstPersonVisual())
        {
            if (USceneComponent* Visual = CreateVisualForDefinition(Definition, true))
            {
                FirstPersonVisuals.Add(Pair.Key, Visual);
            }
        }
    }
}

UTUEquipmentDefinition* UTUOperatorEquipmentComponent::GetEquippedItem(ETUEquipmentSlot Slot) const
{
    if (const TObjectPtr<UTUEquipmentDefinition>* Found = EquippedItems.Find(Slot))
    {
        return Found->Get();
    }
    return nullptr;
}

bool UTUOperatorEquipmentComponent::IsSlotOccupied(ETUEquipmentSlot Slot) const
{
    return EquippedItems.Contains(Slot);
}

float UTUOperatorEquipmentComponent::GetTotalEquipmentWeightKg() const
{
    float TotalWeight = 0.0f;
    for (const TPair<ETUEquipmentSlot, TObjectPtr<UTUEquipmentDefinition>>& Pair : EquippedItems)
    {
        if (const UTUEquipmentDefinition* Definition = Pair.Value.Get())
        {
            TotalWeight += FMath::Max(0.0f, Definition->WeightKg);
        }
    }
    return TotalWeight;
}

USceneComponent* UTUOperatorEquipmentComponent::CreateVisualForDefinition(
    const UTUEquipmentDefinition* Definition,
    bool bFirstPerson)
{
    if (!Definition || !GetOwner())
    {
        return nullptr;
    }

    USkeletalMeshComponent* Parent = bFirstPerson ? FirstPersonArms.Get() : ThirdPersonBody.Get();
    if (!Parent)
    {
        return nullptr;
    }

    const TSoftObjectPtr<USkeletalMesh>& SkeletalAsset = bFirstPerson
        ? Definition->FirstPersonSkeletalMesh
        : Definition->ThirdPersonSkeletalMesh;
    const TSoftObjectPtr<UStaticMesh>& StaticAsset = bFirstPerson
        ? Definition->FirstPersonStaticMesh
        : Definition->ThirdPersonStaticMesh;
    const FName SocketName = bFirstPerson
        ? Definition->FirstPersonAttachSocket
        : Definition->AttachSocket;
    const FTransform RelativeTransform = bFirstPerson
        ? Definition->FirstPersonRelativeTransform
        : Definition->RelativeTransform;
    const bool bUseLeaderPose = bFirstPerson
        ? Definition->bUseFirstPersonLeaderPose
        : Definition->bUseLeaderPose;

    if (!SkeletalAsset.IsNull())
    {
        USkeletalMesh* Mesh = SkeletalAsset.LoadSynchronous();
        if (!Mesh)
        {
            return nullptr;
        }

        USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(GetOwner());
        if (!Component)
        {
            return nullptr;
        }

        GetOwner()->AddInstanceComponent(Component);
        Component->SetupAttachment(Parent, SocketName);
        Component->SetRelativeTransform(RelativeTransform);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetSkeletalMeshAsset(Mesh);
        Component->SetOnlyOwnerSee(bFirstPerson);
        Component->SetOwnerNoSee(!bFirstPerson && Definition->bHideThirdPersonFromOwner);
        if (bFirstPerson)
        {
            Component->SetCastShadow(false);
        }
        Component->RegisterComponent();

        if (bUseLeaderPose)
        {
            if (AreSkeletonsCompatible(Parent, Component))
            {
                Component->SetLeaderPoseComponent(Parent);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("Equipment '%s' requested leader pose but its skeleton does not match parent mesh '%s'. Falling back to socket attachment."),
                    *Definition->GetName(),
                    *Parent->GetName());
            }
        }

        return Component;
    }

    if (!StaticAsset.IsNull())
    {
        UStaticMesh* Mesh = StaticAsset.LoadSynchronous();
        if (!Mesh)
        {
            return nullptr;
        }

        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetOwner());
        if (!Component)
        {
            return nullptr;
        }

        GetOwner()->AddInstanceComponent(Component);
        Component->SetupAttachment(Parent, SocketName);
        Component->SetRelativeTransform(RelativeTransform);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetStaticMesh(Mesh);
        Component->SetOnlyOwnerSee(bFirstPerson);
        Component->SetOwnerNoSee(!bFirstPerson && Definition->bHideThirdPersonFromOwner);
        if (bFirstPerson)
        {
            Component->SetCastShadow(false);
        }
        Component->RegisterComponent();
        return Component;
    }

    return nullptr;
}

void UTUOperatorEquipmentComponent::DestroyVisualForSlot(ETUEquipmentSlot Slot)
{
    if (TObjectPtr<USceneComponent>* ThirdPerson = ThirdPersonVisuals.Find(Slot))
    {
        if (IsValid(ThirdPerson->Get()))
        {
            ThirdPerson->Get()->DestroyComponent();
        }
        ThirdPersonVisuals.Remove(Slot);
    }

    if (TObjectPtr<USceneComponent>* FirstPerson = FirstPersonVisuals.Find(Slot))
    {
        if (IsValid(FirstPerson->Get()))
        {
            FirstPerson->Get()->DestroyComponent();
        }
        FirstPersonVisuals.Remove(Slot);
    }
}

void UTUOperatorEquipmentComponent::DestroyAllVisuals()
{
    for (TPair<ETUEquipmentSlot, TObjectPtr<USceneComponent>>& Pair : ThirdPersonVisuals)
    {
        if (IsValid(Pair.Value.Get()))
        {
            Pair.Value->DestroyComponent();
        }
    }
    ThirdPersonVisuals.Empty();

    for (TPair<ETUEquipmentSlot, TObjectPtr<USceneComponent>>& Pair : FirstPersonVisuals)
    {
        if (IsValid(Pair.Value.Get()))
        {
            Pair.Value->DestroyComponent();
        }
    }
    FirstPersonVisuals.Empty();
}

bool UTUOperatorEquipmentComponent::AreSkeletonsCompatible(
    const USkeletalMeshComponent* Leader,
    const USkeletalMeshComponent* Follower) const
{
    if (!Leader || !Follower)
    {
        return false;
    }

    const USkeletalMesh* LeaderMesh = Leader->GetSkeletalMeshAsset();
    const USkeletalMesh* FollowerMesh = Follower->GetSkeletalMeshAsset();
    if (!LeaderMesh || !FollowerMesh)
    {
        return false;
    }

    return LeaderMesh->GetSkeleton() != nullptr && LeaderMesh->GetSkeleton() == FollowerMesh->GetSkeleton();
}
