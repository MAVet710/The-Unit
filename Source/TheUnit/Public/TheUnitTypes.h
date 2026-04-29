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
