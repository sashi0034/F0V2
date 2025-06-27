#pragma once
#include "TY/ActorBase.h"
#include "TY/ActorContainer.h"
#include "TY/ActorHandle.h"

namespace TY
{
    class AwaiterContext;

    /// @brief boost::coroutine2 Wrapper
    class CoroutineActor : public ActorHandle
    {
    public:
        using yield_type = boost::coroutines2::coroutine<void>::push_type;
        using caller_type = boost::coroutines2::coroutine<void>::pull_type;
        using task_function = std::function<void(AwaiterContext&)>;

        explicit CoroutineActor();

        explicit CoroutineActor(const task_function& task);

        std::shared_ptr<ActorBase> asActor() const override;

        void update();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    inline CoroutineActor StartCoroutine(ActorContainer& container, const CoroutineActor::task_function& coroutine)
    {
        return container.birth(CoroutineActor{coroutine});
    }
}
