#pragma once
#include "CB/Lambert.h"
#include "TY/BasicCamera3D.h"
#include "TY/ConstantBufferWrapper.h"

namespace Race
{
    struct RaceContextContent
    {
        BasicCamera3D camera{};

        struct
        {
            ConstantBufferWrapper<Lambert_b10> lambert{};
        } cb;
    };
}
