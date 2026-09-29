#pragma once

#include "CoreMinimal.h"
#include "TUExecutionTypes.generated.h"

UENUM(BlueprintType)
enum class ETUItemLocation : uint8 { Stash, Carried, Inserted, InHand, Ground, Escrow, Consumed };
UENUM(BlueprintType)
enum class ETUWeaponActionPhase : uint8 { Ready, Begin, Removed, Acquired, Inserted, Chambered };
UENUM(BlueprintType)
enum class ETUReloadPolicy : uint8 { Retain, Drop };
UENUM(BlueprintType)
enum class ETURaidPlayerOutcome : uint8 { Active, DisconnectedPendingResolution, Extracted, Dead, TimedOut, Abandoned };
UENUM(BlueprintType)
enum class ETUTaskPolicy : uint8 { Cumulative, SameRaid, ExtractRequired, PhysicalHandover };
UENUM(BlueprintType)
enum class ETUTaskCondition : uint8 { Visit, Interact, Recover, Eliminate, Survive, Handover, Destroy };
UENUM(BlueprintType)
enum class ETUTaskCreditOwner : uint8 { Individual, EligiblePresentSquad };

/** Index zero is the next cartridge fed. Identities survive containment changes. */
USTRUCT(BlueprintType)
struct THEUNIT_API FTUMagazineInstance
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid InstanceId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid OwnerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid WeaponId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName DefinitionId = TEXT("Magazine_30");
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName CompatibleAmmoId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 Capacity = 30;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FName> Cartridges;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUItemLocation Location = ETUItemLocation::Carried;
};

/** Evolved from the inspected tu-016/017 instance contract; no authoritative reserve/count snapshot. */
USTRUCT(BlueprintType)
struct THEUNIT_API FWeaponInstanceState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid InstanceId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid OwnerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName DefinitionId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName LoadoutSlot;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid InsertedMagazineId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName ChamberAmmoId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bActionOpen = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) float ConditionNormalized = 1.0f;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUItemLocation Location = ETUItemLocation::Carried;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUItemInstance
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid InstanceId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid OwnerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName DefinitionId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid FoundInRaidId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bExtracted = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName ExtractedAtId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUItemLocation Location = ETUItemLocation::Carried;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUWeaponActionState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid WeaponId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid ActionId;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 Revision = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly) ETUWeaponActionPhase Phase = ETUWeaponActionPhase::Ready;
    UPROPERTY(SaveGame, BlueprintReadOnly) ETUReloadPolicy Policy = ETUReloadPolicy::Retain;
    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid ReplacementMagazineId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid OutgoingMagazineId;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bActive = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bRequiresChamberCycle = false;
    UPROPERTY(BlueprintReadOnly) float PhaseEndServerTime = 0.0f;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUItemLedger
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FWeaponInstanceState> Weapons;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FTUMagazineInstance> Magazines;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FTUItemInstance> Items;
    /** Explicit loose/ejected cartridges, not a second reserve balance. */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FName> LooseCartridges;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FTUWeaponActionState> WeaponActions;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 Revision = 0;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUTaskStep
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUTaskCondition Condition = ETUTaskCondition::Interact;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName TargetId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 RequiredCount = 1;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUTaskDefinition
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName TaskId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUTaskPolicy Policy = ETUTaskPolicy::Cumulative;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUTaskCreditOwner CreditOwner = ETUTaskCreditOwner::Individual;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETUTaskCondition Condition = ETUTaskCondition::Interact;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName TargetId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName RequiredExtractId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 RequiredCount = 1;
    /** Empty uses the legacy single condition; otherwise every heterogeneous step is required. */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FTUTaskStep> Steps;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUTaskProgress
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FTUTaskDefinition Definition;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid PlayerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid PendingRaidId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 CommittedCount = 0;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 PendingCount = 0;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bCompleted = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FGuid> ConsumedItemIds;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<int32> CommittedSteps;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<int32> PendingSteps;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TArray<FGuid> ProcessedEventIds;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTURaidOutcome
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid RaidId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid PlayerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 Revision = 1;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) ETURaidPlayerOutcome Outcome = ETURaidPlayerOutcome::Active;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName ExtractId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bTraining = false;
};

USTRUCT(BlueprintType)
struct THEUNIT_API FTUDeploymentRecord
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid RaidId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FGuid PlayerId;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FTUItemLedger Escrow;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bTraining = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bResolved = false;
};
