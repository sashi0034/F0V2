#include "pch.h"
#include "RaceDrawQualityController.h"

#include "TY/GpuMetrics.h"
#include "TY/Screen.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

constexpr float FHD_1_0 = 1.f;
constexpr float FHD_0_8 = 0.8f;
constexpr float FHD_0_4 = 0.4f;

constexpr RaceDrawQualityController::target_type defaultQualityTarget{FHD_0_8, true};

struct RaceDrawQualityController::Impl
{
    int m_frameCount{};

    target_type m_qualityTarget{defaultQualityTarget};

    void Update()
    {
        m_frameCount++;

        if ((m_frameCount % Screen::FrameBufferCount()) == 0)
        {
            m_qualityTarget = evaluateNewQualityTarget();
        }

        ImmediatePrint_BottomRight(
            "{}{}p",
            (m_qualityTarget.fsrEnabled ? "FSR | " : ""),
            static_cast<int>(1080 * m_qualityTarget.renderScale));
    }

private:
    target_type evaluateNewQualityTarget() const
    {
        const float recentExecutionMilliseconds = GpuMetrics::LastExecutionMilliseconds();

        if (m_qualityTarget.renderScale == FHD_1_0)
        {
            if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_8, true};
            }

            return m_qualityTarget;
        }
        else if (m_qualityTarget.renderScale == FHD_0_8)
        {
            if (recentExecutionMilliseconds < 1000.0f / 90)
            {
                return {FHD_1_0, false};
            }
            else if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_4, true};
            }

            return m_qualityTarget;
        }
        else if (m_qualityTarget.renderScale == FHD_0_4)
        {
            if (recentExecutionMilliseconds < 1000.0f / 90)
            {
                return {FHD_0_8, true};
            }

            if (recentExecutionMilliseconds < 50.0f)
            {
                return {FHD_0_4, true};
            }
            else
            {
                return {FHD_0_4, false};
            }
        }
        else
        {
            assert(false);
            return defaultQualityTarget;
        }
    }
};

namespace Race
{
    RaceDrawQualityController::RaceDrawQualityController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceDrawQualityController::update()
    {
        p_impl->Update();
    }

    RaceDrawQualityController::target_type RaceDrawQualityController::getQualityTarget() const
    {
        return p_impl->m_qualityTarget;
    }
}
