#include "Misc/AutomationTest.h"
#include "TUBetaUserSettings.h"
#include "TU_PlayerController.h"
#include "TUFrontEndGameMode.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUBetaPreferencesBounds,"TheUnit.BetaEntry.PreferenceBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUBetaPreferencesBounds::RunTest(const FString&)
{
    auto* S=NewObject<UTUBetaUserSettings>();
    S->MasterVolume=-4.f;S->MouseSensitivity=100.f;S->FieldOfView=1000.f;S->ValidateSettings();
    TestEqual(TEXT("Volume bounded"),S->MasterVolume,0.f);TestEqual(TEXT("Sensitivity bounded"),S->MouseSensitivity,3.f);TestEqual(TEXT("FOV bounded"),S->FieldOfView,110.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUBetaBindingRules,"TheUnit.BetaEntry.BindingIntegrity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUBetaBindingRules::RunTest(const FString&)
{
    auto* S=NewObject<UTUBetaUserSettings>();S->KeyOverrides.Reset();FString Error;
    TestFalse(TEXT("Duplicate movement binding rejected"),S->SetBinding(TEXT("Reload"),EKeys::W,Error));
    TestFalse(TEXT("Escape cannot be stolen"),S->SetBinding(TEXT("Reload"),EKeys::Escape,Error));
    TestFalse(TEXT("Unknown action rejected"),S->SetBinding(TEXT("Invented"),EKeys::J,Error));
    TestTrue(TEXT("Valid reload binding accepted"),S->SetBinding(TEXT("Reload"),EKeys::J,Error));
    TestEqual(TEXT("Override effective"),S->EffectiveKey(TEXT("Reload")),EKeys::J);
    S->KeyOverrides.Add(TEXT("Fire"),EKeys::J);S->ValidateSettings();
    TestEqual(TEXT("Corrupt duplicate overrides reset"),S->KeyOverrides.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUBetaPauseRules,"TheUnit.BetaEntry.PausePolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUBetaPauseRules::RunTest(const FString&)
{
    TestTrue(TEXT("Solo safe area pauses"),ATU_PlayerController::ShouldPauseMenu(NM_Standalone,false));
    TestFalse(TEXT("Live solo raid keeps time"),ATU_PlayerController::ShouldPauseMenu(NM_Standalone,true));
    for(ENetMode M:{NM_Client,NM_ListenServer,NM_DedicatedServer}) {
        TestFalse(TEXT("No shared session paused"),ATU_PlayerController::ShouldPauseMenu(M,false));
        TestFalse(TEXT("No shared raid paused"),ATU_PlayerController::ShouldPauseMenu(M,true));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUBetaMenuNoKit,"TheUnit.BetaEntry.MainMenuHasNoOperator",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUBetaMenuNoKit::RunTest(const FString&)
{
    const auto* Mode=GetDefault<ATUFrontEndGameMode>();
    TestNull(TEXT("Menu cannot mint default kit by spawning operator"),Mode->DefaultPawnClass.Get());return true;
}
#endif
