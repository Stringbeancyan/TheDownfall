// Portable executable check for the SAME rules used by server combat.
// This .inl is intentionally not an Unreal translation unit. From the project root:
// c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -I Source/Downfall/Public -x c++ Source/Downfall/Private/Combat/DownfallCombatRules.portable.inl -o /tmp/downfall-combat-rules
// /tmp/downfall-combat-rules
#include "Combat/DownfallCombatRules.h"
#include <cassert>
#include <iostream>
#include <limits>

int main()
{
    using namespace DownfallCombatRules;
    int Checks = 0;
    for (int Capacity = 1; Capacity <= 200; ++Capacity)
        for (int Magazine = 0; Magazine <= Capacity; ++Magazine)
            for (int Reserve = 0; Reserve <= 200; ++Reserve)
            {
                const int Transfer = RoundsToReload(Magazine, Capacity, Reserve);
                assert(Transfer >= 0 && Magazine + Transfer <= Capacity && Reserve - Transfer >= 0);
                assert((Magazine + Transfer) + (Reserve - Transfer) == Magazine + Reserve);
                ++Checks;
            }
    assert(RoundsToReload(2147483647, -2147483647, 4) == 0);
    assert(RoundsToReload(-2147483647, 2147483647, 2147483647) == 2147483647);
    double Deadline = 0.;
    int Shots = 0;
    for (int Step = 0; Step < 10000; ++Step)
    {
        const double Now = Step * 0.0001;
        if (CanFireAtTime(Now, Deadline)) { ++Shots; Deadline = Now + 0.1; }
    }
    assert(Shots == 10);
    assert(!CanFireAtTime(std::numeric_limits<double>::quiet_NaN(), 0));
    assert(!CanFireAtTime(1, std::numeric_limits<double>::infinity()));
    double LastUpdate = 5.;
    float Tokens = 12.f;
    int Accepted = 0;
    for (int Request = 0; Request < 1000; ++Request)
        Accepted += ConsumeRequestToken(5., LastUpdate, Tokens) ? 1 : 0;
    assert(Accepted == 12);
    assert(ConsumeRequestToken(5.125, LastUpdate, Tokens));
    assert(!ConsumeRequestToken(5.125, LastUpdate, Tokens));
    assert(!ConsumeRequestToken(4., LastUpdate, Tokens));
    assert(LastUpdate == 5.125);
    assert(!CanDamage(1, 1, false, false, false));
    assert(CanDamage(1, 2, false, false, false));
    assert(!CanDamage(1, 1, true, false, true));
    assert(CanDamage(1, 1, true, true, false));
    assert(CanDamage(NeutralTeam, 1, false, false, false));
    std::cout << "PASS: " << Checks << " ammo conservation cases; cadence, request budget, team rules, NaN and overflow guards.\n";
}
