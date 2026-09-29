#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUWeaponPresentationComponent.h"
#include "TU_OperatorCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "TU_CommandCenterGenerator.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPresentationDecayTest, "TheUnit.Presentation.FrameRateDecay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPresentationDecayTest::RunTest(const FString& Parameters)
{
    const float Expected = UTUWeaponPresentationComponent::DecayOffset(4.f, 12.f, 1.f);
    for (const int32 FPS : {30, 60, 144})
    {
        float Offset = 4.f;
        for (int32 Frame = 0; Frame < FPS; ++Frame) Offset = UTUWeaponPresentationComponent::DecayOffset(Offset, 12.f, 1.f / FPS);
        TestTrue(FString::Printf(TEXT("%d Hz same one-second recovery"), FPS), FMath::IsNearlyEqual(Offset, Expected, 0.00001f));
    }
    TestEqual(TEXT("Negative time does not amplify recoil"), UTUWeaponPresentationComponent::DecayOffset(1.f, 12.f, -1.f), 1.f);

    const FVector VectorStart(-3.f, .8f, .4f);
    const FVector VectorExpected = UTUWeaponPresentationComponent::DecayVector(VectorStart, 15.f, 1.f);
    const FRotator RotationStart(4.f, -1.5f, .7f);
    const FRotator RotationExpected = UTUWeaponPresentationComponent::DecayRotation(RotationStart, 15.f, 1.f);
    for (const int32 FPS : {30, 60, 144})
    {
        FVector V = VectorStart;
        FRotator R = RotationStart;
        for (int32 Frame = 0; Frame < FPS; ++Frame)
        {
            V = UTUWeaponPresentationComponent::DecayVector(V, 15.f, 1.f / FPS);
            R = UTUWeaponPresentationComponent::DecayRotation(R, 15.f, 1.f / FPS);
        }
        TestTrue(FString::Printf(TEXT("%d Hz vector recovery is deterministic"), FPS), V.Equals(VectorExpected, .00001f));
        TestTrue(FString::Printf(TEXT("%d Hz rotation recovery is deterministic"), FPS),
            FMath::IsNearlyEqual(R.Pitch, RotationExpected.Pitch, .00001f) &&
            FMath::IsNearlyEqual(R.Yaw, RotationExpected.Yaw, .00001f) &&
            FMath::IsNearlyEqual(R.Roll, RotationExpected.Roll, .00001f));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPresentationStateTest, "TheUnit.Presentation.CommittedPhaseAndAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPresentationStateTest::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("Installed Epic rifle loads"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")));
    TestNotNull(TEXT("Installed Epic body loads"), LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
    TestNotNull(TEXT("Installed Epic handling pose loads"), LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload")));
    UTUWeaponPresentationComponent* Component = NewObject<UTUWeaponPresentationComponent>();
    TestFalse(TEXT("Nonessential camera recoil defaults off"), Component->bCameraRecoilEnabled);
    TestFalse(TEXT("Nonessential camera sway defaults off"), Component->bMovementSwayEnabled);
    FTUWeaponActionState State;
    State.ActionId = FGuid::NewGuid(); State.Revision = 3; State.Phase = ETUWeaponActionPhase::Inserted;
    Component->PresentPhase(State);
    TestEqual(TEXT("Observer snapshot chooses committed phase"), Component->PresentedPhase, ETUWeaponActionPhase::Inserted);
    State.Revision = 2; State.Phase = ETUWeaponActionPhase::Removed;
    Component->PresentPhase(State);
    TestEqual(TEXT("Stale phase does not rewind visual"), Component->PresentedPhase, ETUWeaponActionPhase::Inserted);
    State.Revision = 4; State.Phase = ETUWeaponActionPhase::Ready;
    Component->PresentPhase(State);
    TestEqual(TEXT("Newer reconciliation restores ready"), Component->PresentedPhase, ETUWeaponActionPhase::Ready);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUPresentationPOVTest, "TheUnit.Presentation.TemplatePOVClearance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUPresentationPOVTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("POV test world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ATU_OperatorCharacter* Operator = World->SpawnActor<ATU_OperatorCharacter>();
    ATU_WeaponBase* Weapon = World->SpawnActor<ATU_WeaponBase>();
    if (TestNotNull(TEXT("Operator"), Operator) && TestNotNull(TEXT("Canonical weapon"), Weapon))
    {
        Weapon->AttachToComponent(Operator->GetWorldWeaponAnchor(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        const FVector AimBefore = Weapon->GetActorForwardVector();
        const int32 AmmoBefore = Weapon->GetCurrentAmmo();
        Operator->GetWeaponPresentation()->InitializeForWeapon(Weapon);
        UStaticMeshComponent* Rifle = Weapon->GetWeaponBodyMesh();
        if (TestNotNull(TEXT("Actual template rifle mesh"), Rifle->GetStaticMesh().Get()))
        {
            const FVector BarrelAxis = Rifle->GetComponentTransform().TransformVectorNoScale(FVector::RightVector).GetSafeNormal();
            TestTrue(TEXT("Authored +Y barrel aligns with authoritative forward"), FVector::DotProduct(BarrelAxis, AimBefore) > .999f);
            const FVector Size = Rifle->GetStaticMesh()->GetBoundingBox().TransformBy(Rifle->GetRelativeTransform()).GetSize();
            TestTrue(TEXT("Real mesh is longitudinal, not broadside across sight"), Size.X > Size.Y * 2.f);
        }
        TestTrue(TEXT("Presentation does not rotate authoritative shot axis"), Weapon->GetActorForwardVector().Equals(AimBefore, .001f));
        TestEqual(TEXT("POV correction does not change ammunition"), Weapon->GetCurrentAmmo(), AmmoBefore);
        UCameraComponent* Camera = Operator->FindComponentByClass<UCameraComponent>();
        if (TestNotNull(TEXT("Actual first-person camera"), Camera))
        {
            const FVector Eye = Camera->GetRelativeLocation();
            const UCapsuleComponent* Capsule = Operator->GetCapsuleComponent();
            const float Radius = Capsule->GetUnscaledCapsuleRadius();
            const float CylinderHalf = Capsule->GetUnscaledCapsuleHalfHeight() - Radius;
            const float CapZ = FMath::Max(0.f, FMath::Abs(Eye.Z) - CylinderHalf);
            TestTrue(TEXT("Camera ahead of neck axis"), Eye.X >= 15.f);
            TestTrue(TEXT("Camera and full lean stay within capsule"), Eye.X * Eye.X + 8.f * 8.f + CapZ * CapZ < Radius * Radius);
        }
        FTUWeaponActionState Ready;
        Ready.ActionId = FGuid::NewGuid();
        Operator->GetWeaponPresentation()->PresentPhase(Ready);
        TestTrue(TEXT("Owner head hidden after pose initialization"), Operator->GetOwnerBodyMesh()->IsBoneHiddenByName(TEXT("head")));
        TestFalse(TEXT("Observer head remains present"), Operator->GetMesh()->IsBoneHiddenByName(TEXT("head")));
        TestNotNull(TEXT("Owner body retained"), Operator->GetOwnerBodyMesh()->GetSkeletalMeshAsset());
        TestFalse(TEXT("Canonical weapon remains visible"), Weapon->IsHidden());
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHQInteriorLightsTest, "TheUnit.Presentation.HQInteriorLights", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHQInteriorLightsTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Lighting fixture world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ATU_CommandCenterGenerator* First = World->SpawnActor<ATU_CommandCenterGenerator>();
    ATU_CommandCenterGenerator* Second = World->SpawnActor<ATU_CommandCenterGenerator>();
    if (TestNotNull(TEXT("First HQ"), First) && TestNotNull(TEXT("Independent HQ"), Second))
    {
        TInlineComponentArray<UPointLightComponent*> Lights(First);
        TInlineComponentArray<UPointLightComponent*> OtherLights(Second);
        TestEqual(TEXT("Real interior light coverage: seven corridor, nine room fixtures"), Lights.Num(), 16);
        TestEqual(TEXT("Independent layout produces same light count"), OtherLights.Num(), Lights.Num());
        for (const UPointLightComponent* Light : Lights)
        {
            TestTrue(TEXT("Light registered for rendering"), Light->IsRegistered());
            TestTrue(TEXT("Fixture below roof and above eye height"), Light->GetRelativeLocation().Z > 200.f && Light->GetRelativeLocation().Z < 300.f);
            TestTrue(TEXT("Real positive illumination with bounded radius"), Light->Intensity > 0.f && Light->AttenuationRadius <= 1200.f);
            TestTrue(TEXT("Walls occlude interior illumination"), Light->CastShadows != 0);
            const UPointLightComponent* const* Match = OtherLights.FindByPredicate([Light](const UPointLightComponent* Other) { return Other->GetFName() == Light->GetFName(); });
            TestTrue(TEXT("Peer construction has matching named light transform"), Match && (*Match)->GetRelativeTransform().Equals(Light->GetRelativeTransform(), .001f));
        }
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
#endif
