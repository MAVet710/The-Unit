#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TheUnitTypes.generated.h"

UENUM(BlueprintType)
enum class ETUStance : uint8
{
	Standing,
	Crouched,
	Prone,
	LowReady,
	ADS
};

UENUM(BlueprintType)
enum class ETUBodyRegion : uint8
{
	Head,
	Chest,
	Stomach,
	LeftArm,
	RightArm,
	LeftLeg,
	RightLeg
};

UENUM(BlueprintType)
enum class ETUCalloutCategory : uint8
{
	Movement,
	Contact,
	WeaponState,
	Injury,
	Objective,
	Extraction
};

/** Shared fire-mode vocabulary. Installed fire-control data determines which modes are supported. */
UENUM(BlueprintType)
enum class ETUFireMode : uint8
{
	SemiAuto UMETA(DisplayName = "Semi Auto"),
	Burst UMETA(DisplayName = "Burst"),
	FullAuto UMETA(DisplayName = "Full Auto")
};

/** High-level gameplay trigger categories. */
UENUM(BlueprintType)
enum class ETUTriggerType : uint8
{
	Standard UMETA(DisplayName = "Standard"),
	TwoStage UMETA(DisplayName = "Two Stage"),
	Match UMETA(DisplayName = "Match"),
	Duty UMETA(DisplayName = "Duty"),
	Electronic UMETA(DisplayName = "Electronic")
};

USTRUCT(BlueprintType)
struct FAmmoDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AmmoId = TEXT("Ammo_556_Training_Ball");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 35.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Penetration = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Velocity = 870.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ArmorDamage = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BleedChance = 0.05f;
};

USTRUCT(BlueprintType)
struct FMagazineState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Capacity = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RoundsInMagazine = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRoundChambered = true;
};

USTRUCT(BlueprintType)
struct FWeaponDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName WeaponId = TEXT("WPN_TU556_Training_Rifle");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName = FText::FromString(TEXT("TU-556 Training Rifle"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoilPitch = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoilYaw = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FireRateRPM = 450.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSemiAutoOnly = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HipSpread = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ADSSpread = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CompatibleAmmoId = TEXT("Ammo_556_Training_Ball");
};

/** High-level gameplay categories for modular weapon parts. */
UENUM(BlueprintType)
enum class ETUWeaponPartSlot : uint8
{
	Barrel,
	Muzzle,
	Action,
	Handguard,
	StockBrace,
	Grip,
	MagazineFeed,
	Optic,
	OpticMount,
	RailMount,
	LightLaser,
	Underbarrel,
	Internal,
	FireControl,
	Trigger,
	Cosmetic
};

/** Data-driven gameplay behavior supplied by an installed fire-control module. */
USTRUCT(BlueprintType)
struct FFireControlModuleDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName FireControlId = TEXT("FireControl_Standard_Semi");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName = FText::FromString(TEXT("Standard Fire Control"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ETUFireMode> SupportedFireModes = { ETUFireMode::SemiAuto };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 BurstCount = 3;

	/** Legacy fallback fields retained until all authored builds install a dedicated Trigger part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TriggerProfileId = TEXT("Trigger.Standard");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float TriggerResponseMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float ResetResponseMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float SemiAutoResetDelaySeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequiresReleaseBetweenSemiShots = true;
};

/** Data-driven gameplay behavior supplied by an installed Trigger part. */
USTRUCT(BlueprintType)
struct FTriggerDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TriggerId = TEXT("Trigger_Standard");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName = FText::FromString(TEXT("Standard Trigger"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETUTriggerType TriggerType = ETUTriggerType::Standard;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float TriggerResponseMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float ResetResponseMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float SemiAutoResetDelaySeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequiresReleaseBetweenSemiShots = true;
};

/** Data-table definition for a base weapon platform/receiver family. */
USTRUCT(BlueprintType)
struct FWeaponPlatformDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PlatformId = TEXT("Platform_TU556_Training");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName = FText::FromString(TEXT("TU-556 Training Platform"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BaseWeaponDefinitionId = TEXT("WPN_TU556_Training_Rifle");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> InterfaceTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ETUWeaponPartSlot> SupportedPartSlots;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> CompatibleAmmoIds;
};

/** Reusable part definition. Compatibility is expressed as abstract interface tags. */
USTRUCT(BlueprintType)
struct FWeaponPartDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PartId = TEXT("Part_Default");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName = FText::FromString(TEXT("Weapon Part"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETUWeaponPartSlot Slot = ETUWeaponPartSlot::Internal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> RequiredInterfaceTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ProvidedInterfaceTags;
	/** Used only by FireControl parts to reference their behavior definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName FireControlDefinitionId = NAME_None;
	/** Used only by Trigger parts to reference their behavior definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TriggerDefinitionId = NAME_None;

	/** Multipliers contribute to a derived gameplay configuration; 1.0 leaves the base value unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float RecoilPitchMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float RecoilYawMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float HipSpreadMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float ADSSpreadMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float FireRateRPMMultiplier = 1.0f;
};

/** One installed part reference; an array permits multiple accessories in the same high-level category. */
USTRUCT(BlueprintType)
struct FWeaponInstalledPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETUWeaponPartSlot Slot = ETUWeaponPartSlot::Internal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PartId = NAME_None;
};

/** Serializable composition identity for a modular weapon build. */
USTRUCT(BlueprintType)
struct FWeaponBuildState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PlatformId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWeaponInstalledPart> InstalledParts;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SelectedAmmoId = NAME_None;
};

/** Phase 2 placeholder customization types. */
UENUM(BlueprintType)
enum class ETUGearSlot : uint8
{
	Head, Face, UpperBody, LowerBody, Gloves, Boots, PlateCarrier, Belt, Backpack, PrimaryWeapon, Sidearm,
	Patch_LeftShoulder, Patch_RightShoulder, Patch_Chest, Patch_Back
};

USTRUCT(BlueprintType)
struct FPatchAnchor
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETUGearSlot Slot = ETUGearSlot::Patch_LeftShoulder;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D MaxSize = FVector2D(128.f, 128.f);
};

USTRUCT(BlueprintType)
struct FOperatorProfile
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OperatorId = TEXT("Op_Default");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<ETUGearSlot, FName> EquippedItemIds;
};
