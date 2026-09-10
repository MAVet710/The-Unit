#include "TU_ModularOperatorCharacter.h"

#include "TUEquipmentDefinition.h"
#include "TUHealthComponent.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUOperatorAppearanceData.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutData.h"
#include "TU_WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
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

    // Possession can happen before the authored appearance/catalog is ready. Reapply the
    // persisted loadout after appearance construction so saved Cage gear always wins.
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
        {
            Lifecycle->ApplyOperatorLoadout(this);
        }
    }
}

float ATU_ModularOperatorCharacter::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
        if (const ATU_WeaponBase* Weapon = Cast<ATU_WeaponBase>(DamageCauser))
        {
            const FAmmoDefinition Ammo = Weapon->GetAmmoDefinition();
            const ETUBodyRegion Region = ResolveBodyRegionFromBone(PointDamage.HitInfo.BoneName);
            const FTUArmorHitResult Result = ApplyBallisticRegionalDamage(
                Region,
                DamageAmount,
                Ammo.Penetration,
                Ammo.ArmorDamage,
                FMath::FRand());
            return Result.FinalDamage;
        }
    }

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

ETUBodyRegion ATU_ModularOperatorCharacter::ResolveBodyRegionFromBone(FName BoneName) const
{
    const FString Bone = BoneName.ToString().ToLower();

    if (Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck")))
    {
        return ETUBodyRegion::Head;
    }
    if (Bone.Contains(TEXT("leftarm")) || Bone.Contains(TEXT("leftforearm")) ||
        Bone.Contains(TEXT("lefthand")) || Bone.Contains(TEXT("leftshoulder")))
    {
        return ETUBodyRegion::LeftArm;
    }
    if (Bone.Contains(TEXT("rightarm")) || Bone.Contains(TEXT("rightforearm")) ||
        Bone.Contains(TEXT("righthand")) || Bone.Contains(TEXT("rightshoulder")))
    {
        return ETUBodyRegion::RightArm;
    }
    if (Bone.Contains(TEXT("leftupleg")) || Bone.Contains(TEXT("leftleg")) ||
        Bone.Contains(TEXT("leftfoot")) || Bone.Contains(TEXT("lefttoe")))
    {
        return ETUBodyRegion::LeftLeg;
    }
    if (Bone.Contains(TEXT("rightupleg")) || Bone.Contains(TEXT("rightleg")) ||
        Bone.Contains(TEXT("rightfoot")) || Bone.Contains(TEXT("righttoe")))
    {
        return ETUBodyRegion::RightLeg;
    }
    if (Bone.Contains(TEXT("pelvis")) || Bone.Contains(TEXT("hips")) || Bone.Contains(TEXT("stomach")))
    {
        return ETUBodyRegion::Stomach;
    }

    // Unknown/torso/spine/clavicle bones resolve to Chest rather than silently bypassing armor.
    return ETUBodyRegion::Chest;
}

void ATU_ModularOperatorCharacter::SetOperatorAppearance(UTUOperatorAppearanceData* NewAppearance)
{
    OperatorAppearance = NewAppearance;

    if (HasActorBegunPlay())
    {
        ApplyOperatorAppearance();

        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
            {
                Lifecycle->ApplyOperatorLoadout(this);
            }
        }
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
