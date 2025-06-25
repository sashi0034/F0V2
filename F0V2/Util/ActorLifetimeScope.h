#pragma once
#include "TY/Array.h"
#include "TY/Uncopyable.h"

namespace Util
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
        T Append(const T& actor)
        {
            if constexpr (requires { actor.AsActor(); })
                Append(static_cast<std::shared_ptr<ActorBase>>(actor.AsActor()));
            else
                Append(static_cast<std::shared_ptr<ActorBase>>(actor));

            return actor;
        }

        void Append(const std::shared_ptr<ActorBase>& actor);

        template <typename T>
        friend T operator >>(const T& left, ActorLifetimeScope& right) noexcept
        {
            right.Append(left);
            return left;
        }

        /// @brief 追加されたアクターをすべて破棄する
        void Clear();

        /// @brief 既に死んだアクターをリストから除く
        void CleanUp();

        /// @brief いずれかのアクターが生存しているか
        [[nodiscard]]
        bool AnyActive() const;

        /// @brief リストが空であるか
        [[nodiscard]]
        bool IsEmpty() const;

    private:
        TY::Array<std::shared_ptr<ActorBase>> m_actorList{};
    };
}
