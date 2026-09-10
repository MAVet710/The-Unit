#include "TU_PlayerController.h"

#include "TUHideoutLifecycleSubsystem.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TUMX50TabletComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"

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

void ATU_PlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ATU_ArmedOperatorCharacter* Operator = Cast<ATU_ArmedOperatorCharacter>(InPawn))
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
            {
                Lifecycle->ApplyOperatorLoadout(Operator);
            }
        }
    }
}

void ATU_PlayerController::OnUnPossess()
{
    if (ATU_ArmedOperatorCharacter* Operator = Cast<ATU_ArmedOperatorCharacter>(GetPawn()))
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UTUHideoutLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<UTUHideoutLifecycleSubsystem>())
            {
                Lifecycle->CaptureOperatorLoadout(Operator);
                Lifecycle->SaveProfile();
            }
        }
    }

    Super::OnUnPossess();
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
