#include "TU_ModularOperatorCharacter.h"

#include "TUEquipmentDefinition.h"
#include "TUHealthComponent.h"
#include "TUOperatorAppearanceData.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

ATU_ModularOperatorCharacter::ATU_ModularOperatorCharacter()
{
    EquipmentComponent = CreateDefaultSubobject<UTUOperatorEquipmentComponent>(TEXT("OperatorEquipment"));
    ArmorProtectionComponent = CreateDefaultSubobject<UTUArmorProtectionComponent>(TEXT("ArmorProtection"));
    HealthComponent = CreateDefaultSubobject<UTUHealthComponent>(TEXT("OperatorHealth"));
}

void ATU_ModularOperatorCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (ArmorProtectionComponent)
    {
        ArmorProtectionComponent->InitializeEquipment(EquipmentComponent);
    }

    ApplyOperatorAppearance();
}

void ATU_ModularOperatorCharacter::SetOperatorAppearance(UTUOperatorAppearanceData* NewAppearance)
{
    OperatorAppearance = NewAppearance;

    if (HasActorBegunPlay())
    {
        ApplyOperatorAppearance();
    }
}

void ATU_ModularOperatorCharacter::AddAppearanceLoadoutToCatalog()
{
    if (!OperatorAppearance || !OperatorAppearance->DefaultLoadout)
    {
        return;
    }

    for (UTUEquipmentDefinition* Definition : OperatorAppearance->DefaultLoadout->Items)
    {
        if (!Definition)
        {
            continue;
        }

        const bool bAlreadyPresent = AvailableGear.ContainsByPredicate([Definition](const TObjectPtr<UTUEquipmentDefinition>& Existing)
        {
            return Existing == Definition || (Existing && !Definition->ItemId.IsNone() && Existing->ItemId == Definition->ItemId);
        });

        if (!bAlreadyPresent)
        {
            AvailableGear.Add(Definition);
        }
    }
}

