#pragma once
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class ToyCinnamon : public GameObjectHandle
    {
    public:
        ToyCinnamon();

        void init() override;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
