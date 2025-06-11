#pragma once
#include "Mat4x4.h"
#include "System.h"

namespace TY
{
    class SimpleCamera3D
    {
    public:
        SimpleCamera3D();

        void reset();

        void reset(Float3 eyePosition, Float3 targetPosition = {}, Float3 upDirection = {0, 1, 0});

        void update(float dt = System::DeltaTime());

        Float3 eyePosition() const;

        Float3 targetPosition() const;

        const Mat4x4& matrix() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
