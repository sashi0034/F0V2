#pragma once
#include "TY/ActorBase.h"
#include "TY/Duration.h"
#include "TY/GameTime.h"

namespace TY
{
    /// @brief 一定時間が経過すると true を返す関数を作成する。毎フレーム呼び出すことで更新する。
    std::function<bool()> MakeTimeoutTask(float seconds, float (*deltaTime)() = InGameDeltaTime);

    std::function<bool()> MakeTimeoutTask(Duration seconds, float (*deltaTime)() = InGameDeltaTime);

    /// @brief Actor がキルされると true を返す関数を作成する
    std::function<bool()> MakeExpireObserver(ActorWeakRef actor);
}
