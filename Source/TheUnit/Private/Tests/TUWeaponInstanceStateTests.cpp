#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TUWeaponDefinitionCatalog.h"
#include "TUWeaponInstanceRules.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUWeaponInstanceStateTest,
    "TheUnit.Combat.WeaponInstanceState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUWeaponInstanceStateTest::RunTest(const FString& Parameters)
{
    UTUWeaponDefinitionCatalog* Catalog = NewObject<UTUWeaponDefinitionCatalog>();
    if (!TestNotNull(TEXT("Catalog object"), Catalog))
    {
        return false;
    }

    FWeaponDefinition BaseWeapon;
    BaseWeapon.WeaponId = TEXT("WPN_InstanceTest");

    FAmmoDefinition Ammo;
    Ammo.AmmoId = TEXT("Ammo_InstanceTest");

    FWeaponPlatformDefinition Platform;
    Platform.PlatformId = TEXT("Platform_InstanceTest");
    Platform.BaseWeaponDefinitionId = BaseWeapon.WeaponId;
    Platform.CompatibleAmmoIds = { Ammo.AmmoId };

    Catalog->Platforms = { Platform };
    Catalog->BaseWeaponDefinitions = { BaseWeapon };
    Catalog->Ammunition = { Ammo };

    FWeaponBuildState SharedBuild;
    SharedBuild.PlatformId = Platform.PlatformId;
    SharedBuild.SelectedAmmoId = Ammo.AmmoId;

    FWeaponInstanceState First;
    First.InstanceId = FGuid::NewGuid();
    First.Build = SharedBuild;
    First.MagazineState.Capacity = 30;
    First.MagazineState.RoundsInMagazine = 29;
    First.MagazineState.bRoundChambered = true;
    First.AmmoReserve = 90;
    First.ConditionNormalized = 0.92f;

    FWeaponInstanceState Second = First;
    Second.InstanceId = FGuid::NewGuid();
    Second.ConditionNormalized = 0.61f;

    FString FailureReason;
    TestTrue(TEXT("First instance validates"),
        FTUWeaponInstanceRules::ValidateInstance(First, Catalog, FailureReason));
    TestTrue(TEXT("Second instance with same build validates independently"),
        FTUWeaponInstanceRules::ValidateInstance(Second, Catalog, FailureReason));
    TestTrue(TEXT("Same build can belong to different persistent instances"),
        First.InstanceId != Second.InstanceId);
    TestEqual(TEXT("Shared build identity remains the same"),
        First.Build.PlatformId, Second.Build.PlatformId);

    FWeaponInstanceState Invalid = First;
    Invalid.InstanceId.Invalidate();
    TestFalse(TEXT("Missing persistent identity is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(Invalid, Catalog, FailureReason));

    Invalid = First;
    Invalid.ConditionNormalized = 1.1f;
    TestFalse(TEXT("Out-of-range condition is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(Invalid, Catalog, FailureReason));

    Invalid = First;
    Invalid.MagazineState.RoundsInMagazine = 31;
    TestFalse(TEXT("Magazine over capacity is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(Invalid, Catalog, FailureReason));

    Invalid = First;
    Invalid.Build.SelectedAmmoId = NAME_None;
    TestFalse(TEXT("Loaded instance without ammo identity is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(Invalid, Catalog, FailureReason));

    Invalid = First;
    Invalid.Build.PlatformId = TEXT("Platform_Missing");
    TestFalse(TEXT("Instance with unresolved build is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(Invalid, Catalog, FailureReason));

    TestFalse(TEXT("Null definition catalog is rejected"),
        FTUWeaponInstanceRules::ValidateInstance(First, nullptr, FailureReason));

    return true;
}

#endif
