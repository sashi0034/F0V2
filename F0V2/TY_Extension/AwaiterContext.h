#pragma once

#include "CoroutineActor.h"
#include "GameTime.h"
#include "TY/Array.h"
#include "TY/Uncopyable.h"

namespace TY
{
    /// @brief コルーチン停止に用いるためのコンテキスト
    class AwaiterContext : Uncopyable
    {
    public:
        explicit AwaiterContext(std::reference_wrapper<CoroutineActor::yield_type> yield);

        /// @brief 指定フレーム待機する
        void WaitForFrames(int frames = 1);

        /// @brief 指定時間待機する
        void WaitForTime(double seconds, std::function<double()> deltaTime = InGameDeltaTime);

        // void WaitForTime(Duration seconds, const std::function<double()>& deltaTime = InGameDeltaTime);

        void WaitForever();

        /// @brief true を返すまで待機する
        void WaitForTrue(const std::function<bool()>& poller);

        /// @brief いずれかの関数が true を返すまで待機する
        /// @return true を返した関数のインデックス
        [[nodiscard]]
        int WaitAnyTrue(const Array<std::function<bool()>>& pollers);

        /// @brief Actor が破棄されるまで待機する
        void WaitForExpired(const ActorHandle& actor);

        void WaitForExpired(std::shared_ptr<ActorBase> actor);

        void WaitForExpired(std::weak_ptr<ActorBase> actor);

    protected:
        void yield();

        std::reference_wrapper<CoroutineActor::yield_type> m_yield;
        std::function<bool()> m_resumePoller{};
    };

    /// @brief コルーチンのコンテキスト発行側で用いるためのオブジェクト
    class AwaiterController final : public AwaiterContext
    {
    public:
        using AwaiterContext::AwaiterContext;

        /// @brief コルーチン再開可能であるかを調べる
        [[nodiscard]] bool ValidateResume();
    };
}
