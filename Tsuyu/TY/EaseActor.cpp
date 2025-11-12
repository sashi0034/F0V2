#include "pch.h"
#include "EaseActor.h"

#include "Easing.h"
#include "GameTime.h"

using namespace TY;

namespace
{
    template <typename T>
    std::reference_wrapper<T> getDefaultReference()
    {
        static T value{};
        return std::ref(value);
    }

    struct OptionFlags
    {
        bool endStop;
        bool notInGame;
        bool lerpAngle;

        void setOption(uint64_t option)
        {
            endStop = option & EaseOption::EndStop;
            notInGame = option & EaseOption::NotInGame;
            lerpAngle = option & EaseOption::LerpAngle;
        }
    };
}

template <typename T>
struct EaseActor<T>::Impl : ActorBase
{
    std::reference_wrapper<T> m_valuePtr{getDefaultReference<T>()};
    T m_startValue{};
    T m_endValue{};
    double m_time{};
    double m_duration{};

    function_t m_easeFunction{EaseInLinear};

    OptionFlags m_option{};

    std::function<void()> m_onUpdate{};

    Impl()
    {
        m_option.setOption(EaseOption::Default);
    }

    void update() override
    {
        if (not updateInternal())
        {
            kill();
        }
    }

    void killed() override
    {
        // Do nothing
    }

private:
    bool updateInternal()
    {
        bool alive = true;
        if (m_option.notInGame)
        {
            m_time += StandardDeltaTime();
        }
        else
        {
            m_time += InGameDeltaTime();
        }

        if (m_time >= m_duration)
        {
            // イージング終了
            if (m_option.endStop)
            {
                m_time = m_duration;
            }

            alive = false;
        }

        const double e = m_easeFunction(m_time / m_duration);

        // 値の更新処理
        if (m_option.lerpAngle)
        {
            if constexpr (std::is_floating_point<T>::value)
            {
                m_valuePtr.get() = Math::LerpAngle(m_startValue, m_endValue, e);
            }
            else
            {
                assert(false, "LerpAngle option is only available for floating point types.");
            }
        }
        else
        {
            m_valuePtr.get() = m_startValue * (1 - e) + m_endValue * e;
        }

        if (m_onUpdate) m_onUpdate();

        return alive;
    }
};

namespace TY
{
    template <typename T>
    EaseActor<T>::EaseActor()
        : p_impl(std::make_shared<Impl>())
    {
        p_impl->kill();
    }

    template <typename T>
    EaseActor<T>::EaseActor(T& valuePtr, T endValue, double duration) :
        p_impl(std::make_shared<Impl>())
    {
        p_impl->m_valuePtr = valuePtr;
        p_impl->m_startValue = valuePtr;
        p_impl->m_endValue = endValue;
        p_impl->m_duration = duration;
    }

    template <typename T>
    EaseActor<T>& EaseActor<T>::setFunction(function_t easeFunction)
    {
        assert(easeFunction);
        p_impl->m_easeFunction = easeFunction;
        return *this;
    }

    template <typename T>
    EaseActor<T>& EaseActor<T>::setOption(uint64_t option)
    {
        p_impl->m_option.setOption(option);
        return *this;
    }

    template <typename T>
    EaseActor<T>& EaseActor<T>::onUpdate(const std::function<void()>& onUpdate)
    {
        p_impl->m_onUpdate = onUpdate;
        return *this;
    }

    template <typename T>
    std::shared_ptr<ActorBase> EaseActor<T>::asActor() const
    {
        return p_impl;
    }
}
