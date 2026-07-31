#pragma once

#include "IRaceEffectSystem.h"
#include "TY/Color.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct BoostTrailEffectSpawnParams
    {
        Float3 worldPosition{};
        ColorF32 color{1.0f};
        float intensity{1.0f};
    };

    class BoostTrailEffectSystem : public IRaceEffectSystem
    {
    public:
        BoostTrailEffectSystem();

        void emit(const BoostTrailEffectSpawnParams& params);

        void onRegistered() override;
        void update(const RaceEffectFrameContext& context) override;
        void drawTransparent() const override;
        void onUnregistered() override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    struct DriftSparkEffectSpawnParams
    {
        Float3 worldPosition{};
        Float3 velocity{};
    };

    class DriftSparkEffectSystem : public IRaceEffectSystem
    {
    public:
        DriftSparkEffectSystem();

        void emit(const DriftSparkEffectSpawnParams& params);

        void onRegistered() override;
        void update(const RaceEffectFrameContext& context) override;
        void drawTransparent() const override;
        void onUnregistered() override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    struct CollisionRingEffectSpawnParams
    {
        Float3 worldPosition{};
        ColorF32 color{1.0f};
    };

    class CollisionRingEffectSystem : public IRaceEffectSystem
    {
    public:
        CollisionRingEffectSystem();

        void emit(const CollisionRingEffectSpawnParams& params);

        void onRegistered() override;
        void update(const RaceEffectFrameContext& context) override;
        void drawTransparent() const override;
        void onUnregistered() override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
