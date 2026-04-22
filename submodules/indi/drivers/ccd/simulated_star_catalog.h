#pragma once

#include <vector>

namespace INDI::SimulatorStarCatalog
{

struct Star
{
    double raDegrees { 0 };
    double decDegrees { 0 };
    float magnitude { 0 };
};

struct Query
{
    double centerRaDegrees { 0 };
    double centerDecDegrees { 0 };
    double radiusArcMinutes { 0 };
    double limitingMagnitude { 0 };
};

std::vector<Star> query(const Query &query);

}
