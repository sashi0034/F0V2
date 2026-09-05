#pragma once
#include "HybridArray.h"

namespace TY
{
    template <typename T>
    using DescriptorList = HybridArray<T, 4>;

    template <typename T>
    using MaterialList = HybridArray<T, 1>;
}
