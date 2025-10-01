#pragma once
#include "TY/SimpleCamera3D.h"

namespace Race
{
    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual SimpleCamera3D& camera() = 0;
        virtual const SimpleCamera3D& camera() const = 0;
    };

    IRaceContext& GetRaceContext();
}
