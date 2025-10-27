#pragma once

namespace Race
{
    class RaceCameraController
    {
    public:
        RaceCameraController();

        void update();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
