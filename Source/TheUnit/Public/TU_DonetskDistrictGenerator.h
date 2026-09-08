#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_DonetskDistrictGenerator.generated.h"

class UActorComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Procedural civilian Donetsk reference district for early layout/art validation.
 *
 * Building forms are derived from historical/public civilian architecture references
 * (Artema Street, Soviet residential typologies and the 2012 railway-station complex),
 * but the street arrangement is an original gameplay composition rather than a
 * current 1:1 operational map of a live conflict area.
 */
UCLASS(Blueprintable)
class THEUNIT_API ATU_DonetskDistrictGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATU_DonetskDistrictGenerator();
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Donetsk")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Layout", meta=(ClampMin="12000.0"))
    float DistrictWidthCm = 52000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Layout", meta=(ClampMin="12000.0"))
    float DistrictLengthCm = 62000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateReferenceLabels = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Donetsk|Layout")
    bool bGenerateTransitFurniture = true;

private:
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

    int32 GeneratedNameCounter = 0;

    void RebuildDistrict();
    void ClearGenerated();

    UStaticMeshComponent* AddBox(const FVector& Location, const FVector& Extents, const FString& BaseName,
        const FRotator& Rotation = FRotator::ZeroRotator);
    void AddLabel(const FString& Text, const FVector& Location, const FRotator& Rotation = FRotator(0.0f, 90.0f, 0.0f));

    void BuildRoadNetwork();
    void BuildArtema60Reference();
    void BuildKhrushchyovkaCourtyard();
    void BuildBrezhnevkaBlocks();
    void BuildStalinistStreetWall();
    void BuildRailStationReference();
    void BuildIndustrialEdge();
    void BuildStreetFurniture();

    void BuildSimpleFacadeBlock(const FVector& Origin, int32 Floors, int32 Bays, float BayWidthCm,
        float DepthCm, float FloorHeightCm, const FString& Prefix, bool bBalconies, bool bRaisedGroundFloor);
};