#include "World/DownfallWorldMath.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallClockWrapTest, "Downfall.World.ClockWrap",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDownfallClockWrapTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Negative hours wrap into preceding day"), DownfallWorldMath::WrapHours(-1), 23.0);
    TestEqual(TEXT("Midnight wraps to zero"), DownfallWorldMath::WrapHours(24), 0.0);
    TestEqual(TEXT("Many-day input wraps"), DownfallWorldMath::WrapHours(2402), 2.0);
    TestEqual(TEXT("Late join evaluates server anchor, not local start time"),
        DownfallWorldMath::EvaluateHours(23, 1075, 1000, 1800), 0.0);
    TestEqual(TEXT("Invalid day length freezes at wrapped anchor"),
        DownfallWorldMath::EvaluateHours(25, 800, 0, 0), 1.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallSolarTest, "Downfall.World.SolarPosition",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDownfallSolarTest::RunTest(const FString& Parameters)
{
    const auto EquinoxNoon = DownfallWorldMath::SolarPosition(12, 0, 80);
    TestTrue(TEXT("Equatorial equinox noon near zenith"), EquinoxNoon.AltitudeDegrees > 88.0);
    const auto EquinoxMidnight = DownfallWorldMath::SolarPosition(0, 0, 80);
    TestTrue(TEXT("Equatorial midnight sun below horizon"), EquinoxMidnight.AltitudeDegrees < -88.0);
    const auto NorthernSummer = DownfallWorldMath::SolarPosition(12, 41, 172);
    const auto NorthernWinter = DownfallWorldMath::SolarPosition(12, 41, 355);
    TestTrue(TEXT("Latitude and season lower winter sun"), NorthernSummer.AltitudeDegrees > NorthernWinter.AltitudeDegrees + 40.0);
    TestTrue(TEXT("Northern noon faces south"), FMath::Abs(NorthernSummer.AzimuthDegrees - 180.0) < 1.0);
    TestTrue(TEXT("Sunrise points east"), FMath::Abs(DownfallWorldMath::SolarPosition(6,0,80).AzimuthDegrees - 90.0) < 2.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallMoonTest, "Downfall.World.ApproximateMoonPhase",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDownfallMoonTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Synodic month returns initial phase"), FMath::IsNearlyEqual(DownfallWorldMath::ApproximateMoonPhase(29.53059,.5), .5, 1.e-9));
    TestTrue(TEXT("Negative days remain normalized"), DownfallWorldMath::ApproximateMoonPhase(-50,0) >= 0.0);
    return true;
}
#endif
