#pragma once

namespace ZG::detail
{
    class IEngineUpdatable
    {
    public:
        virtual ~IEngineUpdatable() = default;

        virtual void Update() = 0;
    };
}
