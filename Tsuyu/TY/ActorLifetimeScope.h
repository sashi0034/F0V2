#pragma once
#include "TY/Array.h"
#include "TY/Uncopyable.h"

namespace TY
{
    class ActorBase;

    /// @brief スコープから離れるとともに保持された Actor をキルするコンテナ
    class ActorLifetimeScope : TY::Uncopyable
    {
    public:
        ActorLifetimeScope() = default;

        ~ActorLifetimeScope();

        ActorLifetimeScope(ActorLifetimeScope&&) noexcept;

        template <typename T>
        T append(const T& actor)
        {
            if constexpr (requires { actor.asActor(); })
                append(static_cast<std::shared_ptr<ActorBase>>(actor.asActor()));
            else
                append(static_cast<std::shared_ptr<ActorBase>>(actor));

            return actor;
        }

        void append(const std::shared_ptr<ActorBase>& actor);

        template <typename T>
        friend T operator >>(const T& left, ActorLifetimeScope& right) noexcept
        {
            right.append(left);
            return left;
        }

        /// @brief 追加されたアクターをすべて破棄する
        void clear();

        /// @brief 既に死んだアクターをリストから除く
        void cleanUp();

        /// @brief いずれかのアクターが生存しているか
        [[nodiscard]]
        bool anyActive() const;

        /// @brief リストが空であるか
        [[nodiscard]]
        bool isEmpty() const;

    private:
        TY::Array<std::shared_ptr<ActorBase>> m_actorList{};
    };
}
