#include "pch.h"
#include "Asset0.h"

#include "TY/InlineComponent.h"

namespace
{
    struct Asset0State : IInlineComponent
    {
        BitmapFont m_RocknRoll_24_bitmap{};

        Asset0State()
        {
            const std::string rocknRollPath = "asset/font/RocknRoll/RocknRollOne-Regular.ttf";

            m_RocknRoll_24_bitmap = BitmapFont(rocknRollPath, 24);
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
}

namespace Shape2D_Text
{
    Shape2D::Text RocknRoll_24_Bitmap(const std::u32string& text)
    {
        return Shape2D::Text(Asset0::RocknRoll_24_Bitmap(), text);
    }
}
