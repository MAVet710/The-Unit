#include "TU_CommandCenterStation.h"

#include "TUArmoryWidget.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_PlayerController.h"
#include "TUMissionPackageData.h"
#include "TUMX50TabletComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATU_CommandCenterStation::ATU_CommandCenterStation()
{
    PrimaryActorTick.bCanEverTick = false;

    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    SetRootComponent(StationMesh);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        StationMesh->SetStaticMesh(CubeFinder.Object);
        StationMesh->SetRelativeScale3D(FVector(1.2f, 0.6f, 1.0f));
    }
    StationMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

    InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
    InteractionVolume->SetupAttachment(StationMesh);
    InteractionVolume->SetBoxExtent(FVector(220.0f, 180.0f, 140.0f));
    InteractionVolume->SetRelativeLocation(FVector(-100.0f, 0.0f, 40.0f));
    InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    StationLabel = FText::FromString(TEXT("ARMORY"));
}

void ATU_CommandCenterStation::ConfigureStation(ETUCommandCenterStationType NewType, const FText& NewLabel, FName NewMissionId)
{
    StationType = NewType;
    StationLabel = NewLabel;
    MissionId = NewMissionId;
}

bool ATU_CommandCenterStation::IsOperatorInRange(const APawn* Pawn) const
{
    if (!Pawn)
    {
        return false;
    }

    if (InteractionVolume && InteractionVolume->IsOverlappingActor(Pawn))
    {
        return true;
    }

    return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(450.0f);
}

bool ATU_CommandCenterStation::UseStation(ATU_ArmedOperatorCharacter* Operator)
{
    if (!Operator || !IsOperatorInRange(Operator))
    {
        return false;
    }

    switch (StationType)
    {
        case ETUCommandCenterStationType::Armory:
        case ETUCommandCenterStationType::WeaponBench:
            return Operator->OpenArmoryView(ETUArmoryViewMode::Weapons);

        case ETUCommandCenterStationType::Cage:
        case ETUCommandCenterStationType::UniformBench:
            return Operator->OpenArmoryView(ETUArmoryViewMode::Gear);

        case ETUCommandCenterStationType::Briefing:
        case ETUCommandCenterStationType::MissionLaunch:
        {
            FName BriefingMission = MissionId.IsNone() ? FName(TEXT("OP_KILLHOUSE")) : MissionId;
            FText BriefingTitle = StationType == ETUCommandCenterStationType::MissionLaunch
                ? FText::FromString(TEXT("Deployment Ready Check"))
                : (StationLabel.IsEmpty() ? FText::FromString(TEXT("Operation Briefing")) : StationLabel);

            if (MissionPackage)
            {
                BriefingMission = MissionPackage->Mission.MissionId;
                BriefingTitle = MissionPackage->Mission.MissionTitle;

                if (ATU_PlayerController* PC = Cast<ATU_PlayerController>(Operator->GetController()))
                {
                    if (UTUMX50TabletComponent* Tablet = PC->GetMX50Tablet())
                    {
                        Tablet->ApplyMissionPackage(MissionPackage);
                    }
                }
            }

            return Operator->OpenBriefing(BriefingMission, BriefingTitle);
        }

        case ETUCommandCenterStationType::TestRange:
            Operator->CloseArmory();
            Operator->CloseBriefing();
            return Operator->EquipWeaponSlot(ETUOperatorWeaponSlot::Primary);

        default:
            return false;
    }
}
