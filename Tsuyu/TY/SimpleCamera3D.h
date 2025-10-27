#pragma once
#include "Mat4x4.h"
#include "System.h"
#include "Vector2D.h"

namespace TY
{
    class SimpleCamera3D
    {
    public:
        SimpleCamera3D();

        void reset();

        void reset(const Float3& eyePosition, const Float3& targetPosition = {}, Float3 upDirection = {0, 1, 0});

        void setEyeAndTarget(const Float3& eyePosition, const Float3& targetPosition);

        void transform(float dt, const Float3& moveVector, const Float2& rotateVector);

        void transformBySimpleInput(float dt = System::DeltaTime(), float moveSpeed = 10.0f, float rotateSpeed = 50.0f);

        [[nodiscard]]
        Float3 eyePosition() const;

        [[nodiscard]]
        Float3 targetPosition() const;

        [[nodiscard]]
        Float3 upDirection() const;

        [[nodiscard]]
        const Mat4x4& viewMatrix() const;

        [[nodiscard]]
        const Mat4x4& worldMatrix() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
