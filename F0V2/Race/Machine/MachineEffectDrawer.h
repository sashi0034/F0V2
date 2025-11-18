#pragma once

namespace Race
{
    class MachineEffectDrawer
    {
    public:
        MachineEffectDrawer();

        void init();

        void update();

        void drawTransparent() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
