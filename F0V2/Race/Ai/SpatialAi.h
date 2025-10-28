#pragma once
#include "SpatialTypes.h"
#include "TY/Array.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class SpatialAi : public GameObjectHandle
    {
    public:
        SpatialAi();

        void init() override;

        const SpatialData& data() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
