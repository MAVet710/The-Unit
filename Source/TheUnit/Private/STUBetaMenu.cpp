#include "STUBetaMenu.h"
#include "TU_PlayerController.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"

TSharedRef<SWidget> STUBetaMenu::Label(const FString& Text,int32 Size) const
{
    return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size))
        .ColorAndOpacity(FLinearColor(.86f,.88f,.84f)).AutoWrapText(true);
}
TSharedRef<SWidget> STUBetaMenu::Button(const FString& Text,FName Id)
{
    return SNew(SButton).ButtonColorAndOpacity(FLinearColor(.13f,.17f,.15f))
        .ContentPadding(FMargin(20,12)).OnClicked_Lambda([this,Id]{return Command(Id);})[Label(Text,17)];
}
void STUBetaMenu::Construct(const FArguments& Args)
{
    Owner=Args._Owner; ReadDraft(); Rebuild();
}
void STUBetaMenu::ReadDraft()
{
    if(auto* S=UTUBetaUserSettings::Get()) {
        DraftVolume=S->MasterVolume;DraftSensitivity=S->MouseSensitivity;DraftFOV=S->FieldOfView;
        DraftInvert=S->bInvertVertical;DraftSway=S->bCameraSwayEnabled;
        DraftVSync=S->IsVSyncEnabled();DraftQuality=S->GetOverallScalabilityLevel();
        DraftResolution=S->GetScreenResolution();DraftMode=S->GetFullscreenMode();DraftLimit=S->GetFrameRateLimit();
        if(DraftResolution.X<640 || DraftResolution.Y<360) DraftResolution=FIntPoint(1280,720);
    }
}
void STUBetaMenu::ShowError(const FString& Text) { Message=Text; Rebuild(); }
void STUBetaMenu::Rebuild()
{
    const bool Front=Owner.IsValid() && Owner->IsFrontEndWorld();
    ChildSlot [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.022f,.032f,.03f,.97f)).Padding(FMargin(48,28))
        [ SAssignNew(Layout,SVerticalBox) ] ];
    Layout->AddSlot().AutoHeight().Padding(0,0,0,8)[Label(TEXT("T H E   U N I T"),44)];
    Layout->AddSlot().AutoHeight().Padding(0,0,0,20)[Label(Front?TEXT("PREPARE. DEPLOY. RETURN."):Owner->MenuStatusText(),15)];
    TSharedRef<SVerticalBox> Body=SNew(SVerticalBox);
    Layout->AddSlot().FillHeight(1.f)[SNew(SScrollBox).ScrollBarAlwaysVisible(true).ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)+SScrollBox::Slot()[Body]];
    if(Page==TEXT("home")) {
        if(Front) {
            Body->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(TEXT("Functional preview  /  Solo and training entry"),20)];
            Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("HEADQUARTERS    Prepare your persistent kit"),TEXT("hq"))];
            Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("HOST CO-OP    Open Headquarters as a two-player listen server"),TEXT("hostcoop"))];
            Body->AddSlot().AutoHeight().Padding(0,0,0,4)[Label(TEXT("JOIN CO-OP    Host address"),14)];
            Body->AddSlot().AutoHeight().Padding(0,0,0,6)[SNew(SEditableTextBox).Text_Lambda([this]{return FText::FromString(JoinAddress);})
                .OnTextChanged_Lambda([this](const FText& V){JoinAddress=V.ToString();})];
            Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("JOIN HOST"),TEXT("joincoop"))];
            Body->AddSlot().AutoHeight().Padding(0,0,0,18)[Button(TEXT("TRAINING    Practice in the Killhouse"),TEXT("training"))];
        } else Body->AddSlot().AutoHeight().Padding(0,0,0,16)[Button(TEXT("RESUME"),TEXT("resume"))];
        Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("SETTINGS"),TEXT("settings"))];
        Body->AddSlot().AutoHeight().Padding(0,0,0,18)[Button(TEXT("CONTROLS"),TEXT("controls"))];
        if(!Front) Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("RETURN TO MAIN MENU"),TEXT("leave"))];
        Body->AddSlot().AutoHeight().Padding(0,0,0,18)[Button(TEXT("QUIT TO DESKTOP"),TEXT("quit"))];
        Body->AddSlot().AutoHeight()[Label(Front?TEXT("Training does not award live-raid loot. Enter Headquarters to prepare for a raid. Visual quality and additional beta systems are still in development."):TEXT("Returning to a menu is not extraction. Unresolved live deployments are abandoned only after confirmation."),15)];
    } else if(Page==TEXT("settings")) SettingsContent(Body);
    else if(Page==TEXT("controls")) ControlsContent(Body);
    else if(Page==TEXT("confirm")) {
        Body->AddSlot().AutoHeight().Padding(0,0,0,22)[Label(bQuitRequested?TEXT("Quit to desktop?"):TEXT("Return to main menu?"),28)];
        Body->AddSlot().AutoHeight().Padding(0,0,0,26)[Label(Owner.IsValid()?Owner->ExitWarningText():TEXT("Session unavailable."),19)];
        Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("CANCEL    Stay in this session"),TEXT("cancel"))];
        Body->AddSlot().AutoHeight()[Button(bQuitRequested?TEXT("CONFIRM QUIT"):TEXT("CONFIRM RETURN"),TEXT("confirmexit"))];
    } else if(Page==TEXT("video")) {
        Body->AddSlot().AutoHeight().Padding(0,0,0,20)[Label(TEXT("Keep this display mode?"),28)];
        Body->AddSlot().AutoHeight().Padding(0,0,0,25)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",18))
            .Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("Reverting in %d seconds unless you confirm."),FMath::Max(0,FMath::CeilToInt(ConfirmDeadline-FPlatformTime::Seconds()))));})];
        Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("KEEP DISPLAY MODE AND SAVE"),TEXT("keepvideo"))];
        Body->AddSlot().AutoHeight()[Button(TEXT("REVERT"),TEXT("revertvideo"))];
    }
    if(!Message.IsEmpty()) Layout->AddSlot().AutoHeight().Padding(0,12,0,0)[Label(Message,16)];
    Layout->AddSlot().AutoHeight().Padding(0,18,0,0)[Label(TEXT("ESC  Back / field menu       TAB  Navigate       ENTER  Select       Local prototype, not a finished beta"),12)];
}
void STUBetaMenu::SettingsContent(TSharedRef<SVerticalBox> Body)
{
    Body->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(TEXT("Settings"),26)];
    auto Slider=[&](const FString& Name,float* Target,float Low,float High) {
        Body->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))
            .Text_Lambda([Name,Target]{return FText::FromString(FString::Printf(TEXT("%s   %.2f"),*Name,*Target));})];
        Body->AddSlot().AutoHeight().Padding(0,0,14,8)[SNew(SSlider).Value_Lambda([Target,Low,High]{return (*Target-Low)/(High-Low);})
            .OnValueChanged_Lambda([Target,Low,High](float V){*Target=FMath::Lerp(Low,High,V);})];
    };
    Slider(TEXT("Master volume"),&DraftVolume,0.f,1.f);
    Slider(TEXT("Mouse sensitivity"),&DraftSensitivity,.1f,3.f);
    Slider(TEXT("Base field of view"),&DraftFOV,70.f,110.f);
    auto Check=[&](const FString& Name,bool* Target){
        Body->AddSlot().AutoHeight().Padding(0,6)[SNew(SCheckBox).IsChecked_Lambda([Target]{return *Target?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
            .OnCheckStateChanged_Lambda([Target](ECheckBoxState S){*Target=S==ECheckBoxState::Checked;})[SNew(STextBlock).Text(FText::FromString(Name)).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(FLinearColor(.86f,.88f,.84f))]];
    };
    Check(TEXT("Invert vertical look"),&DraftInvert);Check(TEXT("Cosmetic movement sway"),&DraftSway);Check(TEXT("Vertical sync"),&DraftVSync);
    auto Combo=[&](const FString& Name,TArray<TSharedPtr<FString>>& Options,TFunction<FString()> Text,TFunction<void(FString)> Set){
        Body->AddSlot().AutoHeight().Padding(0,8)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(.45f).VAlign(VAlign_Center)[Label(Name)]
            +SHorizontalBox::Slot().FillWidth(.55f)[SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Options)
                .OnGenerateWidget_Lambda([this](TSharedPtr<FString> V){return Label(*V);})
                .OnSelectionChanged_Lambda([Set](TSharedPtr<FString> V,ESelectInfo::Type){if(V.IsValid()) Set(*V);})
                [SNew(STextBlock).Text_Lambda([Text]{return FText::FromString(Text());}).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))]]];
    };
    ResolutionOptions.Reset();
    for(const TCHAR* R:{TEXT("1280 x 720"),TEXT("1600 x 900"),TEXT("1920 x 1080"),TEXT("2560 x 1440"),TEXT("3840 x 2160")}) ResolutionOptions.Add(MakeShared<FString>(R));
    Combo(TEXT("Resolution"),ResolutionOptions,[this]{return FString::Printf(TEXT("%d x %d"),DraftResolution.X,DraftResolution.Y);},[this](FString V){FString A,B;V.Split(TEXT(" x "),&A,&B);DraftResolution=FIntPoint(FCString::Atoi(*A),FCString::Atoi(*B));});
    ModeOptions={MakeShared<FString>(TEXT("Windowed")),MakeShared<FString>(TEXT("Borderless")),MakeShared<FString>(TEXT("Fullscreen"))};
    Combo(TEXT("Display mode"),ModeOptions,[this]{return DraftMode==EWindowMode::Windowed?TEXT("Windowed"):DraftMode==EWindowMode::WindowedFullscreen?TEXT("Borderless"):TEXT("Fullscreen");},[this](FString V){DraftMode=V==TEXT("Windowed")?EWindowMode::Windowed:V==TEXT("Borderless")?EWindowMode::WindowedFullscreen:EWindowMode::Fullscreen;});
    QualityOptions={MakeShared<FString>(TEXT("Low")),MakeShared<FString>(TEXT("Medium")),MakeShared<FString>(TEXT("High")),MakeShared<FString>(TEXT("Epic"))};
    Combo(TEXT("Graphics preset"),QualityOptions,[this]{return DraftQuality<0?FString(TEXT("Custom")):*QualityOptions[FMath::Clamp(DraftQuality,0,3)];},[this](FString V){for(int32 I=0;I<QualityOptions.Num();++I) if(*QualityOptions[I]==V) DraftQuality=I;});
    LimitOptions.Reset();for(const TCHAR* V:{TEXT("Unlimited"),TEXT("30"),TEXT("60"),TEXT("90"),TEXT("120"),TEXT("144")}) LimitOptions.Add(MakeShared<FString>(V));
    Combo(TEXT("Frame limit"),LimitOptions,[this]{return DraftLimit<=0.f?FString(TEXT("Unlimited")):FString::FromInt(FMath::RoundToInt(DraftLimit));},[this](FString V){DraftLimit=V==TEXT("Unlimited")?0.f:FCString::Atof(*V);});
    Body->AddSlot().AutoHeight().Padding(0,18,0,6)[Button(TEXT("APPLY AND SAVE"),TEXT("apply"))];
    Body->AddSlot().AutoHeight()[Button(TEXT("BACK    Discard unapplied changes"),TEXT("back"))];
}
void STUBetaMenu::ControlsContent(TSharedRef<SVerticalBox> Body)
{
    Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Label(TEXT("Controls"),26)];
    Body->AddSlot().AutoHeight().Padding(0,0,0,14)[Label(TEXT("Select a binding, then press a key or mouse button. Escape cancels capture. Conflicts are rejected; bindings save immediately."),15)];
    auto* Settings=UTUBetaUserSettings::Get();
    if(Settings) for(const auto& B:UTUBetaUserSettings::Bindings()) {
        const FName Id=B.Id;
        Body->AddSlot().AutoHeight().Padding(0,3)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(.62f).VAlign(VAlign_Center)[Label(B.Label)]
            +SHorizontalBox::Slot().FillWidth(.38f)[SNew(SButton).ContentPadding(FMargin(12,5)).OnClicked_Lambda([this,Id]{BeginBindingCapture(Id);return FReply::Handled();})
                [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).Text_Lambda([this,Settings,Id]{return Capturing==Id?FText::FromString(TEXT("Waiting for key...")):Settings->EffectiveKey(Id).GetDisplayName();})]]];
    }
    Body->AddSlot().AutoHeight().Padding(0,18,0,6)[Button(TEXT("RESTORE DEFAULT BINDINGS"),TEXT("resetkeys"))];
    Body->AddSlot().AutoHeight()[Button(TEXT("BACK"),TEXT("back"))];
}
void STUBetaMenu::CaptureKey(FKey Key)
{
    if(Capturing.IsNone()) return;
    if(Key==EKeys::Escape) {Capturing=NAME_None;Message=TEXT("Binding unchanged.");Rebuild();return;}
    if(auto* S=UTUBetaUserSettings::Get()) {
        const auto Previous=S->KeyOverrides;
        if(S->SetBinding(Capturing,Key,Message)) {
            if(!S->CommitPreferences(Owner.Get(),Message)) S->KeyOverrides=Previous;
            else {Message=TEXT("Binding saved.");Capturing=NAME_None;}
        }
    }
    Rebuild();FSlateApplication::Get().SetKeyboardFocus(AsShared());
}
FReply STUBetaMenu::OnKeyDown(const FGeometry&,const FKeyEvent& Event)
{
    if(!Capturing.IsNone()) {CaptureKey(Event.GetKey());return FReply::Handled();}
    if(Event.GetKey()==EKeys::Escape) {
        if(bVideoPending) return Command(TEXT("revertvideo"));
        if(Page!=TEXT("home")) return Command(TEXT("back"));
        if(Owner.IsValid()&&!Owner->IsFrontEndWorld()) return Command(TEXT("resume"));
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
FReply STUBetaMenu::OnPreviewMouseButtonDown(const FGeometry&,const FPointerEvent& Event)
{
    if(!Capturing.IsNone()) {CaptureKey(Event.GetEffectingButton());return FReply::Handled();}
    return FReply::Unhandled();
}
void STUBetaMenu::Tick(const FGeometry& Geometry,double Time,float Delta)
{
    SCompoundWidget::Tick(Geometry,Time,Delta);
    if(bVideoPending && FPlatformTime::Seconds()>=ConfirmDeadline) Command(TEXT("revertvideo"));
}
void STUBetaMenu::SaveDraft()
{
    auto* S=UTUBetaUserSettings::Get();if(!S) {ShowError(TEXT("Settings service unavailable."));return;}
    S->MasterVolume=DraftVolume;S->MouseSensitivity=DraftSensitivity;S->FieldOfView=DraftFOV;
    S->bInvertVertical=DraftInvert;S->bCameraSwayEnabled=DraftSway;
    S->SetVSyncEnabled(DraftVSync);if(DraftQuality>=0) S->SetOverallScalabilityLevel(DraftQuality);S->SetFrameRateLimit(DraftLimit);
    if(S->CommitPreferences(Owner.Get(),Message)) Message=TEXT("Settings saved.");
    Page=TEXT("settings");Rebuild();
}
void STUBetaMenu::CancelPendingDisplay()
{
    if(!bVideoPending) return;
    bVideoPending=false;
    if(auto* S=UTUBetaUserSettings::Get()) {S->SetScreenResolution(OldResolution);S->SetFullscreenMode(OldMode);S->ApplyResolutionSettings(false);S->ConfirmVideoMode();}
    DraftResolution=OldResolution;DraftMode=OldMode;
}
FReply STUBetaMenu::Command(FName Id)
{
    const auto Lifetime=AsShared(); // Travel may remove the viewport and controller references.
    (void)Lifetime;
    if(!Owner.IsValid()) return FReply::Handled();
    if(Id==TEXT("training") || Id==TEXT("hq")) {Owner->EnterFromFrontEnd(Id==TEXT("training"));return FReply::Handled();}
    if(Id==TEXT("hostcoop")) {Owner->HostCoopFromFrontEnd();return FReply::Handled();}
    if(Id==TEXT("joincoop")) {Owner->JoinCoopFromFrontEnd(JoinAddress);return FReply::Handled();}
    if(Id==TEXT("resume")) {Owner->CloseFrontEndMenu();return FReply::Handled();}
    if(Id==TEXT("settings")) {ReadDraft();Page=TEXT("settings");Message.Empty();}
    else if(Id==TEXT("controls")) {Page=TEXT("controls");Message.Empty();}
    else if(Id==TEXT("back") || Id==TEXT("cancel")) {CancelPendingDisplay();Capturing=NAME_None;Page=TEXT("home");Message.Empty();}
    else if(Id==TEXT("leave") || Id==TEXT("quit")) {Page=TEXT("confirm");bQuitRequested=Id==TEXT("quit");Message.Empty();}
    else if(Id==TEXT("confirmexit")) {Owner->RequestConfirmedFrontEndExit(bQuitRequested);return FReply::Handled();}
    else if(Id==TEXT("resetkeys")) {if(auto* S=UTUBetaUserSettings::Get()){const auto Previous=S->KeyOverrides;S->KeyOverrides.Reset();if(S->CommitPreferences(Owner.Get(),Message)) Message=TEXT("Default bindings restored.");else S->KeyOverrides=Previous;}}
    else if(Id==TEXT("apply")) {
        if(auto* S=UTUBetaUserSettings::Get()) {
            OldResolution=S->GetScreenResolution();OldMode=S->GetFullscreenMode();
            if(DraftResolution!=OldResolution || DraftMode!=OldMode) {
                PreviewDisplay(DraftResolution,DraftMode);return FReply::Handled();
            } else {SaveDraft();return FReply::Handled();}
        }
    } else if(Id==TEXT("keepvideo")) {bVideoPending=false;SaveDraft();return FReply::Handled();}
    else if(Id==TEXT("revertvideo")) {CancelPendingDisplay();Page=TEXT("settings");Message=TEXT("Display mode reverted. Other changes are not saved.");}
    Rebuild();FSlateApplication::Get().SetKeyboardFocus(AsShared());return FReply::Handled();
}

void STUBetaMenu::BeginBindingCapture(FName Id)
{
    // Only a selected control inside this game's focused menu receives a new binding.
    if(!UTUBetaUserSettings::Bindings().ContainsByPredicate([&](const FTUBetaBinding& B){return B.Id==Id;}))return;
    Capturing=Id;Message=TEXT("Press the new key. Escape cancels.");
    FSlateApplication::Get().SetKeyboardFocus(AsShared());
}
void STUBetaMenu::PreviewDisplay(FIntPoint Resolution,EWindowMode::Type Mode)
{
    auto* S=UTUBetaUserSettings::Get();if(!S||Resolution.X<640||Resolution.Y<360)return;
    if(bVideoPending)CancelPendingDisplay();
    OldResolution=S->GetScreenResolution();OldMode=S->GetFullscreenMode();
    DraftResolution=Resolution;DraftMode=Mode;
    bVideoPending=true;ConfirmDeadline=FPlatformTime::Seconds()+15.;Page=TEXT("video");
    S->SetScreenResolution(Resolution);S->SetFullscreenMode(Mode);S->ApplyResolutionSettings(false);
    Rebuild();FSlateApplication::Get().SetKeyboardFocus(AsShared());
}
