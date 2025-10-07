#pragma once
#include "TY/Vector4D.h"
#include "DescriptorHeap.h"

using namespace TY;
using namespace TY::detail;

namespace TY::ImmediateDrawer_detail
{
    struct ImmediateDrawer_b1
    {
        Float4 g_transform[2];
        Float4 g_colorMul{1.0f};
        Float4 g_colorAdd{0.0f};
    };
}
