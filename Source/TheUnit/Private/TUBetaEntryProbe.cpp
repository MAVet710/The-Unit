#include "TUBetaEntryProbe.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "TU_PlayerController.h"
#include "STUBetaMenu.h"
#include "TUBetaUserSettings.h"
#include "TU_GameMode.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
// Only active in the explicit automated runner. Normal player input is unchanged.
namespace {
class FTUEntryInputGuard final : public IInputProcessor {
public:
    void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override {}
    bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent&) override {return true;}
    bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent&) override {return true;}
    bool HandleAnalogInputEvent(FSlateApplication&,const FAnalogInputEvent&) override {return true;}
    bool HandleMouseMoveEvent(FSlateApplication&,const FPointerEvent&) override {return true;}
    bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override {return true;}
    bool HandleMouseButtonUpEvent(FSlateApplication&,const FPointerEvent&) override {return true;}
    bool HandleMouseButtonDoubleClickEvent(FSlateApplication&,const FPointerEvent&) override {return true;}
    bool HandleMouseWheelOrGestureEvent(FSlateApplication&,const FPointerEvent&,const FPointerEvent*) override {return true;}
};
}
void UTUBetaEntryProbe::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
    if(!FParse::Value(FCommandLine::Get(),TEXT("TUEntrySmoke="),Id)) return;
    bool Safe=!Id.IsEmpty() && Id.Len()<=64;for(TCHAR C:Id) Safe&=FChar::IsAlnum(C)||C==TEXT('_')||C==TEXT('-');
    FString Slot;Safe&=FParse::Value(FCommandLine::Get(),TEXT("TUProfileSlot="),Slot)&&Slot.StartsWith(TEXT("TU_Automation_"));
    if(!Safe){FPlatformMisc::RequestExitWithStatus(false,2);return;}
    if(FSlateApplication::IsInitialized()){InputGuard=MakeShared<FTUEntryInputGuard>();FSlateApplication::Get().RegisterInputPreProcessor(InputGuard,0);}
    FParse::Value(FCommandLine::Get(),TEXT("TUEntryMode="),Mode);if(Mode.IsEmpty()) Mode=TEXT("roundtrip");
    Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/TUBetaEntry"),Id);
    if(IFileManager::Get().DirectoryExists(*Directory)){FPlatformMisc::RequestExitWithStatus(false,3);return;}
    IFileManager::Get().MakeDirectory(*Directory,true);Started=StageStarted=FPlatformTime::Seconds();
    Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UTUBetaEntryProbe::TickProbe));
