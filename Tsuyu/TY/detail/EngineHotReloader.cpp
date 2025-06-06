#include "pch.h"
#include "EngineHotReloader.h"

#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct TrackingElement
    {
        std::weak_ptr<IEngineHotReloadable> target;
        Array<std::shared_ptr<ITimestamp>> dependencies;
    };

    bool checkHotReloadElement(IEngineHotReloadable& target, const Array<std::shared_ptr<ITimestamp>>& dependencies)
    {
        const auto targetTimestamp = target.timestamp();

        for (const auto& dependency : dependencies)
        {
            const auto dependencyTimestamp = dependency->timestamp();
            if (dependencyTimestamp == InvalidTimestampValue)
            {
                break;
            }

            if (targetTimestamp < dependencyTimestamp)
            {
                // 依存先のアセットが最近更新されたので、ターゲットをホットリロードする
                target.HotReload();
                return true;
            }
        }

        return false;
    }

    struct Impl
    {
        Array<TrackingElement> m_elements{};

        void Update()
        {
            // 更新可能が発生しない状態になるするまでホットリロードを適応する
            // 正常に更新が走る場合、要素個数以上の回数の更新は発生しない
            int reloadedCount{};
            for (reloadedCount = 0; reloadedCount < m_elements.size(); ++reloadedCount)
            {
                const bool reloaded = checkHotReload();
                if (not reloaded) break;
            }

            if (reloadedCount == m_elements.size())
            {
                // いずれかの要素が正常に更新されない異常発生
                LogWarning.Writeln(L"the timestamps of some elements have not been updated in hot reload");
            }
            else if (reloadedCount > 0)
            {
                LogInfo.HR().Writeln(std::format(L"hot reloaded {} elements", reloadedCount));
            }
        }

        // 全要素を探索し、ホットリロード可能な要素があれば適応する
        bool checkHotReload()
        {
            bool hotReloaded{};
            for (int i = m_elements.size() - 1; i >= 0; --i)
            {
                auto& element = m_elements[i];
                if (auto target = element.target.lock())
                {
                    hotReloaded |= checkHotReloadElement(*target, element.dependencies);
                }
                else
                {
                    // アセットが破棄されたので削除する
                    m_elements.erase(m_elements.begin() + i);
                }
            }

            return hotReloaded;
        }
    };

    Impl s_hotReloader{};
}

namespace TY
{
    void EngineHotReloader_impl::Update() const
    {
        s_hotReloader.Update();
    }

    void EngineHotReloader_impl::Destroy() const
    {
        s_hotReloader.m_elements.clear();
    }

    void EngineHotReloader_impl::TrackAsset(
        std::weak_ptr<IEngineHotReloadable> target,
        Array<std::shared_ptr<ITimestamp>> dependencies
    ) const
    {
#ifdef  _DEBUG
        s_hotReloader.m_elements.emplace_back(target, std::move(dependencies));
#endif
    }
}
