#pragma once
#include "TY/BitmapFont.h"
#include "TY/SdfFont.h"
#include "TY/Shape2D.h"

namespace Asset0
{
    BitmapFont RocknRoll_24_Bitmap();
}

namespace Shape2D_Text
{
    Shape2D::Text RocknRoll_24_Bitmap(const std::u32string& text);
}
