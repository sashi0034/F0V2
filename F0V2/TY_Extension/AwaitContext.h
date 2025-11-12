#pragma once

#include "CoroutineActor.h"
#include "TY/ActorLifetimeScope.h"
#include "TY/GameTime.h"
#include "TY/Array.h"
#include "TY/Duration.h"
#include "TY/Uncopyable.h"

namespace TY
{
    /// @brief コルーチン停止に用いるためのコンテキスト
    class AwaitContext : Uncopyable
    {
    public:
        explicit AwaitContext(std::reference_wrapper<CoroutineActor::yield_type> yield);

        /// @brief 指定フレーム待機する
        void waitForFrames(int frames = 1);

        /// @brief 指定時間待機する
        void waitForTime(double seconds, std::function<double()> deltaTime = InGameDeltaTime);

        void waitForTime(Duration seconds, const std::function<double()>& deltaTime = InGameDeltaTime);

        void waitForever();

        /// @brief true を返すまで待機する
        void waitForTrue(const std::function<bool()>& poller);

        /// @brief いずれかの関数が true を返すまで待機する
        /// @return true を返した関数のインデックス
        [[nodiscard]]
        int waitAnyTrue(const Array<std::function<bool()>>& pollers);

        /// @brief Actor が破棄されるまで待機する
        void waitForExpired(const ActorHandle& actor);

        void waitForExpired(std::shared_ptr<ActorBase> actor);

        void waitForExpired(std::weak_ptr<ActorBase> actor);

        // TODO: Remove
        ActorLifetimeScope& lifetime();

        const ActorLifetimeScope& lifetime() const;

    protected:
        void yield();

        std::reference_wrapper<CoroutineActor::yield_type> m_yield;
        std::function<bool()> m_resumePoller{};
        ActorLifetimeScope m_lifetime{};
    };

    /// @brief コルーチンのコンテキスト発行側で用いるためのオブジェクト
    class AwaitController final : public AwaitContext
    {
    public:
        using AwaitContext::AwaitContext;

        /// @brief コルーチン再開可能であるかを調べる
        [[nodiscard]] bool validateResume();
    };
}
