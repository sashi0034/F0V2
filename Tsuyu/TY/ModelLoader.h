#pragma once
#include "ModelData.h"

namespace TY
{
    namespace ModelLoader
    {
        ModelData Load(const std::string& filename);

        /// @brief 拡張子 .obj の Wavefront OBJ の読み込み
        ModelData FromWavefront(const std::string& filename);
    }
}
