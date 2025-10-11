#pragma once
#include "TY_Extension/GameObjectHandle.h"

namespace F0V2
{
    class GameFlowchart : public GameObjectHandle
    {
    public:
        GameFlowchart();

        void init() override;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