void ATU_ModularOperatorCharacter::ApplyOperatorAppearance()
{
    USkeletalMeshComponent* ThirdPersonBody = GetMesh();
    AddAppearanceLoadoutToCatalog();

    if (OperatorAppearance)
    {
        if (ThirdPersonBody)
        {
            if (!OperatorAppearance->ThirdPersonBodyMesh.IsNull())
            {
                if (USkeletalMesh* BodyMesh = OperatorAppearance->ThirdPersonBodyMesh.LoadSynchronous())
                {
                    ThirdPersonBody->SetSkeletalMeshAsset(BodyMesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to load operator body mesh for appearance '%s'."), *OperatorAppearance->GetName());
                }
            }

            ThirdPersonBody->SetRelativeTransform(OperatorAppearance->ThirdPersonMeshTransform);
            ThirdPersonBody->SetOwnerNoSee(OperatorAppearance->bHideThirdPersonBodyFromOwner);

            if (OperatorAppearance->ThirdPersonAnimClass)
            {
                ThirdPersonBody->SetAnimInstanceClass(OperatorAppearance->ThirdPersonAnimClass);
            }
        }

        if (FirstPersonArmsMesh)
        {
            if (!OperatorAppearance->FirstPersonArmsMesh.IsNull())
            {
                if (USkeletalMesh* ArmsMesh = OperatorAppearance->FirstPersonArmsMesh.LoadSynchronous())
                {
                    FirstPersonArmsMesh->SetSkeletalMeshAsset(ArmsMesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to load first-person arms mesh for appearance '%s'."), *OperatorAppearance->GetName());
                }
            }

            FirstPersonArmsMesh->SetRelativeTransform(OperatorAppearance->FirstPersonArmsTransform);
            FirstPersonArmsMesh->SetOnlyOwnerSee(true);

            if (OperatorAppearance->FirstPersonAnimClass)
            {
                FirstPersonArmsMesh->SetAnimInstanceClass(OperatorAppearance->FirstPersonAnimClass);
            }
        }
    }

    if (EquipmentComponent)
    {
        EquipmentComponent->InitializeVisualRoots(ThirdPersonBody, FirstPersonArmsMesh);

        if (OperatorAppearance && OperatorAppearance->DefaultLoadout)
        {
            EquipmentComponent->ApplyLoadout(OperatorAppearance->DefaultLoadout);
        }
    }

    if (ArmorProtectionComponent)
    {
        ArmorProtectionComponent->ResetArmorState();
    }
}

TArray<UTUEquipmentDefinition*> ATU_ModularOperatorCharacter::GetAvailableGear() const
{
    TArray<UTUEquipmentDefinition*> Result;
    Result.Reserve(AvailableGear.Num());
    for (UTUEquipmentDefinition* Definition : AvailableGear)
    {
        if (Definition)
        {
            Result.Add(Definition);
        }
    }
    return Result;
}

void ATU_ModularOperatorCharacter::SetAvailableGear(const TArray<UTUEquipmentDefinition*>& NewGear)
{
    AvailableGear.Reset();
    TSet<FName> SeenIds;

    for (UTUEquipmentDefinition* Definition : NewGear)
    {
        if (!Definition)
        {
            continue;
        }

        if (!Definition->ItemId.IsNone())
        {
            if (SeenIds.Contains(Definition->ItemId))
            {
                continue;
            }
            SeenIds.Add(Definition->ItemId);
        }
        AvailableGear.Add(Definition);
    }

    AddAppearanceLoadoutToCatalog();
}

UTUEquipmentDefinition* ATU_ModularOperatorCharacter::FindAvailableGear(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return nullptr;
    }

    for (UTUEquipmentDefinition* Definition : AvailableGear)
    {
        if (Definition && Definition->ItemId == ItemId)
        {
            return Definition;
        }
    }
    return nullptr;
}

bool ATU_ModularOperatorCharacter::EquipGearById(FName ItemId)
{
    UTUEquipmentDefinition* Definition = FindAvailableGear(ItemId);
    if (!Definition || !EquipmentComponent || !EquipmentComponent->EquipItem(Definition))
    {
        return false;
    }

    if (ArmorProtectionComponent)
    {
        ArmorProtectionComponent->ResetArmorState();
    }
    return true;
}

bool ATU_ModularOperatorCharacter::UnequipGearSlot(ETUEquipmentSlot Slot)
{
    if (!EquipmentComponent || !EquipmentComponent->UnequipSlot(Slot))
    {
        return false;
    }

    if (ArmorProtectionComponent)
    {
        ArmorProtectionComponent->ResetArmorState();
    }
    return true;
}

FName ATU_ModularOperatorCharacter::GetEquippedGearId(ETUEquipmentSlot Slot) const
{
    if (!EquipmentComponent)
    {
        return NAME_None;
    }

    if (UTUEquipmentDefinition* Definition = EquipmentComponent->GetEquippedItem(Slot))
    {
        return Definition->ItemId.IsNone() ? Definition->GetFName() : Definition->ItemId;
    }
    return NAME_None;
}

float ATU_ModularOperatorCharacter::GetTotalCombatLoadoutWeightKg() const
{
    const float WeaponAndMeleeWeight = GetSelectedLoadoutWeightKg();
    const float GearWeight = EquipmentComponent ? EquipmentComponent->GetTotalEquipmentWeightKg() : 0.0f;
    return WeaponAndMeleeWeight + GearWeight;
}

FTUArmorHitResult ATU_ModularOperatorCharacter::ApplyBallisticRegionalDamage(
    ETUBodyRegion Region,
    float IncomingDamage,
    float Penetration,
    float ArmorDamage,
    float CoverageRoll01)
{
    FTUArmorHitResult Result;
    Result.IncomingDamage = FMath::Max(0.0f, IncomingDamage);
    Result.FinalDamage = Result.IncomingDamage;

    if (ArmorProtectionComponent)
    {
        Result = ArmorProtectionComponent->ResolveBallisticHit(
            Region,
            Result.IncomingDamage,
            Penetration,
            ArmorDamage,
            CoverageRoll01);
    }

    if (HealthComponent && Result.FinalDamage > 0.0f)
    {
        HealthComponent->ApplyRegionalDamage(Region, Result.FinalDamage);
    }

    return Result;
}
