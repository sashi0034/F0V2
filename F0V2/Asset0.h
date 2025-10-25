#pragma once
#include "TY/BitmapFont.h"
#include "TY/SdfFont.h"
#include "TY/Immediate2D.h"

namespace Asset0
{
    BitmapFont RocknRoll_24_Bitmap();

    SdfFont RocknRoll_Sdf();

    BitmapFont ZXProto_24_Bitmap();

    SdfFont ZXProto_Sdf();

    BitmapFont MPlus1_16_Bitmap();

    BitmapFont MPlus1_24_Bitmap();

    SdfFont MPlus1_Sdf();
}

namespace Immediate2D_Text
{
    Immediate2D::Text RocknRoll_24_Bitmap(const std::u32string& text);

    Immediate2D::Text RocknRoll_Sdf(const std::u32string& text);

    Immediate2D::Text ZXProto_24_Bitmap(const std::u32string& text);

    Immediate2D::Text ZXProto_Sdf(const std::u32string& text);

    Immediate2D::Text MPlus1_16_Bitmap(const std::u32string& text);

    Immediate2D::Text MPlus1_24_Bitmap(const std::u32string& text);

    Immediate2D::Text MPlus1_Sdf(const std::u32string& text);
}
