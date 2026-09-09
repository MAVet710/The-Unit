#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TU_DonetskArtema60Building.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUDonetskArtema60ReferenceFactsTest,
    "TheUnit.Maps.Donetsk.Artema60.ReferenceFacts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTUDonetskArtema60ReferenceFactsTest::RunTest(const FString& Parameters)
{
    const ATU_DonetskArtema60Building* CDO = GetDefault<ATU_DonetskArtema60Building>();
    if (!TestNotNull(TEXT("Artema 60 reference actor CDO"), CDO))
    {
        return false;
    }

    TestEqual(TEXT("Documented construction year"), CDO->GetDocumentedConstructionYear(), 1928);
    TestEqual(TEXT("Documented original storeys"), CDO->GetDocumentedOriginalStoreys(), 3);
    TestEqual(TEXT("Documented current storeys"), CDO->GetDocumentedCurrentStoreys(), 4);

    TestTrue(TEXT("Working frontage is positive"), CDO->GetEstimatedFrontageCm() > 0.0f);
    TestTrue(TEXT("Working depth is positive"), CDO->GetEstimatedDepthCm() > 0.0f);
    TestTrue(TEXT("Frontage remains wider than depth"), CDO->GetEstimatedFrontageCm() > CDO->GetEstimatedDepthCm());

    return true;
}

#endif
