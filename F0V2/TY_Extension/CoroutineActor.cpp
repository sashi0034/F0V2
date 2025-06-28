#include "pch.h"
#include "CoroutineActor.h"

#include "AwaiterContext.h"
#include "TY/Array.h"

using namespace TY;

namespace
{
    constexpr int maxCoroutines = 256;

    struct StackBuffer
    {
        StackBuffer()
        {
            bytes.resize(boost::context::stack_traits::default_size());
        }

        Array<uint8_t> bytes;
    };

    /// @brief StackAllocator for boost::coroutines2::detail::pull_coroutine
    /// @remark boost::context::basic_fixedsize_stack を参考に実装
    class StackAllocator
    {
    public:
        StackAllocator(StackBuffer* stackPtr, std::function<void()> onDeallocate) :
            m_stackPtr(stackPtr),
            m_onDeallocate(onDeallocate)
        {
        }

        boost::context::stack_context allocate()
        {
            boost::context::stack_context sctx;
            sctx.size = m_stackPtr->bytes.size();
            sctx.sp = m_stackPtr->bytes.data() + sctx.size;
            return sctx;
        }

        void deallocate(boost::context::stack_context& sctx)
        {
            assert(sctx.sp);
            m_onDeallocate();
        }

    private:
        StackBuffer* m_stackPtr;
        std::function<void()> m_onDeallocate;
    };

    Array<int> Range(int start, int end)
    {
        Array<int> range;
        for (int i = start; i <= end; ++i)
        {
            range.push_back(i);
        }

        return range;
    }

    class StackDistributor
    {
    public:
        StackAllocator get()
        {
            if (m_unusedStackIndices.empty())
            {
                throw std::runtime_error("No more stack available for coroutine");
            }

            const int index = m_unusedStackIndices.back();
            m_unusedStackIndices.pop_back();

            return StackAllocator(&m_stackBuffers[index], [this, index]
            {
                m_unusedStackIndices.push_back(index);
            });
        }

    private:
        std::array<StackBuffer, maxCoroutines> m_stackBuffers{};
        Array<int> m_unusedStackIndices{Range(0, maxCoroutines - 1)};
    };

    StackDistributor s_stackDistributor{};
}

struct CoroutineActor::Impl : ActorBase
{
    std::unique_ptr<caller_type> m_task{};

    std::shared_ptr<AwaiterController> m_awaiter{};

    bool m_initialized{};

    void update() override
    {
        if (not isAlive()) return;

        if (m_task == nullptr) return;

        // コルーチンが再開可能であるか問い合わせる
        if (not m_awaiter->ValidateResume()) return;

        // コルーチン再開
        if ((*m_task)())
        {
            // fallthrough
        }
        else
        {
            // 終了
            kill();
        }
    }

    void killed() override
    {
        m_task.reset();
        m_awaiter.reset();
    }
};

namespace TY
{
    CoroutineActor::CoroutineActor() :
        p_impl(std::make_shared<Impl>())
    {
        p_impl->kill();
    }

    CoroutineActor::CoroutineActor(const task_function& task) :
        p_impl(std::make_shared<Impl>())
    {
        auto stack = s_stackDistributor.get();

        // 初回 Update のときにコルーチンを作成する
        p_impl->m_task = std::make_unique<caller_type>(
            stack, // この行をコメントアウトするとデフォルトの boost::context::default_stack が使われます
            [impl = p_impl.get(), task](yield_type& yield)
            {
                impl->m_awaiter = std::make_shared<AwaiterController>(yield);

                task(*impl->m_awaiter);

                // 生成直後にタスクが終了した場合、1 フレーム待機するようにする
                if (not impl->m_initialized) impl->m_awaiter->WaitForFrames(1);
            });

        p_impl->m_initialized = true;
    }

    std::shared_ptr<ActorBase> CoroutineActor::asActor() const
    {
        return p_impl;
    }

    void CoroutineActor::update()
    {
        if (p_impl && p_impl->isAlive())
        {
            p_impl->update();
        }
    }
}
