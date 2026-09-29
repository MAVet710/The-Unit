#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUExecutionTypes.h"
#include "TU_WeaponBase.h"
#include "TUWeaponPartsComponent.generated.h"
class ATU_WeaponBase;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
struct FTUWeaponShotResult;
/** Pure presentation snapshot. It never owns or transfers an inventory item. */
struct THEUNIT_API FTUWeaponPartsFrame
{
    FGuid InsertedId;
    FGuid MovingId;
    FTransform MovingInActor = FTransform::Identity;
    FVector HandOffset = FVector::ZeroVector;
    float HandContactAlpha = 0.f;
    FTransform HandInActor = FTransform::Identity;
    float MagazineGripAlpha = 0.f;
    float ControlGripAlpha = 0.f;
    float ControlPress = 0.f;
    float ActionTravelCm = 0.f;
    float SeatReaction = 0.f;
};
UCLASS(ClassGroup=(TheUnit))
class THEUNIT_API UTUWeaponPartsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTUWeaponPartsComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void UpdatePresentation(float DeltaSeconds);
    static FTUWeaponPartsFrame Evaluate(const FTUItemLedger& Ledger,
        const FTUWeaponActionState& Action, float PhaseProgress);
    bool IsSupported() const { return bSupported; }
    FTransform GetVisibleMeshWorld() const;
    FTransform GetVisibleActorWorld() const;
    const FTUWeaponPartsFrame& GetFrame() const { return Frame; }
    UStaticMeshComponent* GetInsertedVisual() const { return InsertedVisual; }
    UStaticMeshComponent* GetMovingVisual() const { return MovingVisual; }
    float GetWeaponKickCm() const { return WeaponKickCm; }
    UStaticMeshComponent* GetActionVisual() const { return ActionVisual; }
    UStaticMeshComponent* GetReceiverVisual() const { return ReceiverVisual; }
    UStaticMeshComponent* GetControlVisual() const { return ControlVisual; }
    /** Translation/pitch affect visual geometry only, not authority traces. */
    UPROPERTY(EditAnywhere, Category="Presentation") bool bWeaponRecoilEnabled = true;
private:
    bool Configure();
    UFUNCTION() void OnShot(FTUWeaponShotResult Result);
    UPROPERTY() TObjectPtr<ATU_WeaponBase> Weapon;
    UPROPERTY() TObjectPtr<UStaticMesh> ReceiverAsset;
    UPROPERTY() TObjectPtr<UStaticMesh> MagazineAsset;
    UPROPERTY() TObjectPtr<UStaticMesh> ActionAsset;
    UPROPERTY() TObjectPtr<UStaticMesh> ControlAsset;
    UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ReceiverVisual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> InsertedVisual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MovingVisual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ActionVisual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ControlVisual;
    bool bSupported = false;
    float WeaponKickCm = 0.f;
    float SupportLoadRoll = 0.f;
    FTUWeaponPartsFrame Frame;
    FGuid LastActionId;
    ETUWeaponActionPhase LastPhase = ETUWeaponActionPhase::Ready;
    float LastProgress = 0.f;
};
