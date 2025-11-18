#pragma once

namespace Race
{
    struct RaceDrawParameters
    {
        // TODO: skipShadowMap など
    };

    class IRaceDrawer
    {
    public:
        virtual ~IRaceDrawer() = default;

        virtual void prepareDrawParameters(RaceDrawParameters& config, bool init) const
        {
        }

        virtual void drawShadowMap() const
        {
        }

        virtual void drawGBuffer() const
        {
        }

        virtual void drawTransparent() const
        {
        }

        virtual void drawUI() const
        {
        }
    };
}
