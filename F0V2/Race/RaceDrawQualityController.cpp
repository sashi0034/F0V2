#include "pch.h"
#include "RaceDrawQualityController.h"

#include "GameGlobalUI.h"
#include "TY/GpuMetrics.h"
#include "TY/Screen.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

constexpr float FHD_1_0 = 1.f;
constexpr float FHD_0_8 = 0.8f;
constexpr float FHD_0_4 = 0.4f;

constexpr RaceDrawQualityController::data_type defaultQualityData{FHD_0_8, true};

struct RaceDrawQualityController::Impl
{
    int m_frameCount{};

    data_type m_qualityData{defaultQualityData};

    void Update()
    {
        m_frameCount++;

        if ((m_frameCount % Screen::FrameBufferCount()) == 0)
        {
            m_qualityData = evaluateNewQualityData();
        }

        if (IsGameStatsVisible())
        {
            ImmediatePrint_BottomRight(
                "{}{}p",
                (m_qualityData.fsrEnabled ? "FSR | " : ""),
                static_cast<int>(1080 * m_qualityData.renderScale));
        }
    }

private:
    data_type evaluateNewQualityData() const
    {
        const float recentExecutionMilliseconds = GpuMetrics::LastExecutionMilliseconds();

        if (m_qualityData.renderScale == FHD_1_0)
        {
            if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_8, true};
            }

            return m_qualityData;
        }
        else if (m_qualityData.renderScale == FHD_0_8)
        {
            if (recentExecutionMilliseconds < 1000.0f / 90)
            {
                return {FHD_1_0, false};
            }
            else if (recentExecutionMilliseconds > 1000.0f / 60)
            {
                return {FHD_0_4, true};
            }

            return m_qualityData;
        }
        else if (m_qualityData.renderScale == FHD_0_4)
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
            return defaultQualityData;
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

    RaceDrawQualityController::data_type RaceDrawQualityController::getQualityData() const
    {
        return p_impl->m_qualityData;
    }
}
