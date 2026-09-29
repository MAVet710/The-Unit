#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TU_PlayerController.generated.h"

class APawn;
class UTUMX50TabletComponent;
class STUBetaMenu;

/** Player input and local-control bridge for tactical UI commands. */
UCLASS()
class THEUNIT_API ATU_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATU_PlayerController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool IsFrontEndWorld() const;
    bool IsLiveRaidWorld() const;
    static bool ShouldPauseMenu(ENetMode Mode,bool bLive);
    FString MenuStatusText() const;
    FString ExitWarningText() const;
    void ToggleFrontEndMenu();
    void OpenFrontEndMenu();
    void CloseFrontEndMenu(bool bForce=false);
    void EnterFromFrontEnd(bool bTraining);
    bool HostCoopFromFrontEnd();
    bool JoinCoopFromFrontEnd(const FString& Address);
    void RequestConfirmedFrontEndExit(bool bQuit);
    TSharedPtr<STUBetaMenu> GetFrontEndMenu() const { return MenuWidget; }
    bool IsFrontEndExitPending() const { return bFrontEndExitPending; }

    UFUNCTION(BlueprintPure, Category="MX50")
    UTUMX50TabletComponent* GetMX50Tablet() const { return MX50Tablet; }

protected:
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MX50")
    TObjectPtr<UTUMX50TabletComponent> MX50Tablet;

private:
    void ToggleMX50();
    TSharedPtr<STUBetaMenu> MenuWidget;
    bool bPausedByFrontEnd=false;
    bool bFrontEndOwnsInput=false;
    bool bFrontEndExitPending=false;
    bool PersistExitFor(APlayerController* PC,FString& Error);
    void CompleteLocalFrontEndExit(bool bQuit);
    void FrontEndError(const FString& Error);
    UFUNCTION(Server,Reliable) void ServerRequestFrontEndExit(bool bQuit);
    UFUNCTION(Client,Reliable) void ClientFrontEndExitResult(bool bSuccess,bool bQuit,const FString& Error);
};
