#include "TU_PlayerController.h"

#include "TU_ArmedOperatorCharacter.h"
#include "TUMX50TabletComponent.h"
#include "Components/InputComponent.h"

ATU_PlayerController::ATU_PlayerController()
{
    MX50Tablet = CreateDefaultSubobject<UTUMX50TabletComponent>(TEXT("MX50Tablet"));
}

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

    if (MX50Tablet)
    {
        const FTMX50MissionSnapshot Snapshot = MX50Tablet->GetMissionSnapshot();
        Operator->OpenBriefing(Snapshot.MissionId, Snapshot.MissionTitle);
    }
    else
    {
        Operator->OpenBriefing(NAME_None, FText());
    }
}
