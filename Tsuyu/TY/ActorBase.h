#pragma once

namespace TY
{
    /// @brief 毎フレーム更新されることを期待するオブジェクトの基底クラス
    /// @remark これを継承した各オブジェクトのデータは、shared_ptr で管理されることを想定している
    class ActorBase
    {
    public:
        virtual ~ActorBase() = default;

        /// @brief 毎フレーム更新する処理
        virtual void update() { return; };

        // TODO: Remove it?
        /// @brief 描画処理
        virtual void draw() const { return; }

        /// @brief オブジェクトを破壊する
        virtual void kill();

        /// @brief オブジェクトが生存しているか
        /// @remark false になると、親から破棄される。ただし、false を返している間もアクセス可能の状態にしておくこと。
        bool isAlive() const { return m_alive; }

        /// @brief オブジェクト優先度
        /// @remark 大きいほど値であるほど update() において先に処理される
        virtual float orderPriority() const { return 0; }

    protected:
        /// @brief Kill() 呼び出し時に内部から呼ばれる
        /// @remark 継承先では、子オブジェクトの破棄を必ず行うこと
        virtual void killed() = 0;

    private:
        bool m_alive{true};
    };

    using ActorRef = std::shared_ptr<ActorBase>;

    using ActorWeakRef = std::weak_ptr<ActorBase>;

    class EmptyActor : public ActorBase
    {
        void killed() override { return; }
    };
}
