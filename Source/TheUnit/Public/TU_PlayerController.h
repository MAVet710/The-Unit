#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TU_PlayerController.generated.h"

class UTUMX50TabletComponent;

/** Player input and local-control bridge for tactical UI commands. */
UCLASS()
class THEUNIT_API ATU_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATU_PlayerController();

    UFUNCTION(BlueprintPure, Category="MX50")
    UTUMX50TabletComponent* GetMX50Tablet() const { return MX50Tablet; }

protected:
    virtual void SetupInputComponent() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MX50")
    TObjectPtr<UTUMX50TabletComponent> MX50Tablet;

private:
    void ToggleMX50();
};
