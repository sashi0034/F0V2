#pragma once
#include "TY/ActorHandle.h"

namespace Editor
{
    class EditorPlayground : public ActorHandle
    {
    public:
        EditorPlayground();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
