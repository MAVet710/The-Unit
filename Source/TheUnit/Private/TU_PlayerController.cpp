#include "TU_PlayerController.h"

#include "TU_ArmedOperatorCharacter.h"
#include "Components/InputComponent.h"

void ATU_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction(TEXT("ToggleMX50"), IE_Pressed, this, &ATU_PlayerController::ToggleMX50);
    }
}

void ATU_PlayerController::ToggleMX50()
{
    ATU_ArmedOperatorCharacter* Operator = Cast<ATU_ArmedOperatorCharacter>(GetPawn());
    if (!Operator)
    {
        return;
    }

    if (Operator->IsBriefingOpen())
    {
        Operator->CloseBriefing();
        return;
    }

    if (Operator->IsArmoryOpen() || Operator->IsMeleeEquipped())
    {
        return;
    }

    Operator->OpenBriefing(NAME_None, FText());
}
