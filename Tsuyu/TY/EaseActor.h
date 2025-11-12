#pragma once
#include "ActorContainer.h"
#include "ActorHandle.h"
#include "Duration.h"
#include "Vector2D.h"

namespace TY
{
    namespace EaseOption
    {
        enum : uint64_t
        {
            /// @brief イージング終了時に、値を終了値に固定する
            EndStop = 1 << 0,

            /// @brief インゲーム時間を使用しない
            NotInGame = 1 << 1,

            /// @brief 角度の補完 (2D)
            LerpAngle = 1 << 2,
        };

        constexpr uint64_t None = 0;

        constexpr uint64_t Default = EndStop;
    };

    template <typename T>
    class EaseActor : public ActorHandle
    {
    public:
        EaseActor();

        EaseActor(T& valuePtr, T endValue, double duration);

        using function_t = double(*)(double);
        EaseActor& setFunction(function_t easeFunction);

        EaseActor& setOption(uint64_t option);

        EaseActor& onUpdate(const std::function<void()>& onUpdate);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template class EaseActor<float>;

    template class EaseActor<double>;

    template class EaseActor<Float2>;

    // -----------------------------------------------

    template <double easing(double) = nullptr, uint64_t option = EaseOption::Default, typename T>
    EaseActor<T> StartEasing(ActorContainer& parent, T& valuePtr, T endValue, double duration)
    {
        if constexpr (easing == nullptr)
        {
            return parent.birth(EaseActor<T>(valuePtr, endValue, duration));
        }

        return parent.birth(EaseActor<T>(valuePtr, endValue, duration))
                     .setFunction(easing)
                     .setOption(option);
    }

    template <double easing(double) = nullptr, uint64_t option = EaseOption::Default, typename T>
    EaseActor<T> StartEasing(ActorContainer& parent, T& valuePtr, T endValue, Duration duration)
    {
        return StartEasing<easing, option>(parent, valuePtr, endValue, static_cast<double>(duration.count()));
    }
}
