#include "pch.h"
#include "MetaAi.h"

#include "Asset.generated.h"
#include "CharacterAi.h"
#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
    // 逆誤差関数 erf^-1 (近似式: Winitzki approximation)
    float erfinv(float x)
    {
        // Winitzki approximation with good accuracy
        const float a = 0.147f;
        const float ln = std::log(1.0f - x * x);
        const float term1 = 2.0f / (Math::Pi * a) + ln / 2.0f;
        const float inside = term1 * term1 - ln / a;

        float result = std::sqrt(inside) - term1;
        if (x < 0) result = -result;
        return result;
    }

    // 正規分布の密度に従って値を生成
    Array<float> makeGaussianRange(
        int num_elements,
        float min_element,
        float max_element)
    {
        Array<float> v(num_elements);

        if (num_elements == 1)
        {
            v[0] = 0.5f * (min_element + max_element);
            return v;
        }

        for (int i = 0; i < num_elements; ++i)
        {
            float p = (i + 0.5f) / num_elements; // (0, 1)
            float z = erfinv(2.0f * p - 1.0f); // 標準正規の逆CDF: (-inf, +inf)

            // z を (-3, +3) の範囲に正規化
            float t = (z + 3.0f) / 6.0f; // (0, 1) に変換
            t = std::clamp(t, 0.0f, 1.0f);

            // (min, max) の範囲にマッピング
            v[i] = min_element + t * (max_element - min_element);
        }

        return v;
    }

    // -----------------------------------------------

    float evaluateRubberBandingBoost(const MachinePhysicsUnit& machine, float rubberBandingBias)
    {
        // TODO: rubberBandingBias

        if (machine.state.isHovering())
        {
            return 1.0f;
        }

        const auto& stageManager = GetRaceContext().stageManager();

        const auto& playerMachine = GetRaceContext().machineManager().machineList()[PlayerMachineId];
        const float playerDistance = stageManager.getDistanceFromStart(playerMachine.state.m_lapProgress);

        const float thisDistance = stageManager.getDistanceFromStart(machine.state.m_lapProgress);

        const float distanceFromPlayer = thisDistance - (playerDistance + rubberBandingBias);

        float r = -distanceFromPlayer / 200.0f;
        r = Math::Clamp(r, -1.0f, 1.0f); // [-1.0f, 1.0f]
        // r = (r + 1.0f) * 0.5f; // [-1.0f, 1.0f] --> [0.0f, 1.0f]

        float boostFactor;
        if (r < 0.0f)
        {
            boostFactor = 1.0f + r * 0.5f;
        }
        else // if (r >= 0.0f)
        {
            boostFactor = 1.0f + r * 4.0f;
        }

        return boostFactor;
    }

    struct CachePerCharacterAi
    {
        float rubberBandingBias{};
    };
}

struct MetaAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    Array<CachePerCharacterAi> m_characterAiCaches{};

    void Init()
    {
        const int characterAiCount = GetRaceContext().characterAiList().size();
        m_characterAiCaches.resize(characterAiCount);

        rebuildRubberBandingBias(characterAiCount);
    }

private:
    void update() override
    {
        auto& characterAiList = GetRaceContext().characterAiList();
        assert(characterAiList.size() == m_characterAiCaches.size());

        for (int i = 0; i < characterAiList.size(); i++)
        {
            const auto& aiMachine = GetRaceContext().machineManager().machineList()[characterAiList[i].machineId()];

            CharacterAiInputCommand command{};
            command.targeCheatBoost = evaluateRubberBandingBoost(aiMachine, m_characterAiCaches[i].rubberBandingBias);

            characterAiList[i].setInputCommand(command);
        }

        debugUI();
    }

    void debugUI()
    {
        ImGui::Begin("Meta AI");

        if (ImGui::Button("Rebuild Rubber Banding Offset"))
        {
            rebuildRubberBandingBias(GetRaceContext().characterAiList().size());
        }

        ImGui::End();
    }

    void rebuildRubberBandingBias(const int characterAiCount)
    {
        // 線形補間版
        for (int i = 0; i < characterAiCount; ++i)
        {
            constexpr float min = -100.0f;
            constexpr float max = 100.0f;

            const float f = static_cast<float>(i) / static_cast<float>(characterAiCount - 1);
            m_characterAiCaches[i].rubberBandingBias = Math::Lerp(min, max, f);
        }

        // ガウス分布版
#if 0
        const auto gaussianRange = makeGaussianRange(
            characterAiCount,
            -100.0f,
            100.0f);
        for (int i = 0; i < characterAiCount; ++i)
        {
            m_characterAiCaches[i].rubberBandingBias = gaussianRange[i];

#if defined(_DEBUG) && 0
        if (i == 0) std::cout << "----------------------------------------------- rubberBandingOffset\n";

        std::cout << m_characterAiCaches[i].rubberBandingBias << std::endl;
#endif
        }
#endif
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"MetaAi";
    }
};

namespace Race
{
    MetaAi::MetaAi() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MetaAi::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> MetaAi::asGameObject() const
    {
        return p_impl;
    }
}
