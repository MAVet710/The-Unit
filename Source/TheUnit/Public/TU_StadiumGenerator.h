#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_StadiumGenerator.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Procedural Phase 1D.2 graybox stadium generator inspired by old asymmetrical parks.
 * Produces a tactical training playspace without licensed branding.
 */
UCLASS()
class THEUNIT_API ATU_StadiumGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATU_StadiumGenerator();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stadium")
    USceneComponent* Root;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "4.0"))
    float FieldScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateField = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateOutfieldWalls = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateMonsterWall = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateInfield = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateDugouts = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateLowerBowl = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateUpperDeck = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateExteriorShell = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateDebugLabels = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Generation")
    bool bGenerateGameplayMarkers = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "250.0"))
    float LeftFieldDistanceFt = 310.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "300.0"))
    float LeftCenterDistanceFt = 379.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "300.0"))
    float CenterFieldDistanceFt = 390.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "350.0"))
    float DeepCenterDistanceFt = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "300.0"))
    float DeepRightDistanceFt = 380.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "250.0"))
    float RightFieldDistanceFt = 302.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "15.0"))
    float MonsterWallHeightFt = 37.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "8.0"))
    float CenterWallHeightFt = 17.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stadium|Field", meta = (ClampMin = "3.0"))
    float RightFieldWallHeightFt = 5.0f;

private:
    static constexpr float UnitsPerFoot = 30.48f;

    UPROPERTY(Transient)
    UStaticMesh* CubeMesh;

    UPROPERTY(Transient)
    TArray<UActorComponent*> GeneratedComponents;

    void ClearGeneratedComponents();
    void BuildStadium();

    UStaticMeshComponent* AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation = FRotator::ZeroRotator);
    UArrowComponent* AddMarker(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, const FName& Name);
    void SpawnDebugLabel(const FString& Label, const FVector& Location);

    void GenerateField();
    void GenerateInfield();
    void GenerateFoulLines();
    void GenerateOutfieldWalls();
    void GenerateMonsterWall();
    void GenerateDugouts();
    void GenerateLowerBowl();
    void GenerateUpperDeck();
    void GenerateExteriorShell();
    void GenerateGameplayMarkers();

    float FeetToUU(float Feet) const;
    FVector PolarToField(float DistanceFeet, float AngleDegrees) const;
};
