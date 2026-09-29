#include "TUHandlingCapture.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTUHandlingCaptureSafetyTest, "TheUnit.Handling.CaptureSafety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTUHandlingCaptureSafetyTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("unique run label"), ATUHandlingCapture::IsSafeCaptureId(TEXT("handling_60_a1")));
    TestFalse(TEXT("traversal cannot escape evidence directory"), ATUHandlingCapture::IsSafeCaptureId(TEXT("../../manual")));
    TestFalse(TEXT("absolute path rejected"), ATUHandlingCapture::IsSafeCaptureId(TEXT("C:\\manual")));
    TestFalse(TEXT("empty capture id rejected"), ATUHandlingCapture::IsSafeCaptureId(TEXT("")));
    TestFalse(TEXT("JSON injection rejected"), ATUHandlingCapture::IsSafeCaptureId(TEXT("bad\"id")));
    for (int32 Rate : {30, 60, 144}) TestTrue(TEXT("required simulated rate"), ATUHandlingCapture::IsSupportedRate(Rate));
    TestFalse(TEXT("zero rate rejected"), ATUHandlingCapture::IsSupportedRate(0));
    return true;
}
#endif
