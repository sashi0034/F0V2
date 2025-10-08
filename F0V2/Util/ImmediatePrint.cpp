#include "pch.h"
#include "ImmediatePrint.h"

#include "Asset0.h"
#include "ColorPalette.h"
#include "TY/Addon.h"
#include "TY/Array.h"
#include "TY/IAddon.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Scene.h"
#include "TY/Utils.h"

namespace
{
    std::array<Array<std::u32string>, 9> s_texts{};

    struct ImmediatePrintAddon : IAddon
    {
        void draw() override
        {
            ImmediateBuffer textBuffer{};
            for (int a = 0; a < s_texts.size(); ++a)
            {
                const auto& texts = s_texts[a];
                if (texts.empty())
                {
                    continue;
                }

                const auto align = static_cast<Alignment9>(a);
                constexpr float lineHeight = 16.0f;

                Float2 offset = Scene::RectF().getRelativePoint(align);
                offset.y -= (texts.size() - 1) * lineHeight * AlignmentToPivot(align).y;

                for (int y = 0; y < texts.size(); ++y)
                {
                    const auto t =
                        Immediate2D_Text::MPlus1_16_Bitmap(texts[y])
                        .setPosition(offset + Float2{0, y * lineHeight}, align)
                        .setColor(ColorPalette::GamingGreen)
                        .cache();

                    Immediate2D::Rect{t.region.stretched(1.5f)}.setColor(ColorF32{0.0f}).pushAuto();

                    textBuffer.append(t);
                }
            }

            textBuffer.pushAuto();

            ImmediateDrawer::Global().draw();

            s_texts.fill({});
        }
    };
}

namespace Util
{
    void InitImmediatePrintAddon()
    {
        Addon::Register<ImmediatePrintAddon>("ImmediatePrint");
    }

    void ImmediatePrint(const std::string& message, Alignment9 align)
    {
        ImmediatePrint(ToUtf32(message), align);
    }

    void ImmediatePrint(const std::u32string& message, Alignment9 align)
    {
        s_texts[static_cast<int>(align)].push_back(message);
    }
}
