#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    class CourseTextureRenderer : public ActorHandle
    {
    public:
        CourseTextureRenderer();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
