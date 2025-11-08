#include "pch.h"
#include "Asset0.h"

#include "TY/InlineComponent.h"

namespace
{
    struct Asset0State : IInlineComponent
    {
        BitmapFont m_RocknRoll_24_bitmap{};

        SdfFont m_RocknRoll_Sdf_font{};

        BitmapFont m_ZXProto_24_bitmap{};

        SdfFont m_ZXProto_Sdf_font{};

        BitmapFont m_Audiowide_24_bitmap{};

        SdfFont m_Audiowide_Sdf_font{};

        BitmapFont m_MPlus1_16_bitmap{};

        BitmapFont m_MPlus1_24_bitmap{};

        SdfFont m_MPlus1_Sdf_font{};

        Asset0State()
        {
            const std::string rocknRollPath = "asset/font/RocknRoll/RocknRollOne-Regular.ttf";

            m_RocknRoll_24_bitmap = BitmapFont(rocknRollPath, 24);

            m_RocknRoll_Sdf_font = SdfFont(rocknRollPath, 48);

            // -----------------------------------------------

            const std::string zxProtoPath = "asset/font/0xProto/0xProto-Regular.ttf";

            m_ZXProto_24_bitmap = BitmapFont(zxProtoPath, 24);

            m_ZXProto_Sdf_font = SdfFont(zxProtoPath, 48);

            // -----------------------------------------------

            const std::string AudiowidePath = "asset/font/Audiowide/Audiowide-Regular.ttf";

            m_Audiowide_24_bitmap = BitmapFont(AudiowidePath, 24);

            m_Audiowide_Sdf_font = SdfFont(AudiowidePath, 48);

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

    SdfFont RocknRoll_Sdf()
    {
        return s_asset0state->m_RocknRoll_Sdf_font;
    }

    BitmapFont ZXProto_24_Bitmap()
    {
        return s_asset0state->m_ZXProto_24_bitmap;
    }

    SdfFont ZXProto_Sdf()
    {
        return s_asset0state->m_ZXProto_Sdf_font;
    }

    BitmapFont Audiowide_24_Bitmap()
    {
        return s_asset0state->m_Audiowide_24_bitmap;
    }

    SdfFont Audiowide_Sdf()
    {
        return s_asset0state->m_Audiowide_Sdf_font;
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
}

namespace Immediate2D_Text
{
    Immediate2D::Text RocknRoll_24_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::RocknRoll_24_Bitmap(), text);
    }

    Immediate2D::Text RocknRoll_Sdf(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::RocknRoll_Sdf(), text);
    }

    Immediate2D::Text ZXProto_24_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::ZXProto_24_Bitmap(), text);
    }

    Immediate2D::Text ZXProto_Sdf(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::ZXProto_Sdf(), text);
    }

    Immediate2D::Text Audiowide_24_Bitmap(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::Audiowide_24_Bitmap(), text);
    }

    Immediate2D::Text Audiowide_Sdf(const std::u32string& text)
    {
        return Immediate2D::Text(Asset0::Audiowide_Sdf(), text);
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
}
