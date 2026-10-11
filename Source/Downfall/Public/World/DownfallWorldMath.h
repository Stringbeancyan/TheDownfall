#pragma once

#include <algorithm>
#include <cmath>

/** Pure environmental math. Hours are local SOLAR time, not civil time. */
namespace DownfallWorldMath
{
    inline constexpr double Pi = 3.14159265358979323846;
    inline constexpr double DegreesToRadians(double Degrees) { return Degrees * Pi / 180.0; }
    inline constexpr double RadiansToDegrees(double Radians) { return Radians * 180.0 / Pi; }

    inline double WrapHours(const double Hours)
    {
        if (!std::isfinite(Hours)) return 0.0;
        const double Remainder = std::fmod(Hours, 24.0);
        return Remainder < 0.0 ? Remainder + 24.0 : Remainder;
    }

    inline double EvaluateHours(const double AnchorHour, const double ServerSeconds,
        const double AnchorServerSeconds, const double SecondsPerDay)
    {
        if (!std::isfinite(SecondsPerDay) || SecondsPerDay <= 0.0) return WrapHours(AnchorHour);
        return WrapHours(AnchorHour + (ServerSeconds - AnchorServerSeconds) * 24.0 / SecondsPerDay);
    }

    struct FSolarPosition
    {
        double AltitudeDegrees = 0.0;
        double AzimuthDegrees = 0.0; // 0 = north (+X), 90 = east (+Y).
    };

    /** NOAA fractional-year declination approximation; no terrain/refraction correction.
     * Approximate within about a degree; local solar time avoids UTC/longitude ambiguity.
     */
    inline FSolarPosition SolarPosition(double SolarHour, double LatitudeDegrees, int DayOfYear)
    {
        SolarHour = WrapHours(SolarHour);
        LatitudeDegrees = std::clamp(LatitudeDegrees, -89.9, 89.9);
        DayOfYear = std::clamp(DayOfYear, 1, 365);
        const double Gamma = 2.0 * Pi / 365.0 * (DayOfYear - 1.0 + (SolarHour - 12.0) / 24.0);
        const double Declination = 0.006918 - 0.399912 * std::cos(Gamma)
            + 0.070257 * std::sin(Gamma) - 0.006758 * std::cos(2.0 * Gamma)
            + 0.000907 * std::sin(2.0 * Gamma) - 0.002697 * std::cos(3.0 * Gamma)
            + 0.00148 * std::sin(3.0 * Gamma);
        const double Latitude = DegreesToRadians(LatitudeDegrees);
        const double HourAngle = DegreesToRadians((SolarHour - 12.0) * 15.0);
        const double SinAltitude = std::sin(Latitude) * std::sin(Declination)
            + std::cos(Latitude) * std::cos(Declination) * std::cos(HourAngle);
        FSolarPosition Result;
        Result.AltitudeDegrees = RadiansToDegrees(std::asin(std::clamp(SinAltitude, -1.0, 1.0)));
        Result.AzimuthDegrees = std::fmod(RadiansToDegrees(std::atan2(
            std::sin(HourAngle), std::cos(HourAngle) * std::sin(Latitude)
                - std::tan(Declination) * std::cos(Latitude))) + 540.0, 360.0);
        return Result;
    }

    /** Artistic synodic approximation, not an astronomical ephemeris. 0=new, .5=full. */
    inline double ApproximateMoonPhase(const double ElapsedGameDays, const double InitialPhase)
    {
        const double Phase = std::fmod(InitialPhase + ElapsedGameDays / 29.53059, 1.0);
        return Phase < 0.0 ? Phase + 1.0 : Phase;
    }
}