#endif
}
void UTUBetaEntryProbe::Deinitialize(){if(InputGuard.IsValid()&&FSlateApplication::IsInitialized()){FSlateApplication::Get().UnregisterInputPreProcessor(InputGuard);InputGuard.Reset();}if(Handle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(Handle);Super::Deinitialize();}
void UTUBetaEntryProbe::Next(int32 Value){Stage=Value;StageStarted=FPlatformTime::Seconds();}
bool UTUBetaEntryProbe::Require(bool OK,const FString& Why){if(!OK) Finish(false,Why);return OK;}
void UTUBetaEntryProbe::Capture(const FString& Name)
{
    const FString Path=FPaths::Combine(Directory,Name+TEXT(".png"));
    Evidence.Add(Path);PendingCapture=Path;PendingCaptureAt=FPlatformTime::Seconds()+.75;
    // Slate's native window and the RHI backbuffer must agree after a resize.
    // Requesting a UI screenshot on the resize tick can address the old rectangle.
}
void UTUBetaEntryProbe::Finish(bool Passed,const FString& Reason)
{
    if(bFinished)return;bFinished=true;
    FString Safe=Reason.Replace(TEXT("\\"),TEXT("/")).Replace(TEXT("\""),TEXT("'"));
    const FString Json=FString::Printf(TEXT("{\"passed\":%s,\"mode\":\"%s\",\"stage\":%d,\"reason\":\"%s\",\"scripted_menu_callbacks\":true,\"manual_playtest\":false,\"screenshots\":%d}"),Passed?TEXT("true"):TEXT("false"),*Mode,Stage,*Safe,Evidence.Num());
    const bool Written=FFileHelper::SaveStringToFile(Json,*FPaths::Combine(Directory,TEXT("result.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("TUENTRY_RESULT %s DIRECTORY=%s"),*Json,*Directory);
    if(!Passed||!Written) FPlatformMisc::RequestExitWithStatus(false,1);
}
bool UTUBetaEntryProbe::CheckPreferences(ATU_PlayerController* PC)
{
    auto* S=UTUBetaUserSettings::Get();
    return Require(S && FMath::IsNearlyEqual(S->MasterVolume,.37f,.001f)&&FMath::IsNearlyEqual(S->MouseSensitivity,1.35f,.001f)&&FMath::IsNearlyEqual(S->FieldOfView,96.f,.001f)&&S->bInvertVertical&&!S->bCameraSwayEnabled&&FMath::IsNearlyEqual(S->GetFrameRateLimit(),60.f,.01f),TEXT("Saved preference values mismatch"))
        && Require(S->EffectiveKey(TEXT("Reload"))==EKeys::J && PC->PlayerInput && PC->PlayerInput->ActionMappings.ContainsByPredicate([](const FInputActionKeyMapping& K){return K.ActionName==TEXT("Reload")&&K.Key==EKeys::J;}),TEXT("Saved reload binding was not applied to player input"));
}
bool UTUBetaEntryProbe::TickProbe(float)
{
    if(bFinished)return false;
    if(GFrameCounter<30)return true; // Never drive UI during nested engine startup ticks.
    const double Now=FPlatformTime::Seconds();
    if(!bReadyToRun){bReadyToRun=true;Started=StageStarted=Now;return true;}
    if(!PendingCapture.IsEmpty()) {
        if(Now<PendingCaptureAt)return true;
        FScreenshotRequest::RequestScreenshot(PendingCapture,true,false);
        PendingCapture.Empty();StageStarted=Now;return true;
    }
    if(Now-Started>140.){Finish(false,TEXT("Menu flow deadline"));return false;}
    UWorld* World=GetGameInstance()->GetWorld();auto* PC=World?Cast<ATU_PlayerController>(UGameplayStatics::GetPlayerController(World,0)):nullptr;
    if(!PC||Now-StageStarted<1.)return true;
    const FString Map=UGameplayStatics::GetCurrentLevelName(World,true);auto Menu=PC->GetFrontEndMenu();
    auto* Life=GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>();auto* S=UTUBetaUserSettings::Get();
    if(!Require(Life&&Life->GetProfile()&&S,TEXT("Missing profile or settings service")))return false;
    if(Mode==TEXT("verify")||Mode==TEXT("verify-abandon")) {
        if(!Menu.IsValid()||!PC->IsFrontEndWorld())return true;
        if(Stage==0){
            if(Mode==TEXT("verify")){int32 X=0,Y=0;PC->GetViewportSize(X,Y);UE_LOG(LogTemp,Display,TEXT("TUENTRY_DISPLAY_RESTART saved=%dx%d viewport=%dx%d mode=%d"),S->GetScreenResolution().X,S->GetScreenResolution().Y,X,Y,int32(S->GetFullscreenMode()));if(!CheckPreferences(PC)||!Require(S->GetScreenResolution()==FIntPoint(1600,900)&&X==1600&&Y==900,TEXT("Confirmed display mode did not persist to actual restarted viewport")))return false;}
            if(Mode==TEXT("verify-abandon")&&!Require(Life->GetProfile()->Outcomes.ContainsByPredicate([](const FTURaidOutcome& O){return O.Outcome==ETURaidPlayerOutcome::Abandoned&&!O.bTraining;})&&Life->GetProfile()->Stash.Weapons.IsEmpty(),TEXT("Abandoned live kit did not remain lost after restart")))return false;
            Capture(TEXT("restart_verified"));Next(1);return true;
        }
        Finish(true,TEXT("Separate process preferences or abandoned outcome verified"));Menu->Command(TEXT("quit"));Menu->Command(TEXT("confirmexit"));return false;
    }
    if(Mode==TEXT("abandon")) {
        if(Stage==0){
            auto* GM=World->GetAuthGameMode<ATU_GameMode>();if(!GM||!PC->GetPawn()||!GM->FindParticipant(PC->GetPawn()))return true;
            PC->OpenFrontEndMenu();if(!Require(PC->IsLiveRaidWorld()&&!PC->IsPaused(),TEXT("Live raid menu paused time")))return false;
            FrozenTime=World->GetTimeSeconds();Capture(TEXT("live_menu"));Next(1);return true;
        }
        if(Stage==1){
            if(!Require(World->GetTimeSeconds()>FrozenTime+.5f&&Menu.IsValid(),TEXT("Live raid timer did not advance with menu")))return false;
            Menu->Command(TEXT("leave"));Menu->Command(TEXT("cancel"));
            auto* GM=World->GetAuthGameMode<ATU_GameMode>();const auto* Part=PC->GetPawn()?GM->FindParticipant(PC->GetPawn()):nullptr;
            if(!Require(Part&&Part->Outcome==ETURaidPlayerOutcome::Active,TEXT("Cancel changed player outcome")))return false;
            Life->InjectSaveFailure(1);Menu->Command(TEXT("leave"));Menu->Command(TEXT("confirmexit"));
            if(!Require(!PC->IsFrontEndWorld()&&!PC->IsFrontEndExitPending()&&!Menu->LastMessage().IsEmpty(),TEXT("Failed save allowed exit or hid the error")))return false;
            Capture(TEXT("save_failure_stayed"));Next(2);return true;
        }
        if(Stage==2){Life->InjectSaveFailure(0);PC->RequestConfirmedFrontEndExit(false);Next(3);return true;}
        if(Stage==3&&PC->IsFrontEndWorld()&&Menu.IsValid()){
            if(!Require(Life->GetProfile()->Outcomes.ContainsByPredicate([](const FTURaidOutcome& O){return O.Outcome==ETURaidPlayerOutcome::Abandoned&&!O.bTraining;})&&Life->GetProfile()->Stash.Weapons.IsEmpty(),TEXT("Exit did not durably abandon live kit")))return false;
            Capture(TEXT("abandoned_return"));Next(4);return true;
        }
        if(Stage==4&&Menu.IsValid()){Finish(true,TEXT("Cancel preserved live raid; save failure blocked exit; retry abandoned with no extraction reward"));Menu->Command(TEXT("quit"));Menu->Command(TEXT("confirmexit"));return false;}
        return true;
    }
    if(Stage==0){
        if(!PC->IsFrontEndWorld()||!Menu.IsValid())return true;
        if(!Require(!PC->GetPawn(),TEXT("Main menu spawned a playable kit")))return false;
        Capture(TEXT("main_menu"));S->MasterVolume=.37f;S->MouseSensitivity=1.35f;S->FieldOfView=96.f;S->SetFrameRateLimit(60.f);S->bInvertVertical=true;S->bCameraSwayEnabled=false;FString Error;
        if(!Require(S->CommitPreferences(PC,Error),Error))return false;
        Next(1);return true;
    }
    if(Stage==1){Menu->Command(TEXT("settings"));Capture(TEXT("settings"));Next(2);return true;}
    if(Stage==2){
        PreviousResolution=S->GetScreenResolution();Menu->PreviewDisplay(FIntPoint(1600,900),EWindowMode::Windowed);
        if(!Require(Menu->PageName()==TEXT("video"),TEXT("Display preview did not require confirmation")))return false;
        Capture(TEXT("display_confirmation"));Next(3);return true;
    }
    if(Stage==3){
        if(Now-StageStarted<16.5)return true;
        if(!Require(Menu->PageName()==TEXT("settings")&&S->GetScreenResolution()==PreviousResolution,FString::Printf(TEXT("Unconfirmed display change did not revert: page=%s saved=%dx%d expected=%dx%d"),*Menu->PageName(),S->GetScreenResolution().X,S->GetScreenResolution().Y,PreviousResolution.X,PreviousResolution.Y)))return false;
        Menu->Command(TEXT("controls"));Menu->BeginBindingCapture(TEXT("Reload"));
        Menu->OnKeyDown(FGeometry(),FKeyEvent(EKeys::W,FModifierKeysState(),0,false,0,0));
        if(!Require(S->EffectiveKey(TEXT("Reload"))==EKeys::R,TEXT("Conflicting binding replaced reload")))return false;
        Menu->OnKeyDown(FGeometry(),FKeyEvent(EKeys::J,FModifierKeysState(),0,false,0,0));
        if(!CheckPreferences(PC))return false;Capture(TEXT("controls_rebound"));Next(31);return true;
    }
    if(Stage==31){
        FString Error;S->MasterVolume=1.f;if(!Require(S->CommitPreferences(PC,Error),Error))return false;
        S->MasterVolume=.37f;if(!Require(S->CommitPreferences(PC,Error),Error))return false;
        Menu->PreviewDisplay(FIntPoint(1600,900),EWindowMode::Windowed);Capture(TEXT("display_keep_prompt"));Next(32);return true;
    }
    if(Stage==32){int32 X=0,Y=0;PC->GetViewportSize(X,Y);if(!Require(X==1600&&Y==900,TEXT("Display preview did not change the actual viewport")))return false;Menu->Command(TEXT("keepvideo"));Next(4);return true;}
    if(Stage==4){
        Menu->Command(TEXT("controls"));Menu->Command(TEXT("resetkeys"));if(!Require(S->EffectiveKey(TEXT("Reload"))==EKeys::R,TEXT("Default binding reset failed")))return false;
        Menu->BeginBindingCapture(TEXT("Reload"));Menu->OnKeyDown(FGeometry(),FKeyEvent(EKeys::J,FModifierKeysState(),0,false,0,0));
        if(!CheckPreferences(PC))return false;
        Menu->Command(TEXT("back"));Menu->Command(TEXT("training"));Next(5);return true;
    }
    if(Stage==5&&Map==TEXT("Killhouse")&&PC->GetPawn()){
        if(!CheckPreferences(PC))return false;PC->OpenFrontEndMenu();if(!Require(PC->IsPaused(),TEXT("Local training did not pause")))return false;
        FrozenTime=World->GetTimeSeconds();Capture(TEXT("training_menu"));Next(6);return true;
    }
    if(Stage==6){
        if(!Require(FMath::IsNearlyEqual(World->GetTimeSeconds(),FrozenTime,.02f),TEXT("Training advanced while paused")))return false;
        Menu->OnKeyDown(FGeometry(),FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
        if(!Require(!PC->GetFrontEndMenu().IsValid()&&!PC->IsPaused()&&!PC->IsMoveInputIgnored()&&!PC->IsLookInputIgnored(),TEXT("Resume did not restore normal input")))return false;
        Capture(TEXT("training_resumed"));Next(7);return true;
    }
    if(Stage==7){PC->OpenFrontEndMenu();Menu=PC->GetFrontEndMenu();Menu->Command(TEXT("leave"));Capture(TEXT("training_exit_confirmation"));Next(8);return true;}
    if(Stage==8){Menu->Command(TEXT("cancel"));if(!Require(Map==TEXT("Killhouse"),TEXT("Cancel caused travel")))return false;Menu->Command(TEXT("leave"));Menu->Command(TEXT("confirmexit"));Next(9);return true;}
    if(Stage==9&&PC->IsFrontEndWorld()&&Menu.IsValid()){Menu->Command(TEXT("hq"));Next(10);return true;}
    if(Stage==10&&Map==TEXT("CommandCenter")&&PC->GetPawn()){
        PC->OpenFrontEndMenu();Menu=PC->GetFrontEndMenu();Capture(TEXT("headquarters_menu"));Next(11);return true;
    }
    if(Stage==11){Menu->Command(TEXT("leave"));Menu->Command(TEXT("confirmexit"));Next(12);return true;}
    if(Stage==12&&PC->IsFrontEndWorld()&&Menu.IsValid()){
        if(!CheckPreferences(PC))return false;
        for(const auto& Path:Evidence)if(!Require(IFileManager::Get().FileSize(*Path)>0,TEXT("Missing requested UI capture: ")+Path))return false;
        Finish(true,TEXT("Main menu, persisted preferences, binding conflict and remap, display timeout rollback, training pause/resume, HQ and confirmed return passed"));
        Menu->Command(TEXT("quit"));Menu->Command(TEXT("confirmexit"));return false;
    }
    return true;
}
