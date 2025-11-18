#pragma once

namespace Race
{
    class MachineEffectDrawer
    {
    public:
        MachineEffectDrawer();

        void init();

        void finalize();

        void update();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
