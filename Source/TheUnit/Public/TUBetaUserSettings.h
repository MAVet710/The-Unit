#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "TUBetaUserSettings.generated.h"

class APlayerController;
struct FTUBetaBinding
{
    FName Id;
    FString Label;
    FName Mapping;
    FKey DefaultKey;
    float AxisScale = 0.f; // zero denotes an action
};

/** Local preferences only. This class never reads or writes inventory/save-game ownership. */
UCLASS(Config=GameUserSettings)
class THEUNIT_API UTUBetaUserSettings : public UGameUserSettings
{
    GENERATED_BODY()
public:
    UTUBetaUserSettings();
    UPROPERTY(Config) float MasterVolume = 1.f;
    UPROPERTY(Config) float MouseSensitivity = 1.f;
    UPROPERTY(Config) float FieldOfView = 90.f;
    UPROPERTY(Config) bool bInvertVertical = false;
    UPROPERTY(Config) bool bCameraSwayEnabled = true;
    UPROPERTY(Config) TMap<FName, FKey> KeyOverrides;
    virtual void SetToDefaults() override;
    virtual void ValidateSettings() override;
    static UTUBetaUserSettings* Get();
    static const TArray<FTUBetaBinding>& Bindings();
    FKey EffectiveKey(FName Id) const;
    bool SetBinding(FName Id, FKey Key, FString& Error);
    bool CommitPreferences(APlayerController* Controller, FString& Error);
    void ApplyToController(APlayerController* Controller) const;
    void ApplyStartupDisplay();
private:
    bool bStartupDisplayApplied=false;
};
