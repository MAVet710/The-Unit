#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TUBetaUserSettings.h"
class ATU_PlayerController;
class SVerticalBox;
class SScrollBox;
/** Native, keyboard-navigable local front end. The controller owns all travel decisions. */
class STUBetaMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STUBetaMenu) {} SLATE_ARGUMENT(TWeakObjectPtr<ATU_PlayerController>, Owner) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual FReply OnPreviewMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual void Tick(const FGeometry&,double,float) override;
    FReply Command(FName Id); // same command dispatch used by visible buttons and opt-in QA
    FString PageName() const { return Page; }
    void ShowError(const FString& Text);
    void CancelPendingDisplay();
    FString LastMessage() const { return Message; }
    void BeginBindingCapture(FName Id);
    void PreviewDisplay(FIntPoint Resolution,EWindowMode::Type Mode);
private:
    TWeakObjectPtr<ATU_PlayerController> Owner;
    TSharedPtr<SVerticalBox> Layout;
    FString Page=TEXT("home"), Message;
    FString JoinAddress=TEXT("127.0.0.1:7777");
    FName Capturing;
    bool bQuitRequested=false, bVideoPending=false;
    double ConfirmDeadline=0.;
    FIntPoint OldResolution;
    EWindowMode::Type OldMode=EWindowMode::Windowed;
    float DraftVolume=1.f, DraftSensitivity=1.f, DraftFOV=90.f, DraftLimit=60.f;
    bool DraftInvert=false, DraftSway=true, DraftVSync=false;
    int32 DraftQuality=2;
    FIntPoint DraftResolution;
    EWindowMode::Type DraftMode=EWindowMode::Windowed;
    TArray<TSharedPtr<FString>> ResolutionOptions, ModeOptions, QualityOptions, LimitOptions;
    void Rebuild();
    void ReadDraft();
    void SaveDraft();
    void CaptureKey(FKey Key);
    TSharedRef<SWidget> Button(const FString&,FName);
    TSharedRef<SWidget> Label(const FString&,int32 Size=16) const;
    void SettingsContent(TSharedRef<SVerticalBox> Body);
    void ControlsContent(TSharedRef<SVerticalBox> Body);
};
