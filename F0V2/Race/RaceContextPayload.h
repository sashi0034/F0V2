#pragma once
#include "CB/Lambert.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/SimpleCamera3D.h"

namespace Race
{
    struct RaceContextPayload
    {
        SimpleCamera3D camera{};

        struct
        {
            ConstantBufferWrapper<Lambert_b10> lambert{};
        } cb;
    };
}
