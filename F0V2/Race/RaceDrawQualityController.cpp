#include "pch.h"
#include "RaceDrawQualityController.h"

#include "TY/GpuMetrics.h"
#include "TY/Screen.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

constexpr float FHD_1_0 = 1.f;
constexpr float FHD_0_8 = 0.8f;
constexpr float FHD_0_6 = 0.6f;

struct RaceDrawQualityController::Impl
{
    int m_frameCount{};

    target_type m_qualityTarget{FHD_0_8};

    void Update()
    {
        m_frameCount++;

        if ((m_frameCount % Screen::FrameBufferCount()) == 0)
        {
            m_qualityTarget = evaluateNewQualityTarget();
        }

        ImmediatePrint_BottomRight("{}p", static_cast<int>(1080 * m_qualityTarget.renderScale));
    }

private:
    target_type evaluateNewQualityTarget() const
    {
        const float recentExecutionMilliseconds = GpuMetrics::LastExecutionMilliseconds();

        if (m_qualityTarget.renderScale == FHD_1_0)
        {
            if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_8};
            }

            return m_qualityTarget;
        }
        else if (m_qualityTarget.renderScale == FHD_0_8)
        {
            if (recentExecutionMilliseconds < 1000.0f / 90)
            {
                return {FHD_1_0};
            }
            else if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_6};
            }

            return m_qualityTarget;
        }
        else if (m_qualityTarget.renderScale == FHD_0_6)
        {
            if (recentExecutionMilliseconds < 1000.0f / 90)
            {
                return {FHD_0_8};
            }

            return m_qualityTarget;
        }
        else
        {
            assert(false);
            return {FHD_0_8};
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
