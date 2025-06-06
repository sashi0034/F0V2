#pragma once

namespace TY::detail
{
    class IEngineUpdatable
    {
    public:
        virtual ~IEngineUpdatable() = default;

        virtual void Update() = 0;
    };
}
