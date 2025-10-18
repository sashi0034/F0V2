#pragma once
#include "ActorBase.h"
#include "Array.h"

using namespace TY;

namespace TY
{
    /// @brief Actor のリストを管理するコンテナ
    class ActorContainer
    {
    public:
        void updateEach();

        /// @brief アクターを破棄する
        /// @remarks UpdateEach の最中に呼び出された場合は、遅延実行される
        void killEach();

        template <typename T>
        T birth(const T& actor)
        {
            if constexpr (requires { actor.asActor(); })
                birth(static_cast<std::shared_ptr<ActorBase>>(actor.asActor()));
            else
                birth(static_cast<std::shared_ptr<ActorBase>>(actor));

            return actor;
        }

        void birth(const std::shared_ptr<ActorBase>& actor);

        const Array<std::shared_ptr<ActorBase>>& actorList() const { return m_actorList; }

    private:
        Array<std::shared_ptr<ActorBase>> m_actorList{};

        bool m_iterating{};

        bool m_shouldKill{};
    };
}
