#pragma once
#include "TY_Extension/GameObjectHandle.h"

namespace Editor
{
    class EditorNodeTool : public GameObjectHandle
    {
    public:
        EditorNodeTool();

        void init() override;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

        void drawGBuffer() const;

        void debugUI() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
