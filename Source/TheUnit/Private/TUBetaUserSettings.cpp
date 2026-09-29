#include "TUBetaUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "AudioDevice.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManager.h"

UTUBetaUserSettings::UTUBetaUserSettings() {}
UTUBetaUserSettings* UTUBetaUserSettings::Get()
{
    return GEngine ? Cast<UTUBetaUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}
void UTUBetaUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    MasterVolume=1.f; MouseSensitivity=1.f; FieldOfView=90.f;
    bInvertVertical=false; bCameraSwayEnabled=true; KeyOverrides.Reset();
}
const TArray<FTUBetaBinding>& UTUBetaUserSettings::Bindings()
{
    static const TArray<FTUBetaBinding> Items = {
        {"Forward", "Move forward", "MoveForward", EKeys::W, 1.f},
        {"Back", "Move backward", "MoveForward", EKeys::S, -1.f},
        {"Right", "Move right", "MoveRight", EKeys::D, 1.f},
        {"Left", "Move left", "MoveRight", EKeys::A, -1.f},
        {"Sprint", "Sprint (hold)", "Sprint", EKeys::LeftShift},
        {"Crouch", "Crouch (hold)", "Crouch", EKeys::LeftControl},
        {"ADS", "Aim (hold)", "ADS", EKeys::RightMouseButton},
        {"Fire", "Fire", "Fire", EKeys::LeftMouseButton},
        {"LeanLeft", "Lean left (hold)", "LeanLeft", EKeys::Q},
        {"LeanRight", "Lean right (hold)", "LeanRight", EKeys::E},
        {"Interact", "Interact", "Interact", EKeys::F},
        {"Reload", "Reload and retain magazine", "Reload", EKeys::R},
        {"EmergencyReload", "Reload and drop magazine", "EmergencyReload", EKeys::G},
        {"InspectWeapon", "Inspect weapon", "InspectWeapon", EKeys::I},
        {"CycleAction", "Cycle weapon action", "CycleAction", EKeys::C},
        {"CycleReady", "Change ready position", "CycleReady", EKeys::H},
        {"CycleFireMode", "Change fire mode", "CycleFireMode", EKeys::B},
        {"EquipPrimary", "Primary weapon", "EquipPrimary", EKeys::One},
        {"EquipSecondary", "Secondary weapon", "EquipSecondary", EKeys::Two},
        {"ToggleMelee", "Melee", "ToggleMelee", EKeys::V},
        {"CycleMelee", "Select melee", "CycleMelee", EKeys::N},
        {"ToggleArmory", "Armory (headquarters)", "ToggleArmory", EKeys::L},
        {"ToggleMX50", "Mission tablet", "ToggleMX50", EKeys::T}
    };
    return Items;
}
FKey UTUBetaUserSettings::EffectiveKey(FName Id) const
{
    if(const FKey* Override=KeyOverrides.Find(Id)) return *Override;
    for(const auto& Binding:Bindings()) if(Binding.Id==Id) return Binding.DefaultKey;
    return EKeys::Invalid;
}
bool UTUBetaUserSettings::SetBinding(FName Id,FKey Key,FString& Error)
{
    if(!Bindings().ContainsByPredicate([&](const auto& V){return V.Id==Id;})) { Error=TEXT("Unknown control.");return false; }
    if(!Key.IsValid() || Key.IsGamepadKey() || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || Key==EKeys::Escape || Key==EKeys::Tab || Key==EKeys::Tilde)
    { Error=TEXT("Choose a keyboard or mouse button. Escape, Tab and the console key are reserved.");return false; }
    for(const auto& Binding:Bindings()) if(Binding.Id!=Id && EffectiveKey(Binding.Id)==Key)
    { Error=FString::Printf(TEXT("%s already uses %s. Change that binding first."),*Binding.Label,*Key.GetDisplayName().ToString());return false; }
    KeyOverrides.Add(Id,Key); Error.Empty();return true;
}
void UTUBetaUserSettings::ValidateSettings()
{
    Super::ValidateSettings();
    MasterVolume=FMath::IsFinite(MasterVolume)?FMath::Clamp(MasterVolume,0.f,1.f):1.f;
    MouseSensitivity=FMath::IsFinite(MouseSensitivity)?FMath::Clamp(MouseSensitivity,.1f,3.f):1.f;
    FieldOfView=FMath::IsFinite(FieldOfView)?FMath::Clamp(FieldOfView,70.f,110.f):90.f;
    // Validate as a complete mapping. Corrupt/ambiguous mappings revert as a set.
    TSet<FKey> Used; bool Invalid=false;
    for(const auto& Binding:Bindings()) {
        const FKey K=EffectiveKey(Binding.Id);
        Invalid |= !K.IsValid() || K.IsGamepadKey() || K.IsAxis1D() || K.IsAxis2D() || K.IsAxis3D() || K==EKeys::Escape || K==EKeys::Tab || K==EKeys::Tilde || Used.Contains(K);
        Used.Add(K);
    }
    if(Invalid) KeyOverrides.Reset();
    TArray<FName> Unknown;
    for(const auto& It:KeyOverrides) if(!Bindings().ContainsByPredicate([&](const auto& B){return B.Id==It.Key;})) Unknown.Add(It.Key);
    for(FName Id:Unknown) KeyOverrides.Remove(Id);
}
void UTUBetaUserSettings::ApplyToController(APlayerController* PC) const
{
    if(!PC || !PC->IsLocalController()) return;
    if(PC->PlayerInput) {
        UPlayerInput* Input=PC->PlayerInput.Get();
        Input->FlushPressedKeys();
        for(const auto& B:Bindings()) {
            if(B.AxisScale==0.f) {
                const auto Existing=Input->ActionMappings;
                for(const auto& M:Existing) if(M.ActionName==B.Mapping && !M.Key.IsGamepadKey()) Input->RemoveActionMapping(M);
                Input->AddActionMapping(FInputActionKeyMapping(B.Mapping,EffectiveKey(B.Id)));
            } else {
                const auto Existing=Input->AxisMappings;
                for(const auto& M:Existing) if(M.AxisName==B.Mapping && M.Scale==B.AxisScale && !M.Key.IsGamepadKey() && !M.Key.IsAxis1D()) Input->RemoveAxisMapping(M);
                Input->AddAxisMapping(FInputAxisKeyMapping(B.Mapping,EffectiveKey(B.Id),B.AxisScale));
            }
        }
        Input->ForceRebuildingKeyMaps(false);
    }
    if(PC->GetWorld()) {
        FAudioDeviceHandle Audio=PC->GetWorld()->GetAudioDevice();
        if(Audio.IsValid()) Audio->SetTransientPrimaryVolume(MasterVolume);
    }
}
bool UTUBetaUserSettings::CommitPreferences(APlayerController* PC,FString& Error)
{
    const FConfigBranch* Branch=GConfig?GConfig->FindBranch(TEXT("GameUserSettings"),FString()):nullptr;
    const FString DiskPath=Branch?Branch->IniPath:FString();
    if(DiskPath.IsEmpty()) {Error=TEXT("Settings storage path is unavailable.");return false;}
    if(IFileManager::Get().IsReadOnly(*DiskPath)) { Error=TEXT("Settings file is read-only. Changes were not saved.");return false; }
    UE_LOG(LogTemp,Display,TEXT("TU_PREFERENCES_SAVE frame=%llu path=%s"),GFrameCounter,*DiskPath);
    ValidateSettings(); ApplyNonResolutionSettings(); ConfirmVideoMode(); SaveSettings();
    if(GConfig) GConfig->Flush(false,GGameUserSettingsIni);
    if(!IFileManager::Get().FileExists(*DiskPath)) { Error=TEXT("Settings could not be saved. Check folder write permissions.");return false; }
    FConfigFile Disk=Branch->CombinedStaticLayers;Disk.Combine(DiskPath);
    const TCHAR* Section=TEXT("/Script/TheUnit.TUBetaUserSettings");
    float Volume=-1.f,Sensitivity=-1.f,FOV=-1.f;
    const bool Verified=Disk.GetFloat(Section,TEXT("MasterVolume"),Volume)
        && Disk.GetFloat(Section,TEXT("MouseSensitivity"),Sensitivity)
        && Disk.GetFloat(Section,TEXT("FieldOfView"),FOV)
        && FMath::IsNearlyEqual(Volume,MasterVolume,.0001f)
        && FMath::IsNearlyEqual(Sensitivity,MouseSensitivity,.0001f)
        && FMath::IsNearlyEqual(FOV,FieldOfView,.0001f);
    if(!Verified) {Error=TEXT("Settings write could not be verified. Please retry; no successful save is claimed.");return false;}
    ApplyToController(PC); Error.Empty();return true;
}

void UTUBetaUserSettings::ApplyStartupDisplay()
{
    if(bStartupDisplayApplied)return;
    bStartupDisplayApplied=true;
    // Apply to the initialized local viewport, once per process. Explicit launch
    // overrides remain respected. Never rerun during a pending settings preview.
    ValidateSettings();ApplyResolutionSettings(true);ApplyNonResolutionSettings();
}
