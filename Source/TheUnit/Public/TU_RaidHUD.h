#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TU_RaidHUD.generated.h"

/** Read-only raid feedback; every mutation stays in the gameplay authority. */
UCLASS()
class THEUNIT_API ATU_RaidHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    /** Returns rendered text height; backing is measured to fit the current viewport. */
    float DrawReadableText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale = 1.f);
};
