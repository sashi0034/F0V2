#pragma once
#include "TY/Alignment.h"

namespace Util
{
    void InitImmediatePrintAddon();

    void ImmediatePrint(const std::string& message, Alignment9 align = Alignment9::TopLeft);

    void ImmediatePrint(const std::u32string& message, Alignment9 align = Alignment9::TopLeft);
}
