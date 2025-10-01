#pragma once
#include "TY/TriangleBvh.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class StageManager : public GameObjectHandle
    {
    public:
        StageManager();

        void init() override;

        TriangleBvh& staticBvh();
        const TriangleBvh& staticBvh() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
