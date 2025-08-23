#pragma once
#include "TY/Vector4D.h"
#include "DescriptorHeap.h"

using namespace TY;
using namespace TY::detail;

namespace TY::ShapeDrawer_detail
{
    struct ShapeDraw_b0
    {
        Float4 g_transform[2];
        Float4 g_colorMul{1.0f};
        Float4 g_colorAdd{0.0f};
    };
}
