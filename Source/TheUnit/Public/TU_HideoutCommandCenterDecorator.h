#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutCommandCenterDecorator.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTUHideoutProgressionComponent;

/** Additive lived-in/upgradeable hideout layer for the existing command-center hub. */
UCLASS(Blueprintable)
class THEUNIT_API ATU_HideoutCommandCenterDecorator : public AActor
{
    GENERATED_BODY()

public:
    ATU_HideoutCommandCenterDecorator();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure) FString GetGeneratedGeometrySignature() const;
    UFUNCTION(BlueprintPure) int32 GetGeneratedCollisionComponentCount() const;

    UFUNCTION(BlueprintPure, Category="Hideout")
    UTUHideoutProgressionComponent* GetProgression() const { return Progression; }

    /** Rebuild graybox/environment state after persistence or an upgrade changes module levels. */
    UFUNCTION(BlueprintCallable, Category="Hideout")
    void RefreshFromProgression();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hideout")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hideout")
    TObjectPtr<UTUHideoutProgressionComponent> Progression;

    /** If true, the decorator aligns to the first command-center generator found at BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout")
    bool bSnapToCommandCenterAtBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hideout")
    bool bGenerateLabels = true;

private:
    // Read-only presentation snapshot from host progression, never a guest save authority.
    UPROPERTY(ReplicatedUsing=OnRep_LayoutModules) TArray<FTUHideoutModuleState> LayoutModules;
    UFUNCTION() void OnRep_LayoutModules();
    bool bInitialLayoutReceived = false;
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

    void Rebuild();
    void ClearGenerated();
    UStaticMeshComponent* AddCube(const FName& Name, const FVector& Location, const FVector& Extents, const FRotator& Rotation = FRotator::ZeroRotator, bool bCollision = false);
    void BuildUtilities();
    void BuildStorageAndStaging();
    void BuildMaintenance();
    void BuildMedical();
    void BuildCommsAndPlanning();
    void BuildArmoryAndRangeSupport();
};
