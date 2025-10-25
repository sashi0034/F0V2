#include "pch.h"
#include "Asset0.h"

#include "TY/InlineComponent.h"

namespace
{
    struct Asset0State : IInlineComponent
    {
        BitmapFont m_RocknRoll_24_bitmap{};

        BitmapFont m_MPlus1_16_bitmap{};

        BitmapFont m_MPlus1_24_bitmap{};

        SdfFont m_MPlus1_Sdf_font{};

        SdfFont m_RocknRoll_Sdf_font{};

        Asset0State()
        {
            const std::string rocknRollPath = "asset/font/RocknRoll/RocknRollOne-Regular.ttf";

            m_RocknRoll_24_bitmap = BitmapFont(rocknRollPath, 24);

            m_RocknRoll_Sdf_font = SdfFont(rocknRollPath, 48);

            // -----------------------------------------------

            const std::string mplus1Path = "asset/font/M_PLUS_1/MPLUS1-Regular.ttf";

            m_MPlus1_16_bitmap = BitmapFont(mplus1Path, 16);

            m_MPlus1_24_bitmap = BitmapFont(mplus1Path, 24);

            m_MPlus1_Sdf_font = SdfFont(mplus1Path, 48);
        }
    };

    InlineComponent<Asset0State> s_asset0state{};
}

namespace Asset0
{
    BitmapFont RocknRoll_24_Bitmap()
    {
        return s_asset0state->m_RocknRoll_24_bitmap;
    }

    BitmapFont MPlus1_16_Bitmap()
    {
        return s_asset0state->m_MPlus1_16_bitmap;
    }

    BitmapFont MPlus1_24_Bitmap()
    {
        return s_asset0state->m_MPlus1_24_bitmap;
    }

    SdfFont MPlus1_Sdf()
    {
        return s_asset0state->m_MPlus1_Sdf_font;
    }

    SdfFont RocknRoll_Sdf()
    {
        return s_asset0state->m_RocknRoll_Sdf_font;
    }
}

namespace Immediate2D_Text
{
    Immediate2D::Text RocknRoll_24_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::RocknRoll_24_Bitmap(), text);
    }

    Immediate2D::Text MPlus1_16_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::MPlus1_16_Bitmap(), text);
    }

    Immediate2D::Text MPlus1_24_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::MPlus1_24_Bitmap(), text);
    }

    Immediate2D::Text MPlus1_Sdf(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::MPlus1_Sdf(), text);
    }

    Immediate2D::Text RocknRoll_Sdf(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::RocknRoll_Sdf(), text);
    }
}
