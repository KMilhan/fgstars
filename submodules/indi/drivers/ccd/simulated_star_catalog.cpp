#include "simulated_star_catalog.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace
{

constexpr double kPi = 3.14159265358979323846;

struct StarLayer
{
    double cellSizeDegrees;
    int starsPerCell;
    double minMagnitude;
    double maxMagnitude;
};

constexpr std::array kLayers =
{
    StarLayer{8.0, 1, 0.5, 6.5},
    StarLayer{2.0, 1, 3.5, 9.5},
    StarLayer{0.5, 1, 6.5, 12.5},
    StarLayer{0.2, 2, 9.0, 16.5},
};

constexpr double degreesToRadians(double degrees)
{
    return degrees * kPi / 180.0;
}

double wrapDegrees(double degrees)
{
    degrees = std::fmod(degrees, 360.0);
    if (degrees < 0)
        degrees += 360.0;
    return degrees;
}

double shortestDeltaDegrees(double first, double second)
{
    double delta = wrapDegrees(first) - wrapDegrees(second);
    if (delta > 180.0)
        delta -= 360.0;
    else if (delta < -180.0)
        delta += 360.0;
    return delta;
}

int wrapIndex(int index, int size)
{
    index %= size;
    if (index < 0)
        index += size;
    return index;
}

std::uint64_t mix(std::uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::uint64_t starSeed(int layerIndex, int raIndex, int decIndex, int starIndex)
{
    std::uint64_t seed = 0x6d2b79f5aa11d985ULL;
    seed ^= mix(static_cast<std::uint64_t>(layerIndex + 1));
    seed ^= mix(static_cast<std::uint64_t>(raIndex + 1) << 21);
    seed ^= mix(static_cast<std::uint64_t>(decIndex + 1) << 42);
    seed ^= mix(static_cast<std::uint64_t>(starIndex + 1) << 7);
    return mix(seed);
}

std::uint64_t fallbackSeed(const INDI::SimulatorStarCatalog::Query &query)
{
    const auto raKey = static_cast<std::uint64_t>(std::llround(wrapDegrees(query.centerRaDegrees) * 3600.0));
    const auto decKey = static_cast<std::uint64_t>(std::llround((query.centerDecDegrees + 90.0) * 3600.0));
    const auto radiusKey = static_cast<std::uint64_t>(std::llround(query.radiusArcMinutes * 100.0));
    const auto limitKey = static_cast<std::uint64_t>(std::llround(query.limitingMagnitude * 100.0));

    std::uint64_t seed = 0xa0761d6478bd642fULL;
    seed ^= mix(raKey);
    seed ^= mix(decKey << 1);
    seed ^= mix(radiusKey << 2);
    seed ^= mix(limitKey << 3);
    return mix(seed);
}

double randomUnit(std::uint64_t seed)
{
    constexpr double scale = 1.0 / static_cast<double>(1ULL << 53);
    return static_cast<double>(mix(seed) >> 11) * scale;
}

double angularDistanceDegrees(double ra1Degrees, double dec1Degrees, double ra2Degrees, double dec2Degrees)
{
    const double dec1 = degreesToRadians(dec1Degrees);
    const double dec2 = degreesToRadians(dec2Degrees);
    const double deltaRa = degreesToRadians(shortestDeltaDegrees(ra1Degrees, ra2Degrees));
    const double cosineDistance = std::clamp(std::sin(dec1) * std::sin(dec2)
                                  + std::cos(dec1) * std::cos(dec2) * std::cos(deltaRa), -1.0, 1.0);
    return std::acos(cosineDistance) * 180.0 / kPi;
}

void appendLayerStars(std::vector<INDI::SimulatorStarCatalog::Star> *stars,
                      const INDI::SimulatorStarCatalog::Query &query,
                      const StarLayer &layer,
                      int layerIndex)
{
    if (query.limitingMagnitude < layer.minMagnitude)
        return;

    const double radiusDegrees = std::max(query.radiusArcMinutes / 60.0, 0.05);
    const double centerDecRadians = degreesToRadians(query.centerDecDegrees);
    const double raPaddingDegrees = radiusDegrees / std::max(std::cos(centerDecRadians), 0.1) + layer.cellSizeDegrees;
    const double minDec = std::max(-90.0, query.centerDecDegrees - radiusDegrees - layer.cellSizeDegrees);
    const double maxDec = std::min(90.0, query.centerDecDegrees + radiusDegrees + layer.cellSizeDegrees);

    const int raCellCount = static_cast<int>(std::round(360.0 / layer.cellSizeDegrees));
    const int decCellCount = static_cast<int>(std::round(180.0 / layer.cellSizeDegrees));
    const int raStart = static_cast<int>(std::floor((query.centerRaDegrees - raPaddingDegrees) / layer.cellSizeDegrees));
    const int raEnd = static_cast<int>(std::floor((query.centerRaDegrees + raPaddingDegrees) / layer.cellSizeDegrees));
    const int decStart = std::clamp(static_cast<int>(std::floor((minDec + 90.0) / layer.cellSizeDegrees)), 0, decCellCount - 1);
    const int decEnd = std::clamp(static_cast<int>(std::floor((maxDec + 90.0) / layer.cellSizeDegrees)), 0, decCellCount - 1);

    for (int decIndex = decStart; decIndex <= decEnd; ++decIndex)
    {
        const double decLower = -90.0 + decIndex * layer.cellSizeDegrees;
        const double decUpper = std::min(90.0, decLower + layer.cellSizeDegrees);
        for (int raIndex = raStart; raIndex <= raEnd; ++raIndex)
        {
            const int wrappedRaIndex = wrapIndex(raIndex, raCellCount);
            for (int starIndex = 0; starIndex < layer.starsPerCell; ++starIndex)
            {
                const std::uint64_t seed = starSeed(layerIndex, wrappedRaIndex, decIndex, starIndex);
                const double raDegrees = wrapDegrees((wrappedRaIndex + randomUnit(seed ^ 0x1111111111111111ULL))
                                                     * layer.cellSizeDegrees);
                const double decDegrees = decLower
                                          + randomUnit(seed ^ 0x2222222222222222ULL) * (decUpper - decLower);
                const double magnitudeFraction = std::sqrt(randomUnit(seed ^ 0x3333333333333333ULL));
                const double magnitude = layer.minMagnitude
                                         + (layer.maxMagnitude - layer.minMagnitude) * magnitudeFraction;

                if (magnitude > query.limitingMagnitude)
                    continue;

                if (angularDistanceDegrees(query.centerRaDegrees, query.centerDecDegrees, raDegrees, decDegrees) > radiusDegrees)
                    continue;

                stars->push_back({raDegrees, decDegrees, static_cast<float>(magnitude)});
            }
        }
    }
}

void appendFallbackStars(std::vector<INDI::SimulatorStarCatalog::Star> *stars,
                         const INDI::SimulatorStarCatalog::Query &query)
{
    const double radiusDegrees = std::max(query.radiusArcMinutes / 60.0, 0.05);
    const std::size_t targetCount = std::clamp<std::size_t>(static_cast<std::size_t>(std::ceil(radiusDegrees * 24.0)), 4, 12);

    if (stars->size() >= targetCount || query.limitingMagnitude <= 1.0)
        return;

    const std::uint64_t seed = fallbackSeed(query);
    const double centerDecRadians = degreesToRadians(query.centerDecDegrees);
    const double raScale = std::max(std::cos(centerDecRadians), 0.1);

    for (std::size_t index = stars->size(); index < targetCount; ++index)
    {
        const auto entrySeed = seed ^ mix(static_cast<std::uint64_t>(index + 1));
        const double angle = 2.0 * kPi * randomUnit(entrySeed ^ 0x4444444444444444ULL);
        const double distance = radiusDegrees * (0.2 + 0.65 * randomUnit(entrySeed ^ 0x5555555555555555ULL));
        const double raDegrees = wrapDegrees(query.centerRaDegrees + std::cos(angle) * distance / raScale);
        const double decDegrees = std::clamp(query.centerDecDegrees + std::sin(angle) * distance, -89.95, 89.95);
        const double magnitude = std::clamp(query.limitingMagnitude - (0.7 + 2.5 * randomUnit(entrySeed ^ 0x6666666666666666ULL)),
                                            0.5, query.limitingMagnitude - 0.1);

        stars->push_back({raDegrees, decDegrees, static_cast<float>(magnitude)});
    }
}

}

namespace INDI::SimulatorStarCatalog
{

std::vector<Star> query(const Query &query)
{
    std::vector<Star> stars;
    stars.reserve(256);

    for (int layerIndex = 0; layerIndex < static_cast<int>(kLayers.size()); ++layerIndex)
        appendLayerStars(&stars, query, kLayers[layerIndex], layerIndex);

    appendFallbackStars(&stars, query);

    std::sort(stars.begin(), stars.end(), [](const Star &left, const Star &right)
    {
        if (left.magnitude != right.magnitude)
            return left.magnitude < right.magnitude;

        if (left.raDegrees != right.raDegrees)
            return left.raDegrees < right.raDegrees;

        return left.decDegrees < right.decDegrees;
    });

    constexpr std::size_t maxStars = 3000;
    if (stars.size() > maxStars)
        stars.resize(maxStars);

    return stars;
}

}
