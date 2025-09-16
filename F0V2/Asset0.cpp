#include "pch.h"
#include "Asset0.h"

#include "TY/InlineComponent.h"

namespace
{
    struct Asset0State : IInlineComponent
    {
        BitmapFont m_RocknRoll_24_bitmap{};

        BitmapFont m_MPlus1_24_bitmap{};

        Asset0State()
        {
            const std::string rocknRollPath = "asset/font/RocknRoll/RocknRollOne-Regular.ttf";

            m_RocknRoll_24_bitmap = BitmapFont(rocknRollPath, 24);

            const std::string mplus1Path = "asset/font/M_PLUS_1/MPLUS1-Regular.ttf";

            m_MPlus1_24_bitmap = BitmapFont(mplus1Path, 24);
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

    BitmapFont MPlus1_24_Bitmap()
    {
        return s_asset0state->m_MPlus1_24_bitmap;
    }
}

namespace Shape2D_Text
{
    Shape2D::Text RocknRoll_24_Bitmap(const std::u32string& text)
    {
        return Shape2D::Text(Asset0::RocknRoll_24_Bitmap(), text);
    }

    Shape2D::Text MPlus1_24_Bitmap(const std::u32string& text)
    {
        return Shape2D::Text(Asset0::MPlus1_24_Bitmap(), text);
    }
}
