#pragma once
#include "Duration.h"
#include "System.h"

namespace TY
{
    //-----------------------------------------------
    //
    //	This section is modified part of the Siv3D Engine.
    //
    //	Copyright (c) 2008-2025 Ryo Suzuki
    //	Copyright (c) 2016-2025 OpenSiv3D Project
    //
    //	Licensed under the MIT License.
    //
    //-----------------------------------------------

    namespace Periodic
    {
        /// @brief サインカーブに従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sine0_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief サインカーブに従って、周期的に [0.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sine0_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Square0_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Square0_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param dutyCycle デューティー比、周期内で波形の大きさが 1.0 である時間の割合
        /// @param t 経過時間（秒）
        /// @return 0.0 または 1.0
        /// @remark デューティー比が 0.5 のとき `Square0_1()` と一致します。
        [[nodiscard]]
        float Pulse0_1(float periodSec, float dutyCycle, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param dutyCycle デューティー比、周期内で波形の大きさが 1.0 である時間の割合
        /// @param t 経過時間（秒）
        /// @return 0.0 または 1.0
        /// @remark デューティー比が 0.5 のとき `Square0_1()` と一致します。
        [[nodiscard]]
        float Pulse0_1(const Duration& period, float dutyCycle, float t = System::Time()) noexcept;

        /// @brief 三角波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Triangle0_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief 三角波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Triangle0_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief のこぎり波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sawtooth0_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief のこぎり波に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sawtooth0_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief ジャンプする運動に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Jump0_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief ジャンプする運動に従って、周期的に [0.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [0.0, 1.0] の範囲の値
        [[nodiscard]]
        float Jump0_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief サインカーブに従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sine1_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief サインカーブに従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sine1_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Square1_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Square1_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param dutyCycle デューティー比、周期内で波形の大きさが 1.0 である時間の割合
        /// @param t 経過時間（秒）
        /// @return -1.0 または 1.0
        /// @remark デューティー比が 0.5 のとき `Square1_1()` と一致します。
        [[nodiscard]]
        float Pulse1_1(float periodSec, float dutyCycle, float t = System::Time()) noexcept;

        /// @brief 矩形波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param dutyCycle デューティー比、周期内で波形の大きさが 1.0 である時間の割合
        /// @param t 経過時間（秒）
        /// @return -1.0 または 1.0
        /// @remark デューティー比が 0.5 のとき `Square1_1()` と一致します。
        [[nodiscard]]
        float Pulse1_1(const Duration& period, float dutyCycle, float t = System::Time()) noexcept;

        /// @brief 三角波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Triangle1_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief 三角波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Triangle1_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief のこぎり波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sawtooth1_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief のこぎり波に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Sawtooth1_1(const Duration& period, float t = System::Time()) noexcept;

        /// @brief ジャンプする運動に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param periodSec 周期（秒）
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Jump1_1(float periodSec, float t = System::Time()) noexcept;

        /// @brief ジャンプする運動に従って、周期的に [-1.0, 1.0] の値を返します。
        /// @param period 周期
        /// @param t 経過時間（秒）
        /// @return [-1.0, 1.0] の範囲の値
        [[nodiscard]]
        float Jump1_1(const Duration& period, float t = System::Time()) noexcept;
    }
}
