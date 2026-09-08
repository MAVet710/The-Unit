#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

    UFUNCTION(BlueprintPure, Category="Hideout")
    UTUHideoutProgressionComponent* GetProgression() const { return Progression; }

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
};
