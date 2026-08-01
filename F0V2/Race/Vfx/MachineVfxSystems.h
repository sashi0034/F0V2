#pragma once

#include "IRaceVfxSystem.h"
#include "TY/Color.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct DriftSparkVfxSpawnParams
    {
        Float3 worldPosition{};
        Float3 velocity{};
    };

    class DriftSparkVfxSystem : public IRaceVfxSystem
    {
    public:
        DriftSparkVfxSystem();

        void emit(const DriftSparkVfxSpawnParams& params);

        void onRegistered() override;
        void update(const RaceVfxFrameContext& context) override;
        void drawTransparent() const override;
        void onUnregistered() override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    struct CollisionRingVfxSpawnParams
    {
        Float3 worldPosition{};
        ColorF32 color{1.0f};
    };

    class CollisionRingVfxSystem : public IRaceVfxSystem
    {
    public:
        CollisionRingVfxSystem();

        void emit(const CollisionRingVfxSpawnParams& params);

        void onRegistered() override;
        void update(const RaceVfxFrameContext& context) override;
        void drawTransparent() const override;
        void onUnregistered() override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
