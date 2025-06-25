#pragma once

namespace Util
{
	/// @brief 毎フレーム更新されることを期待するオブジェクトの基底クラス
	/// @remark これを継承した各オブジェクトのデータは、shared_ptr で管理されることを想定している
	class ActorBase
	{
	public:
		virtual ~ActorBase() = default;

		/// @brief 毎フレーム更新する処理
		virtual void Update() { return; };

		/// @brief 描画処理
		virtual void Draw() const { return; }

		/// @brief オブジェクトを破壊する
		virtual void Kill();

		/// @brief オブジェクトが生存しているか
		/// @remark false になると、親から破棄される。ただし、false を返している間もアクセス可能の状態にしておくこと。
		bool IsAlive() const { return m_alive; }

		/// @brief オブジェクト優先度
		/// @remark 大きいほど値であるほど Update() と Draw() において先に処理される
		virtual double OrderPriority() const { return 0; }

	protected:
		/// @brief Kill() 呼び出し時に内部から呼ばれる
		/// @remark 継承先では、子オブジェクトの破棄を必ず行うこと
		virtual void Killed() = 0;

	private:
		bool m_alive{true};
	};

	using ActorRef = std::shared_ptr<ActorBase>;

	using ActorWeakRef = std::weak_ptr<ActorBase>;

	class EmptyActor : public ActorBase
	{
		void Killed() override { return; }
	};
}
