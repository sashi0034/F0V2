#pragma once
#include "ActorBase.h"
#include "TY/Array.h"

using namespace TY;

namespace Util
{
    /// @brief Actor のリストを管理するコンテナ
    class ActorContainer
    {
    public:
        void UpdateEach();

        void DrawEach() const;

        /// @brief アクターを破棄する
        /// @remarks UpdateEach の最中に呼び出された場合は、遅延実行される
        void KillEach();

        template <typename T>
        T Birth(const T& actor)
        {
            if constexpr (requires { actor.AsActor(); })
                Birth(static_cast<std::shared_ptr<ActorBase>>(actor.AsActor()));
            else
                Birth(static_cast<std::shared_ptr<ActorBase>>(actor));

            return actor;
        }

        void Birth(const std::shared_ptr<ActorBase>& actor);

        const Array<std::shared_ptr<ActorBase>>& ActorList() const { return m_actorList; }

    private:
        Array<std::shared_ptr<ActorBase>> m_actorList{};

        bool m_iterating{};

        bool m_shouldKill{};
    };
}
