#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

/** Pure rules shared by authoritative combat and automation tests. All distances use cm. */
namespace DownfallCombatRules
{
    constexpr std::uint8_t NeutralTeam = 255;

    inline bool CanDamage(const std::uint8_t SourceTeam, const std::uint8_t TargetTeam, const bool bSameActor,
        const bool bAllowSelfDamage, const bool bAllowFriendlyFire)
    {
        if (bSameActor) return bAllowSelfDamage;
        return bAllowFriendlyFire || SourceTeam == NeutralTeam || TargetTeam == NeutralTeam || SourceTeam != TargetTeam;
    }

    inline bool CanFireAtTime(const double Now, const double EarliestShotTime)
    {
        return std::isfinite(Now) && std::isfinite(EarliestShotTime) && Now >= EarliestShotTime;
    }

    inline std::int32_t RoundsToReload(const std::int32_t Magazine, const std::int32_t Capacity, const std::int32_t Reserve)
    {
        const std::int32_t SafeCapacity = std::max(0, Capacity);
        const std::int32_t SafeMagazine = std::clamp(Magazine, 0, SafeCapacity);
        return std::min(SafeCapacity - SafeMagazine, std::max(0, Reserve));
    }

    /** Costs one token. Rejected requests never move LastUpdate backwards. */
    inline bool ConsumeRequestToken(const double Now, double& LastUpdate, float& Tokens,
        const float Capacity = 12.f, const float TokensPerSecond = 8.f)
    {
        if (!std::isfinite(Now) || !std::isfinite(LastUpdate) || !std::isfinite(Tokens)
            || !std::isfinite(Capacity) || !std::isfinite(TokensPerSecond) || Capacity < 1.f || TokensPerSecond <= 0.f) return false;
        const double Elapsed = std::max(0.0, Now - LastUpdate);
        LastUpdate = std::max(LastUpdate, Now);
        Tokens = std::clamp(Tokens + static_cast<float>(Elapsed) * TokensPerSecond, 0.f, Capacity);
        if (Tokens < 1.f) return false;
        Tokens -= 1.f;
        return true;
    }
}
