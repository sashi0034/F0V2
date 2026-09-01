#pragma once
#include "TY/BasicCamera3D.h"

namespace Race
{
    // TODO: g_sharedState に統一
    struct RaceContextContent
    {
        BasicCamera3D camera{};
    };
}
