#if WITH_DEV_AUTOMATION_TESTS
#include "TUHandlingSight.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHandlingSightTransformTest, "TheUnit.Handling.SightTransform", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHandlingSightTransformTest::RunTest(const FString& Parameters)
{
    // Synthetic asymmetric calibration tests composition, not the production rifle calibration.
    FTUHandlingSightProfile Profile;
    Profile.EyeInActor = FTransform(FRotator(2.f, -3.f, 1.f), FVector(-18.f, 2.f, 11.f));
    Profile.SightInActor = FTransform(FQuat::Identity, FVector(20.f, 0.f, 0.f)) * Profile.EyeInActor;
    Profile.FrontSightInActor = Profile.EyeInActor.TransformPosition(FVector(65.f, 0.f, 0.f));
    for (const float Pitch : {-75.f, 0.f, 75.f})
    for (const float Lean : {-15.f, 0.f, 15.f})
    for (const float Height : {45.f, 80.f})
    {
        const FTransform Eye(FRotator(Pitch, 127.f, Lean), FVector(150.f, -230.f, Height));
        const FTransform Weapon = TUHandlingSight::SolveWeaponWorld(Eye, Profile);
        TestTrue(TEXT("Calibrated physical eye composes exactly"), (Profile.EyeInActor * Weapon).Equals(Eye, .001f));
        const FVector RearEyeSpace = Eye.InverseTransformPosition((Profile.SightInActor * Weapon).GetLocation());
        const FVector FrontEyeSpace = Eye.InverseTransformPosition(Weapon.TransformPosition(Profile.FrontSightInActor));
        TestTrue(TEXT("Rear sight stays on eye axis"), FMath::Abs(RearEyeSpace.Y) < .001f && FMath::Abs(RearEyeSpace.Z) < .001f && RearEyeSpace.X > 0.f);
        TestTrue(TEXT("Front sight stays on eye axis"), FMath::Abs(FrontEyeSpace.Y) < .001f && FMath::Abs(FrontEyeSpace.Z) < .001f && FrontEyeSpace.X > RearEyeSpace.X);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHandlingSightUnsupportedTest, "TheUnit.Handling.UnsupportedSight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHandlingSightUnsupportedTest::RunTest(const FString& Parameters)
{
    FTUHandlingSightProfile Profile;
    Profile.CalibrationId = TEXT("Stale");
    TestFalse(TEXT("Missing geometry has no claimed calibration"), TUHandlingSight::ResolveProfile(nullptr, Profile));
    TestTrue(TEXT("Unsupported resolution clears stale profile"), Profile.CalibrationId.IsNone());
    TestFalse(TEXT("Empty mesh component has no claimed calibration"), TUHandlingSight::ResolveProfile(NewObject<UStaticMeshComponent>(), Profile));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHandlingSightAssetTest, "TheUnit.Handling.MeasuredRifleSight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHandlingSightAssetTest::RunTest(const FString& Parameters)
{
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>();
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")));
    Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    FTUHandlingSightProfile Profile;
    if (!TestTrue(TEXT("Audited template rifle resolves"), TUHandlingSight::ResolveProfile(Mesh, Profile))) return false;
    const FVector Rear = Profile.SightInActor.GetLocation();
    TestTrue(TEXT("Measured aperture center accounts for mesh yaw -90"), Rear.Equals(FVector(1.296, .012, 16.096), .001f));
    TestTrue(TEXT("Actual central front post, not guard top"), Profile.FrontSightInActor.Equals(FVector(34.363831, -.001824, 16.717348), .001f));
    for (const float Fov : {65.f, 75.f, 90.f, 110.f})
    for (const float Pitch : {-70.f, 0.f, 70.f})
    {
        const FTransform Eye(FRotator(Pitch, 110.f, 12.f), FVector(50.f, -30.f, 45.f));
        const FTransform Weapon = TUHandlingSight::SolveWeaponWorld(Eye, Profile);
        for (const FVector Point : {Rear, Profile.FrontSightInActor})
        {
            const FVector EyeSpace = Eye.InverseTransformPosition(Weapon.TransformPosition(Point));
            const double ProjectionScale = 1. / FMath::Tan(FMath::DegreesToRadians(Fov * .5f));
            TestTrue(TEXT("Measured sight geometry projects to center across FOV and pitch"), EyeSpace.X > 0. && FMath::Abs(EyeSpace.Y / EyeSpace.X * ProjectionScale) < .00001 && FMath::Abs(EyeSpace.Z / EyeSpace.X * ProjectionScale) < .00001);
        }
    }
    return true;
}
#endif
