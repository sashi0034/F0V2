#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    class CourseDynamicTextureDrawer : public ActorHandle
    {
    public:
        CourseDynamicTextureDrawer();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
