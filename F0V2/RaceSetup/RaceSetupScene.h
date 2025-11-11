#pragma once
#include "TY/ActorHandle.h"

namespace RaceSetup
{
    class RaceSetupScene : public ActorHandle
    {
    public:
        RaceSetupScene();

        void init();

        bool isConfirmed() const;

        std::string selectedCourseFilepath() const;

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
