#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUMX50TabletComponent.generated.h"

UENUM(BlueprintType)
enum class ETUMX50Page : uint8
{
    Mission UMETA(DisplayName="Mission"),
    Map UMETA(DisplayName="Map"),
    Intel UMETA(DisplayName="Intel"),
    Team UMETA(DisplayName="Team"),
    Drone UMETA(DisplayName="Drone / Video"),
    Loadout UMETA(DisplayName="Loadout / Status")
};

USTRUCT(BlueprintType)
struct FTMX50MissionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MissionId = TEXT("OP_KILLHOUSE");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText MissionTitle = FText::FromString(TEXT("Kill House Evaluation"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Objective = FText::FromString(TEXT("Clear the training structure and complete the assigned evaluation."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Area = FText::FromString(TEXT("Command Training Facility"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ThreatSummary = FText::FromString(TEXT("Training threat package. Live mission intelligence plugs into this field later."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText TeamSummary = FText::FromString(TEXT("Local operator. Co-op roster and readiness integrate here later."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDroneFeedAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDeploymentAuthorized = false;
};

/**
 * Persistent state for the operator-worn MX50 tactical tablet.
 * The component deliberately contains mission/UI state only; final art, animation,
 * networking and mission-service integration can replace the prototype data without
 * changing the chest-mounted tablet interaction contract.
 */
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUMX50TabletComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTUMX50TabletComponent();

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetMissionContext(FName MissionId, const FText& MissionTitle);

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetMissionSnapshot(const FTMX50MissionSnapshot& InSnapshot);

    UFUNCTION(BlueprintPure, Category="MX50")
    FTMX50MissionSnapshot GetMissionSnapshot() const { return MissionSnapshot; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetActivePage(ETUMX50Page Page) { ActivePage = Page; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    ETUMX50Page CyclePage(int32 Direction = 1);

    UFUNCTION(BlueprintPure, Category="MX50")
    ETUMX50Page GetActivePage() const { return ActivePage; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetDroneFeedAvailable(bool bAvailable) { MissionSnapshot.bDroneFeedAvailable = bAvailable; }

    UFUNCTION(BlueprintCallable, Category="MX50")
    void SetDeploymentAuthorized(bool bAuthorized) { MissionSnapshot.bDeploymentAuthorized = bAuthorized; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MX50")
    FTMX50MissionSnapshot MissionSnapshot;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MX50")
    ETUMX50Page ActivePage = ETUMX50Page::Mission;
};
