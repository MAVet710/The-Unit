#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TUHideoutLifecycleSubsystem.generated.h"

class ATU_ArmedOperatorCharacter;
class UTUHideoutProgressionComponent;
class UTUHideoutSaveGame;
class UTUMissionPackageData;

/** Owns local hideout persistence, loadout persistence and HQ <-> mission travel state. */
UCLASS()
class THEUNIT_API UTUHideoutLifecycleSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    bool LoadProfile();

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    bool SaveProfile();

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void ApplyHideoutState(UTUHideoutProgressionComponent* Progression) const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void CaptureHideoutState(const UTUHideoutProgressionComponent* Progression);

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void ApplyOperatorLoadout(ATU_ArmedOperatorCharacter* Operator) const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Persistence")
    void CaptureOperatorLoadout(const ATU_ArmedOperatorCharacter* Operator);

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    bool DeployToMission(ATU_ArmedOperatorCharacter* Operator, const UTUMissionPackageData* MissionPackage);

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    bool ReturnToHideout(bool bOperationCompleted);

    UFUNCTION(BlueprintPure, Category="Hideout|Mission")
    bool IsMissionInProgress() const;

    UFUNCTION(BlueprintPure, Category="Hideout|Mission")
    FName GetActiveMissionId() const;

    UFUNCTION(BlueprintCallable, Category="Hideout|Mission")
    void SetHideoutMapName(FName MapName);

    UFUNCTION(BlueprintPure, Category="Hideout|Persistence")
    UTUHideoutSaveGame* GetProfile() const { return Profile; }

protected:
    UPROPERTY(Transient)
    TObjectPtr<UTUHideoutSaveGame> Profile;

    UPROPERTY(EditDefaultsOnly, Category="Hideout|Persistence")
    FString SaveSlotName = TEXT("TheUnit_HideoutProfile");

    UPROPERTY(EditDefaultsOnly, Category="Hideout|Persistence")
    int32 SaveUserIndex = 0;
};
