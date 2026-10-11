#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/DownfallCombatRules.h"
#include "Combat/DownfallWeaponDefinition.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallCombatCadenceTest, "Downfall.Combat.CadenceSurvivesTriggerSpam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownfallCombatCadenceTest::RunTest(const FString& Parameters)
{
    double Deadline = 0.0;
    int32 ShotCount = 0;
    // 10,000 input transitions over one second may not exceed the 10 Hz weapon cadence.
    for (int32 Step = 0; Step < 10000; ++Step)
    {
        const double Now = Step * 0.0001;
        if (DownfallCombatRules::CanFireAtTime(Now, Deadline))
        {
            ++ShotCount;
            Deadline = Now + 0.1;
        }
    }
    TestEqual(TEXT("Rapid trigger/equip requests cannot buy additional fire time"), ShotCount, 10);
    TestFalse(TEXT("Clock cannot move backwards past deadline"), DownfallCombatRules::CanFireAtTime(-1.0, Deadline));
    TestTrue(TEXT("Exact deadline is allowed"), DownfallCombatRules::CanFireAtTime(Deadline, Deadline));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallReloadConservationTest, "Downfall.Combat.ReloadConservesAmmunition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownfallReloadConservationTest::RunTest(const FString& Parameters)
{
    for (int32 Capacity = 1; Capacity <= 40; ++Capacity)
    {
        for (int32 Magazine = 0; Magazine <= Capacity; ++Magazine)
        {
            for (int32 Reserve = 0; Reserve <= 60; ++Reserve)
            {
                const int32 Transfer = DownfallCombatRules::RoundsToReload(Magazine, Capacity, Reserve);
                const int32 NewMagazine = Magazine + Transfer;
                const int32 NewReserve = Reserve - Transfer;
                if (NewMagazine > Capacity || NewReserve < 0 || Transfer < 0 ||
                    Magazine + Reserve != NewMagazine + NewReserve)
                {
                    AddError(FString::Printf(TEXT("Conservation failure: capacity=%d magazine=%d reserve=%d"), Capacity, Magazine, Reserve));
                    return false;
                }
            }
        }
    }
    TestEqual(TEXT("Negative reserve never creates rounds"), DownfallCombatRules::RoundsToReload(0, 30, -5), 0);
    TestEqual(TEXT("Overfilled magazine never loses or creates rounds"), DownfallCombatRules::RoundsToReload(31, 30, 5), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallTeamDamageTest, "Downfall.Combat.TeamDamageRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownfallTeamDamageTest::RunTest(const FString& Parameters)
{
    using namespace DownfallCombatRules;
    TestFalse(TEXT("Friendly fire disabled"), CanDamage(1, 1, false, false, false));
    TestTrue(TEXT("Enemy fire allowed"), CanDamage(1, 2, false, false, false));
    TestFalse(TEXT("Self damage disabled even with friendly fire enabled"), CanDamage(1, 1, true, false, true));
    TestTrue(TEXT("Explicit self damage overrides team rule"), CanDamage(1, 1, true, true, false));
    TestTrue(TEXT("Neutral cover can be damaged"), CanDamage(1, NeutralTeam, false, false, false));
    TestTrue(TEXT("Environmental damage has no team"), CanDamage(NeutralTeam, 1, false, false, false));
    TestTrue(TEXT("Friendly fire setting is respected"), CanDamage(2, 2, false, false, true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallRequestBudgetTest, "Downfall.Combat.RPCRequestBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownfallRequestBudgetTest::RunTest(const FString& Parameters)
{
    double LastUpdate = 5.0;
    float Tokens = 12.f;
    int32 Accepted = 0;
    for (int32 Index = 0; Index < 1000; ++Index)
        Accepted += DownfallCombatRules::ConsumeRequestToken(5.0, LastUpdate, Tokens) ? 1 : 0;
    TestEqual(TEXT("Burst allowance is bounded"), Accepted, 12);
    TestFalse(TEXT("Exhaustion rejects additional expensive work"), DownfallCombatRules::ConsumeRequestToken(5.0, LastUpdate, Tokens));
    TestTrue(TEXT("One eighth second recovers one token"), DownfallCombatRules::ConsumeRequestToken(5.125, LastUpdate, Tokens));
    TestFalse(TEXT("Recovered token spent exactly once"), DownfallCombatRules::ConsumeRequestToken(5.125, LastUpdate, Tokens));
    TestFalse(TEXT("Backwards time cannot replenish tokens"), DownfallCombatRules::ConsumeRequestToken(4.0, LastUpdate, Tokens));
    TestEqual(TEXT("Backwards request preserves clock"), LastUpdate, 5.125);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDownfallWeaponDefaultsTest, "Downfall.Combat.NativeWeaponDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownfallWeaponDefaultsTest::RunTest(const FString& Parameters)
{
    FDownfallWeaponSpec Rifle = FDownfallWeaponSpec::MakeRifle();
    FDownfallWeaponSpec Sidearm = FDownfallWeaponSpec::MakeSidearm();
    TestTrue(TEXT("Rifle runtime settings are valid"), Rifle.IsValid());
    TestTrue(TEXT("Sidearm runtime settings are valid"), Sidearm.IsValid());
    TestTrue(TEXT("Definitions use distinct identifiers"), Rifle.Id != Sidearm.Id);
    TestTrue(TEXT("Rifle supports held automatic fire"), Rifle.bAutomatic);
    TestFalse(TEXT("Sidearm requires each trigger press"), Sidearm.bAutomatic);
    Rifle.SecondsBetweenShots = 0.f;
    TestFalse(TEXT("Zero cadence rejected"), Rifle.IsValid());
    Sidearm.ProjectileLifetime = 1000.f;
    TestFalse(TEXT("Unbounded lifetime rejected"), Sidearm.IsValid());
    return true;
}

#endif
