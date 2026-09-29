#include "TU_PlayerController.h"
#include "STUBetaMenu.h"
#include "TUBetaUserSettings.h"
#include "TUFrontEndGameMode.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TU_GameMode.h"
#include "TU_GameState.h"
#include "TU_PlayerState.h"
#include "TUHideoutLifecycleSubsystem.h"
#include "TUHideoutSaveGame.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

void ATU_PlayerController::BeginPlay()
{
    Super::BeginPlay();
    if(IsLocalController()) GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]{
        if(auto* Settings=UTUBetaUserSettings::Get()) {Settings->ApplyStartupDisplay();Settings->ApplyToController(this);}
        if(IsFrontEndWorld()) OpenFrontEndMenu();
    }));
}
void ATU_PlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if(IsLocalController()||MenuWidget.IsValid()) CloseFrontEndMenu(true);
    Super::EndPlay(Reason);
}
bool ATU_PlayerController::IsFrontEndWorld() const
{
    return UGameplayStatics::GetCurrentLevelName(this,true)==TEXT("MainMenu") || (GetWorld() && GetWorld()->GetAuthGameMode<ATUFrontEndGameMode>());
}
bool ATU_PlayerController::IsLiveRaidWorld() const
{
    if(IsFrontEndWorld() || UGameplayStatics::GetCurrentLevelName(this,true)==TEXT("CommandCenter")) return false;
    const auto* State=GetWorld()?GetWorld()->GetGameState<ATU_GameState>():nullptr;
    return !State || !State->bTraining;
}
bool ATU_PlayerController::ShouldPauseMenu(ENetMode Mode,bool bLive)
{
    return Mode==NM_Standalone && !bLive;
}
FString ATU_PlayerController::MenuStatusText() const
{
    return bPausedByFrontEnd?TEXT("SESSION PAUSED  /  Local training or headquarters"):
        TEXT("SESSION CONTINUES  /  Time and threats are still active");
}
FString ATU_PlayerController::ExitWarningText() const
{
    if(IsFrontEndWorld()) return TEXT("Your saved preferences and profile will be kept.");
    if(GetNetMode()==NM_ListenServer) return TEXT("You are the host. Leaving ends this session for connected players. Unresolved live deployments are abandoned and their deployed gear is lost. This is not extraction. Existing successful outcomes are preserved.");
    if(IsLiveRaidWorld()) return TEXT("Leaving abandons your unresolved raid. Deployed gear and carried raid loot are lost according to the existing outcome rules. This does not count as extraction. Cancel to stay.");
    if(GetNetMode()==NM_Client) return TEXT("You will leave the shared session. The server must save your outcome before returning you to the menu. Other players remain in their session.");
    return TEXT("Your existing live profile is preserved. Training does not award raid loot. Any pending training session is closed without live rewards.");
}
void ATU_PlayerController::ToggleFrontEndMenu()
{
    if(MenuWidget.IsValid()) {if(!IsFrontEndWorld()) CloseFrontEndMenu();}
    else OpenFrontEndMenu();
}
void ATU_PlayerController::OpenFrontEndMenu()
{
    if(!IsLocalController() || MenuWidget.IsValid() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    if(auto* Op=Cast<ATU_ArmedOperatorCharacter>(GetPawn())) {
        if(Op->IsArmoryOpen()) Op->CloseArmory();
        if(Op->IsBriefingOpen()) Op->CloseBriefing();
        Op->SuspendInputForMenu();
        Op->DisableInput(this);
    }
    if(PlayerInput) PlayerInput->FlushPressedKeys();
    SetIgnoreMoveInput(true);SetIgnoreLookInput(true);bFrontEndOwnsInput=true;
    if(ShouldPauseMenu(GetNetMode(),IsLiveRaidWorld()) && !IsPaused()) bPausedByFrontEnd=SetPause(true);
    MenuWidget=SNew(STUBetaMenu).Owner(this);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(),100);
    bShowMouseCursor=true;
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(MenuWidget);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
}
void ATU_PlayerController::CloseFrontEndMenu(bool bForce)
{
    if(IsFrontEndWorld() && !bForce) return;
    if(MenuWidget.IsValid()) {
        MenuWidget->CancelPendingDisplay();
        if(GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
        MenuWidget.Reset();
    }
    if(bPausedByFrontEnd) {SetPause(false);bPausedByFrontEnd=false;}
    if(bFrontEndOwnsInput) {
        SetIgnoreMoveInput(false);SetIgnoreLookInput(false);bFrontEndOwnsInput=false;
        if(GetPawn()) GetPawn()->EnableInput(this);
        if(PlayerInput) PlayerInput->FlushPressedKeys();
    }
    bShowMouseCursor=false;
    if(IsLocalController()){FInputModeGameOnly Mode;SetInputMode(Mode);}
}
void ATU_PlayerController::FrontEndError(const FString& Error)
{
    bFrontEndExitPending=false;
    if(MenuWidget.IsValid()) MenuWidget->ShowError(Error);
}
void ATU_PlayerController::EnterFromFrontEnd(bool bTraining)
{
    if(!IsLocalController() || !IsFrontEndWorld()) return;
    auto* Life=GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>();
    if(!Life || !Life->GetProfile() || Life->IsRecoveryBlocked()) {FrontEndError(TEXT("Profile recovery requires attention. Your existing saves have not been replaced."));return;}
    for(const auto& D:Life->GetProfile()->Deployments) if(!D.bResolved) {FrontEndError(TEXT("An earlier deployment is still unresolved. Retry after recovery completes."));return;}
    CloseFrontEndMenu(true);
    UGameplayStatics::OpenLevel(this,bTraining?FName(TEXT("/Game/TheUnit/Maps/Killhouse")):FName(TEXT("/Game/TheUnit/Maps/CommandCenter")));
}
bool ATU_PlayerController::HostCoopFromFrontEnd()
{
    if(!IsLocalController() || !IsFrontEndWorld() || !GetWorld()) return false;
    auto* Life=GetGameInstance()?GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>():nullptr;
    if(!Life || !Life->GetProfile() || Life->IsRecoveryBlocked()) { FrontEndError(TEXT("Profile recovery must complete before hosting.")); return false; }
    for(const auto& D:Life->GetProfile()->Deployments) if(!D.bResolved) { FrontEndError(TEXT("Resolve the earlier deployment before hosting.")); return false; }
    CloseFrontEndMenu(true);
    UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/TheUnit/Maps/CommandCenter")),true,TEXT("listen"));
    return true;
}
bool ATU_PlayerController::JoinCoopFromFrontEnd(const FString& Address)
{
    if(!IsLocalController() || !IsFrontEndWorld()) return false;
    FString Target=Address.TrimStartAndEnd();
    if(Target.IsEmpty() || Target.Len()>253 || Target.Contains(TEXT(" ")) || Target.Contains(TEXT("?")) || Target.Contains(TEXT(";")))
    { FrontEndError(TEXT("Enter a valid host address.")); return false; }
    CloseFrontEndMenu(true);
    ClientTravel(Target,ETravelType::TRAVEL_Absolute);
    return true;
}
bool ATU_PlayerController::PersistExitFor(APlayerController* PC,FString& Error)
{
    if(!HasAuthority() || !PC || !GetWorld() || !GetGameInstance()) {Error=TEXT("Only the session authority can resolve an exit.");return false;}
    if(auto* Mode=GetWorld()->GetAuthGameMode<ATU_GameMode>()) {
        APawn* LeavingPawn=PC->GetPawn();
        if(LeavingPawn) {
            const auto* State=Mode->FindParticipant(LeavingPawn);
            if(State && (State->Outcome==ETURaidPlayerOutcome::Active || State->Outcome==ETURaidPlayerOutcome::DisconnectedPendingResolution)) {
                if(!Mode->ResolvePlayerOutcome(LeavingPawn,ETURaidPlayerOutcome::Abandoned)) {
                    Error=TEXT("The raid outcome has not been saved yet. Stay here and retry. No extraction or successful exit has been claimed.");return false;
                }
            } else if(State && State->bSavePending) {Error=TEXT("An outcome save is pending. Please retry.");return false;}
        }
    }
    if(auto* Life=GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>()) {
        const auto* PS=PC->GetPlayerState<ATU_PlayerState>();
        const FGuid Id=PC->IsLocalController()?Life->GetLocalPlayerId():(PS?PS->PersistentPlayerId:FGuid());
        if(Life->GetProfile()) for(const auto& D:Life->GetProfile()->Deployments)
            if(!D.bResolved && D.PlayerId==Id) {Error=TEXT("Your deployment is still unresolved. Please wait and retry.");return false;}
    }
    return true;
}
void ATU_PlayerController::RequestConfirmedFrontEndExit(bool bQuit)
{
    if(!IsLocalController() || bFrontEndExitPending) return;
    bFrontEndExitPending=true;
    if(!HasAuthority()) {ServerRequestFrontEndExit(bQuit);if(MenuWidget.IsValid()) MenuWidget->ShowError(TEXT("Waiting for the server to save your outcome..."));return;}
    FString Error;
    TArray<APlayerController*> Controllers;
    if(GetNetMode()==NM_ListenServer) {
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It) if(It->Get()) Controllers.Add(It->Get());
    } else Controllers.Add(this);
    // Close only unresolved outcomes. Already-extracted/dead participants are not re-awarded.
    for(auto* PC:Controllers) if(!PersistExitFor(PC,Error)) {FrontEndError(Error);return;}
    auto* Life=GetGameInstance()->GetSubsystem<UTUHideoutLifecycleSubsystem>();
    if(Life) {
        if(!Life->GetProfile() || Life->IsRecoveryBlocked()) {FrontEndError(TEXT("Profile is unavailable or recovery is blocked. No save was overwritten."));return;}
        for(const auto& D:Life->GetProfile()->Deployments) if(!D.bResolved) {FrontEndError(TEXT("A participant outcome is still pending. Wait for resolution, then retry."));return;}
        if(!Life->SaveProfile()) {FrontEndError(Life->GetLastPersistenceError());return;}
    }
    for(auto* PC:Controllers) if(PC!=this) PC->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("Host ended the session. Unresolved deployments were abandoned; committed outcomes were preserved.")));
    CompleteLocalFrontEndExit(bQuit);
}
void ATU_PlayerController::ServerRequestFrontEndExit_Implementation(bool bQuit)
{
    FString Error;
    if(!PersistExitFor(this,Error)) {ClientFrontEndExitResult(false,bQuit,Error);return;}
    // Client cannot manufacture a successful extraction, change other players or write host preferences.
    ClientFrontEndExitResult(true,bQuit,FString());
}
void ATU_PlayerController::ClientFrontEndExitResult_Implementation(bool bSuccess,bool bQuit,const FString& Error)
{
    if(!bSuccess) {FrontEndError(Error);return;}
    CompleteLocalFrontEndExit(bQuit);
}
void ATU_PlayerController::CompleteLocalFrontEndExit(bool bQuit)
{
    CloseFrontEndMenu(true);
    if(bQuit) UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
    else if(GetNetMode()==NM_Client) ClientTravel(TEXT("/Game/TheUnit/Maps/MainMenu"),TRAVEL_Absolute);
    else UGameplayStatics::OpenLevel(this,TEXT("/Game/TheUnit/Maps/MainMenu"));
}
