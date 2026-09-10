#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_KillhouseGenerator.generated.h"

class UArrowComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Procedural open-top kill house generator inspired by a modular shoot-house layout.
 * Generates a single-story room maze with a central stair, elevated cross-catwalk,
 * observer rails, wall posts, and an exterior return stair during editor construction.
 */
UCLASS()
class THEUNIT_API ATU_KillhouseGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATU_KillhouseGenerator();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Killhouse")
    USceneComponent* Root;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "1800.0"))
    float BuildingWidth = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "2200.0"))
    float BuildingLength = 3600.0f;

    /** Height of the room walls in centimeters. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "240.0", ClampMax = "450.0"))
    float FloorHeight = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "10.0", ClampMax = "40.0"))
    float WallThickness = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Doors", meta = (ClampMin = "90.0", ClampMax = "180.0"))
    float DoorWidth = 115.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Doors", meta = (ClampMin = "190.0", ClampMax = "280.0"))
    float DoorHeight = 225.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Catwalk", meta = (ClampMin = "260.0", ClampMax = "500.0"))
    float CatwalkHeight = 330.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Catwalk", meta = (ClampMin = "160.0", ClampMax = "450.0"))
    float CatwalkWidth = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Catwalk", meta = (ClampMin = "8.0", ClampMax = "30.0"))
    float CatwalkThickness = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Catwalk", meta = (ClampMin = "80.0", ClampMax = "140.0"))
    float RailHeight = 105.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Catwalk", meta = (ClampMin = "3.0", ClampMax = "15.0"))
    float RailThickness = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Stairs", meta = (ClampMin = "160.0", ClampMax = "400.0"))
    float StairWidth = 240.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Stairs", meta = (ClampMin = "700.0", ClampMax = "1800.0"))
    float CenterStairRun = 1350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Stairs", meta = (ClampMin = "700.0", ClampMax = "1800.0"))
    float ExteriorStairRun = 1050.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Stairs", meta = (ClampMin = "8", ClampMax = "20"))
    int32 StairStepCount = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout")
    bool bGenerateCatwalk = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout")
    bool bGenerateExteriorStair = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout")
    bool bGenerateWallPosts = true;

    /** The reference layout is intentionally open-top; this remains optional for testing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout")
    bool bGenerateRoof = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Visuals")
    UMaterialInterface* WallMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Visuals")
    UMaterialInterface* FloorMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Visuals")
    UMaterialInterface* MetalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Debug")
    bool bGenerateDebugLabels = false;

private:
    UPROPERTY(Transient)
    UStaticMesh* CubeMesh;

    UPROPERTY(Transient)
    TArray<UActorComponent*> GeneratedComponents;

    int32 GeneratedNameCounter = 0;

    void BuildKillhouse();
    UStaticMeshComponent* AddCube(
        const FVector& Location,
        const FVector& Extents,
        const FString& BaseName,
        UMaterialInterface* Material = nullptr,
        const FRotator& Rotation = FRotator::ZeroRotator);
    UArrowComponent* AddMarker(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, const FString& BaseName);

    void GenerateFloor();
    void GeneratePerimeterWalls();
    void GenerateReferenceRoomLayout();
    void GenerateCenterStair();
    void GenerateCrossCatwalk();
    void GenerateExteriorStair();
    void GenerateWallPosts();

    void AddWallWithDoorX(float Y, float XMin, float XMax, float DoorCenterX, const FString& Prefix);
    void AddWallWithDoorY(float X, float YMin, float YMax, float DoorCenterY, const FString& Prefix);
    void GenerateGuardRailAlongX(float XMin, float XMax, float Y, float ZBase, const FString& Prefix);
    void GenerateGuardRailAlongY(float YMin, float YMax, float X, float ZBase, const FString& Prefix);

    void SpawnDebugLabel(const FString& Label, const FVector& Location);
    void ClearGeneratedComponents();
    FName MakeGeneratedName(const FString& BaseName);
};
