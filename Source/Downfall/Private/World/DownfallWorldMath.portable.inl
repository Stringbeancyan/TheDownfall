// Compile explicitly as C++17; UBT does not discover this .inl as a translation unit.
#include "World/DownfallWorldMath.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using namespace DownfallWorldMath;
    assert(WrapHours(-1) == 23);
    assert(WrapHours(24) == 0);
    assert(WrapHours(std::numeric_limits<double>::quiet_NaN()) == 0);
    assert(EvaluateHours(23, 1075, 1000, 1800) == 0);
    assert(EvaluateHours(25, 800, 0, 0) == 1);
    // Absolute server anchors must give the same clock regardless of join time.
    for (int Second = 0; Second < 86400; ++Second)
    {
        const double Hour = EvaluateHours(2, Second, 0, 1800);
        assert(Hour >= 0 && Hour < 24);
        const double Reanchored = EvaluateHours(EvaluateHours(2, Second - 5, 0, 1800), Second, Second - 5, 1800);
        const double Difference = std::abs(Hour - Reanchored);
        assert(Difference < 1.e-8 || std::abs(Difference - 24) < 1.e-8);
    }
    assert(SolarPosition(12, 0, 80).AltitudeDegrees > 88);
    assert(SolarPosition(0, 0, 80).AltitudeDegrees < -88);
    assert(std::abs(SolarPosition(6, 0, 80).AzimuthDegrees - 90) < 2);
    assert(std::abs(SolarPosition(12, 41, 172).AzimuthDegrees - 180) < 1);
    assert(SolarPosition(12, 41, 172).AltitudeDegrees > SolarPosition(12, 41, 355).AltitudeDegrees + 40);
    int Positions = 0;
    for (int Day = 1; Day <= 365; ++Day)
        for (int Hour = 0; Hour < 24; ++Hour)
            for (int Latitude = -89; Latitude <= 89; ++Latitude)
            {
                const auto Sun = SolarPosition(Hour, Latitude, Day);
                assert(std::isfinite(Sun.AltitudeDegrees) && std::abs(Sun.AltitudeDegrees) <= 90);
                assert(std::isfinite(Sun.AzimuthDegrees) && Sun.AzimuthDegrees >= 0 && Sun.AzimuthDegrees < 360);
                ++Positions;
            }
    assert(std::abs(ApproximateMoonPhase(29.53059, .5) - .5) < 1.e-9);
    assert(ApproximateMoonPhase(-50, 0) >= 0 && ApproximateMoonPhase(-50, 0) < 1);
    std::cout << "PASS: 86400 clock reanchor cases; " << Positions
        << " bounded solar positions; equinox, season, sunrise and approximate lunar phase checks.\n";
}
