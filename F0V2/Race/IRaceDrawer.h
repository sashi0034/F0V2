#pragma once

namespace Race
{
    struct RaceDrawParameters
    {
        bool drawForward{};
        bool draw2D{};
    };

    class IRaceDrawer
    {
    public:
        virtual ~IRaceDrawer() = default;

        virtual void prepareDrawParameters(RaceDrawParameters& config, bool init) const = 0;

        virtual void drawGBuffer() const
        {
        }

        virtual void draw2D() const
        {
        }
    };
}
