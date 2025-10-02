#include "pch.h"
#include "ActorContainer.h"

namespace TY
{
    void ActorContainer::updateEach()
    {
        // 生存していないオブジェクトを削除
        for (int i = m_actorList.size() - 1; i >= 0; --i)
        {
            if (not m_actorList[i]->isAlive())
            {
                m_actorList.erase(m_actorList.begin() + i);
            }
        }

        double previousPriority{};
        bool needSort = false;

        // 更新処理
        m_iterating = true;
        for (int i = 0; i < m_actorList.size(); ++i)
        {
            if (not m_actorList[i]->isAlive()) continue;

            m_actorList[i]->update();

            if (m_shouldKill) break;

            if (needSort) continue;

            // 優先度が入れ替わっている部分があれば後からソートする
            if (i > 0 && previousPriority < m_actorList[i]->orderPriority())
            {
                needSort = true;
                continue;
            }

            previousPriority = m_actorList[i]->orderPriority();
        }

        m_iterating = false;

        if (m_shouldKill)
        {
            killEach();
            return;
        }

        if (needSort)
        {
            // ソート
            std::ranges::stable_sort(
                m_actorList,
                [](const std::shared_ptr<ActorBase>& left, const std::shared_ptr<ActorBase>& right)
                {
                    return left->orderPriority() > right->orderPriority();
                });
        }
    }

    void ActorContainer::drawEach() const
    {
        // 描画処理
        for (int i = 0; i < m_actorList.size(); ++i)
        {
            if (not m_actorList[i]->isAlive()) continue;

            m_actorList[i]->draw();
        }
    }

    void ActorContainer::killEach()
    {
        if (m_iterating)
        {
            m_shouldKill = true;
            return;
        }

        // リストをクリアする前に Kill を呼ぶ
        for (auto& actor : m_actorList)
        {
            actor->kill();
        }

        m_actorList.clear();

        m_shouldKill = false;
    }

    void ActorContainer::birth(const std::shared_ptr<ActorBase>& actor)
    {
        assert(actor != nullptr);
        m_actorList.push_back(actor);
    }
}
