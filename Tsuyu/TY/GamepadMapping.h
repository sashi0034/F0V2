#pragma once

namespace TY
{
    struct GamepadMapping
    {
        // int dpad_up{0};
        // int dpad_down{1};
        // int dpad_left{2};
        // int dpad_right{3};
        int a{0};
        int b{1};
        int x{2};
        int y{3};
        int lb{4};
        int rb{5};
        int lt{6}; // Set to -1 when axis_trigger is enabled.
        int rt{7}; // Set to -1 when axis_trigger is enabled.
        int menu{8};
        int view{9};
        int axis_lx{0}; // TODO: axis_lx_inverted
        int axis_ly{1};
        int axis_rx{2};
        int axis_ry{3};
        int axis_trigger{-1}; // Set to -1 when lt and rt are enabled.
        bool axis_trigger_inverted{};

        void writeToTomlFile(const std::string& path) const;

        static GamepadMapping FromTomlFile(const std::string& path);
    };
}
