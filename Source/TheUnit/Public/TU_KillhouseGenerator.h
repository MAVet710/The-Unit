#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TU_KillhouseGenerator.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Procedural Phase 1D.1 graybox kill house generator.
 * Generates a compact 2-story CQB structure during editor construction.
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "1200.0"))
    float BuildingWidth = 2400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "1200.0"))
    float BuildingLength = 3200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "220.0"))
    float FloorHeight = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Dimensions", meta = (ClampMin = "10.0"))
    float WallThickness = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout", meta = (ClampMin = "3", ClampMax = "8"))
    int32 FirstFloorRoomCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout", meta = (ClampMin = "3", ClampMax = "8"))
    int32 SecondFloorRoomCount = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout", meta = (ClampMin = "180.0", ClampMax = "600.0"))
    float HallwayWidth = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout", meta = (ClampMin = "100.0", ClampMax = "400.0"))
    float StairWidth = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Layout")
    bool bGenerateRoof = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Killhouse|Debug")
    bool bGenerateDebugLabels = true;

private:
    UPROPERTY(Transient)
    UStaticMesh* CubeMesh;

    UPROPERTY(Transient)
    TArray<UActorComponent*> GeneratedComponents;

    void BuildKillhouse();
    UStaticMeshComponent* AddCube(const FVector& Location, const FVector& Extents, const FName& Name, const FRotator& Rotation = FRotator::ZeroRotator);
    UArrowComponent* AddMarker(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, const FName& Name);

    void GenerateFloor(float ZOffset);
    void GenerateWalls(float ZOffset);
    void GenerateRoom(const FVector& Center, const FVector2D& Size, float ZBase, const FString& RoomName, bool bLabel, bool bDoorToHallway, bool bDoorAtPositiveX);
    void GenerateStairs(float ZBase);
    void GenerateSecondFloor(float ZOffset);
    void SpawnDebugLabel(const FString& Label, const FVector& Location);
    void ClearGeneratedComponents();
};
