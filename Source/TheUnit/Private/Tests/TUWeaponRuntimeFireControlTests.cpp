#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "TUWeaponBuildResolver.h"
#include "TU_WeaponBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponRuntimeFireControlTest,
    "TheUnit.Combat.WeaponRuntimeFireControl",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponRuntimeFireControlTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        true, ERHIFeatureLevel::Num, &InitValues);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    ATU_WeaponBase* Weapon = World->SpawnActor<ATU_WeaponBase>();
    if (!TestNotNull(TEXT("Weapon actor"), Weapon))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestFalse(TEXT("Legacy weapon begins without modular fire control"), Weapon->HasActiveFireControl());

    FTUResolvedWeaponBuild Resolved;
    Resolved.DerivedWeaponDefinition = Weapon->GetWeaponDefinition();
    Resolved.bHasFireControl = true;
    Resolved.FireControlDefinition.FireControlId = TEXT("FC_TwoMode");
    Resolved.FireControlDefinition.TriggerProfileId = TEXT("Trigger.TwoStage");
    Resolved.FireControlDefinition.SupportedFireModes = { ETUFireMode::SemiAuto, ETUFireMode::Burst, ETUFireMode::Burst };
    Resolved.FireControlDefinition.BurstCount = 2;
    Resolved.FireControlDefinition.TriggerResponseMultiplier = 0.85f;
    Resolved.FireControlDefinition.ResetResponseMultiplier = 1.15f;
    Resolved.FireControlDefinition.SemiAutoResetDelaySeconds = 0.04f;
    Resolved.FireControlDefinition.bRequiresReleaseBetweenSemiShots = false;

    FString FailureReason;
    TestTrue(TEXT("Resolved modular build applies"), Weapon->ApplyResolvedBuild(Resolved, FailureReason));
    TestTrue(TEXT("Applied build has no failure reason"), FailureReason.IsEmpty());
    TestTrue(TEXT("Modular fire control becomes active"), Weapon->HasActiveFireControl());
    TestEqual(TEXT("Duplicate supported modes are sanitized"), Weapon->GetAvailableFireModes().Num(), 2);
    TestEqual(TEXT("Trigger profile comes from installed fire control"),
        Weapon->GetActiveTriggerProfileId(), FName(TEXT("Trigger.TwoStage")));
    TestEqual(TEXT("Burst count comes from installed fire control"), Weapon->GetConfiguredBurstCount(), 2);
    TestEqual(TEXT("Trigger response is exposed"), Weapon->GetTriggerResponseMultiplier(), 0.85f);
    TestEqual(TEXT("Reset response is exposed"), Weapon->GetResetResponseMultiplier(), 1.15f);
    TestEqual(TEXT("Semi reset delay is exposed"), Weapon->GetSemiAutoResetDelaySeconds(), 0.04f);
    TestFalse(TEXT("Release requirement is data driven"), Weapon->RequiresReleaseBetweenSemiShots());
    TestFalse(TEXT("Multi-mode modular definition is not marked semi-only"), Weapon->GetWeaponDefinition().bSemiAutoOnly);

    Weapon->SetFireMode(ETUFireMode::FullAuto);
    TestEqual(TEXT("Unsupported full-auto selection is rejected"),
        Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    Weapon->SetFireMode(ETUFireMode::Burst);
    TestEqual(TEXT("Supported burst selection succeeds"),
        Weapon->GetCurrentFireMode(), ETUFireMode::Burst);
    const int32 AmmoBeforeBurst = Weapon->GetCurrentAmmo();
    Weapon->StartFire();
    TestEqual(TEXT("Configured burst uses installed burst count"),
        Weapon->GetCurrentAmmo(), AmmoBeforeBurst - 2);

    FTUResolvedWeaponBuild SemiOnly = Resolved;
    SemiOnly.FireControlDefinition.FireControlId = TEXT("FC_SemiOnly");
    SemiOnly.FireControlDefinition.TriggerProfileId = TEXT("Trigger.Match");
    SemiOnly.FireControlDefinition.SupportedFireModes = { ETUFireMode::SemiAuto };
    TestTrue(TEXT("Semi-only build reapplies"), Weapon->ApplyResolvedBuild(SemiOnly, FailureReason));
    TestTrue(TEXT("Semi-only derived definition preserves compatibility flag"),
        Weapon->GetWeaponDefinition().bSemiAutoOnly);
    TestEqual(TEXT("Current mode falls back to supported semi mode"),
        Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);
    Weapon->SetFireMode(ETUFireMode::Burst);
    TestEqual(TEXT("Burst is unavailable after fire-control swap"),
        Weapon->GetCurrentFireMode(), ETUFireMode::SemiAuto);

    const FName StableTriggerProfile = Weapon->GetActiveTriggerProfileId();
    FTUResolvedWeaponBuild Invalid = SemiOnly;
    Invalid.FireControlDefinition.SupportedFireModes.Reset();
    TestFalse(TEXT("Fire control with no modes is rejected"), Weapon->ApplyResolvedBuild(Invalid, FailureReason));
    TestFalse(TEXT("Invalid apply reports a reason"), FailureReason.IsEmpty());
    TestEqual(TEXT("Rejected apply leaves active configuration unchanged"),
        Weapon->GetActiveTriggerProfileId(), StableTriggerProfile);

    FTUResolvedWeaponBuild MissingControl;
    MissingControl.DerivedWeaponDefinition = Weapon->GetWeaponDefinition();
    TestFalse(TEXT("Build without fire control is rejected for modular runtime configuration"),
        Weapon->ApplyResolvedBuild(MissingControl, FailureReason));

    World->DestroyWorld(false);
    return true;
}

#endif
