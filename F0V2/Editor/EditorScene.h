#pragma once
#include "TY/ActorHandle.h"

namespace Editor
{
    class EditorScene : public ActorHandle
    {
    public:
        EditorScene();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
