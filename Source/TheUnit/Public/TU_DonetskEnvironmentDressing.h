#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_DonetskEnvironmentDressing.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * Production detail layer for the Donetsk raid.
 *
 * The district generator owns large-scale architecture and collision. This actor owns
 * repeatable street-scale art so the world can be visually dense without spawning
 * hundreds of independent actors.
 */
UCLASS()
class THEUNIT_API ATU_DonetskEnvironmentDressing : public AActor
{
    GENERATED_BODY()

public:
    ATU_DonetskEnvironmentDressing();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LampPosts;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadDashes;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Barriers;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Sedans;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Dumpsters;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BusShelters;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> UtilityCabinets;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Kiosks;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RubblePiles;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Pallets;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TrafficSigns;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Benches;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Planters;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Crates;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Manholes;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Puddles;

    void RebuildDressing();
    static void Add(UHierarchicalInstancedStaticMeshComponent* Layer, const FVector& Location,
        float YawDegrees = 0.0f, const FVector& Scale = FVector::OneVector);
};
