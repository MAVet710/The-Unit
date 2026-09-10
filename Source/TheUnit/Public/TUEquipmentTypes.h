#pragma once

#include "CoreMinimal.h"
#include "TUEquipmentTypes.generated.h"

/** Stable tactical gear slots used by operator loadouts and UI. */
UENUM(BlueprintType)
enum class ETUEquipmentSlot : uint8
{
    Headwear UMETA(DisplayName="Headwear"),
    Headset UMETA(DisplayName="Headset"),
    Eyewear UMETA(DisplayName="Eyewear"),
    Facewear UMETA(DisplayName="Facewear"),
    NVG UMETA(DisplayName="NVG"),
    TorsoArmor UMETA(DisplayName="Torso Armor"),
    ChestRig UMETA(DisplayName="Chest Rig"),
    Backpack UMETA(DisplayName="Backpack"),
    Belt UMETA(DisplayName="Battle Belt"),
    LeftHip UMETA(DisplayName="Left Hip"),
    RightHip UMETA(DisplayName="Right Hip"),
    Gloves UMETA(DisplayName="Gloves"),
    KneePads UMETA(DisplayName="Knee Pads"),
    Footwear UMETA(DisplayName="Footwear"),
    Accessory UMETA(DisplayName="Accessory")
};
