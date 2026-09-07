#include "TU_ModularOperatorCharacter.h"

#include "TUOperatorAppearanceData.h"
#include "TUOperatorEquipmentComponent.h"
#include "TUOperatorLoadoutData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

ATU_ModularOperatorCharacter::ATU_ModularOperatorCharacter()
{
    EquipmentComponent = CreateDefaultSubobject<UTUOperatorEquipmentComponent>(TEXT("OperatorEquipment"));
}

void ATU_ModularOperatorCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyOperatorAppearance();
}

void ATU_ModularOperatorCharacter::SetOperatorAppearance(UTUOperatorAppearanceData* NewAppearance)
{
    OperatorAppearance = NewAppearance;

    if (HasActorBegunPlay())
    {
        ApplyOperatorAppearance();
    }
}

void ATU_ModularOperatorCharacter::ApplyOperatorAppearance()
{
    USkeletalMeshComponent* ThirdPersonBody = GetMesh();

    if (OperatorAppearance)
    {
        if (ThirdPersonBody)
        {
            if (!OperatorAppearance->ThirdPersonBodyMesh.IsNull())
            {
                if (USkeletalMesh* BodyMesh = OperatorAppearance->ThirdPersonBodyMesh.LoadSynchronous())
                {
                    ThirdPersonBody->SetSkeletalMeshAsset(BodyMesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to load operator body mesh for appearance '%s'."), *OperatorAppearance->GetName());
                }
            }

            ThirdPersonBody->SetRelativeTransform(OperatorAppearance->ThirdPersonMeshTransform);
            ThirdPersonBody->SetOwnerNoSee(OperatorAppearance->bHideThirdPersonBodyFromOwner);

            if (OperatorAppearance->ThirdPersonAnimClass)
            {
                ThirdPersonBody->SetAnimInstanceClass(OperatorAppearance->ThirdPersonAnimClass);
            }
        }

        if (FirstPersonArmsMesh)
        {
            if (!OperatorAppearance->FirstPersonArmsMesh.IsNull())
            {
                if (USkeletalMesh* ArmsMesh = OperatorAppearance->FirstPersonArmsMesh.LoadSynchronous())
                {
                    FirstPersonArmsMesh->SetSkeletalMeshAsset(ArmsMesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to load first-person arms mesh for appearance '%s'."), *OperatorAppearance->GetName());
                }
            }

            FirstPersonArmsMesh->SetRelativeTransform(OperatorAppearance->FirstPersonArmsTransform);
            FirstPersonArmsMesh->SetOnlyOwnerSee(true);

            if (OperatorAppearance->FirstPersonAnimClass)
            {
                FirstPersonArmsMesh->SetAnimInstanceClass(OperatorAppearance->FirstPersonAnimClass);
            }
        }
    }

    if (EquipmentComponent)
    {
        EquipmentComponent->InitializeVisualRoots(ThirdPersonBody, FirstPersonArmsMesh);

        if (OperatorAppearance && OperatorAppearance->DefaultLoadout)
        {
            EquipmentComponent->ApplyLoadout(OperatorAppearance->DefaultLoadout);
        }
    }
}
