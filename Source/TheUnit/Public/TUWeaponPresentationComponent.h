#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TUExecutionTypes.h"
#include "TU_WeaponBase.h"
#include "TUWeaponPresentationComponent.generated.h"
class USoundBase;
class UStaticMesh;
class UAnimSequence;
UCLASS(ClassGroup=(TheUnit), meta=(BlueprintSpawnableComponent))
class THEUNIT_API UTUWeaponPresentationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTUWeaponPresentationComponent();
    UFUNCTION(BlueprintCallable) void InitializeForWeapon(ATU_WeaponBase* Weapon);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintCallable) void PresentPhase(const FTUWeaponActionState& State);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accessibility") bool bCameraRecoilEnabled = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accessibility") bool bMovementSwayEnabled = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") float RecoilRecoveryRate = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") float RecoilDegrees = 0.7f;
    // Distinct licensed sound slots. Unassigned slots intentionally stay silent.
    UPROPERTY(EditAnywhere, Category="Audio") TObjectPtr<USoundBase> MagazineReleaseSound;
    UPROPERTY(EditAnywhere, Category="Audio") TObjectPtr<USoundBase> MagazineAcquireSound;
    UPROPERTY(EditAnywhere, Category="Audio") TObjectPtr<USoundBase> MagazineInsertSound;
    UPROPERTY(EditAnywhere, Category="Audio") TObjectPtr<USoundBase> ActionCycleSound;
    UPROPERTY(EditAnywhere, Category="Audio") TObjectPtr<USoundBase> ShotSound;
    UPROPERTY(BlueprintReadOnly) ETUWeaponActionPhase PresentedPhase = ETUWeaponActionPhase::Ready;
    UPROPERTY(BlueprintReadOnly) bool bActionInProgress = false;
    float GetCameraRecoilOffset() const { return bCameraRecoilEnabled ? CameraRecoilRotation.Pitch : 0.f; }
    FRotator GetCameraRecoilRotation() const { return bCameraRecoilEnabled ? CameraRecoilRotation : FRotator::ZeroRotator; }
    FTransform GetWeaponPresentationOffset() const;
    float GetSwayOffset() const { return bMovementSwayEnabled ? SwayOffset : 0.f; }
    static float DecayOffset(float Offset, float Rate, float DeltaTime);
    static FVector DecayVector(const FVector& Offset, float Rate, float DeltaTime);
    static FRotator DecayRotation(const FRotator& Offset, float Rate, float DeltaTime);
    ATU_WeaponBase* GetActiveWeapon() const { return ActiveWeapon; }
    float GetEvaluatedAnimationTime() const { return EvaluatedAnimationTime; }
    FVector GetRightHandGripWorld() const { return RightGripWorld.GetLocation(); }
    FVector GetLeftHandGripWorld() const { return LeftGripWorld.GetLocation(); }
    bool HasCalibratedHandContacts() const { return bCalibratedContacts; }
    static float PhaseAnimationFraction(ETUWeaponActionPhase Phase, float Remaining, float Duration);
private:
    void UpdateEvaluatedAnimation(float DeltaTime);
    FTransform RightGripWorld = FTransform::Identity;
    FTransform LeftGripWorld = FTransform::Identity;
    float EvaluatedAnimationTime = 0.f;
    float ReloadWeight = 0.f;
    bool bCalibratedContacts = false;
    UFUNCTION() void HandleShot(FTUWeaponShotResult Result);
    UPROPERTY() TObjectPtr<ATU_WeaponBase> ActiveWeapon;
    UPROPERTY() TObjectPtr<UStaticMesh> PrototypeRifle;
    UPROPERTY() TObjectPtr<UAnimSequence> PrototypeReload;
    UPROPERTY() TObjectPtr<UAnimSequence> PrototypeIdle;
    FGuid LastAction;
    int32 LastRevision = INDEX_NONE;
    FVector WeaponRecoilLocation = FVector::ZeroVector;
    FRotator WeaponRecoilRotation = FRotator::ZeroRotator;
    FRotator CameraRecoilRotation = FRotator::ZeroRotator;
    FVector WeaponSwayLocation = FVector::ZeroVector;
    FRotator WeaponSwayRotation = FRotator::ZeroRotator;
    float SwayOffset = 0.f;
    int32 ShotSequence = 0;
    double MotionTime = 0.;
};
