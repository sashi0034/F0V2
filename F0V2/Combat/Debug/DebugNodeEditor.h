#pragma once
#include "TY_Extension/GameObjectHandle.h"

namespace Combat
{
    class DebugNodeEditor : public GameObjectHandle
    {
    public:
        DebugNodeEditor();

        void init() override;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
