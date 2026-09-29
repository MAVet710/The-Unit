#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TUAmmoCatalog.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUAmmoCatalogTest,"TheUnit.Combat.AmmoCatalog.BetaEight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTUAmmoCatalogTest::RunTest(const FString&)
{
    const auto A=UTUAmmoCatalog::BetaDefinitions(); TestEqual(TEXT("Eight beta ammunition types"),A.Num(),8);
    TSet<FName> Ids; for(const auto& V:A){TestFalse(TEXT("Unique ammo id"),Ids.Contains(V.AmmoId));Ids.Add(V.AmmoId);TestTrue(TEXT("Positive velocity"),V.Velocity>0.f);}
    FAmmoDefinition X; TestTrue(TEXT("Barrier variant resolves"),UTUAmmoCatalog::Find(TEXT("Ammo_TU556_Barrier"),X));
    return true;
}
#endif
