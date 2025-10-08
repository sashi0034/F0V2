#include "pch.h"
#include "ImmediatePrint.h"

#include "Asset0.h"
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
            for (int a = 0; a < s_texts.size(); ++a)
            {
                const auto& texts = s_texts[a];
                if (texts.empty())
                {
                    continue;
                }

                const auto align = static_cast<Alignment9>(a);
                for (int y = 0; y < texts.size(); ++y)
                {
                    Immediate2D_Text::MPlus1_16_Bitmap(texts[y])
                        .setPosition(Scene::RectF().getRelativePoint(align) + Float2{0, y * 20.0f}, align)
                        .setColor(ColorF32{1.0f})
                        .cache()
                        .pushAuto();
                }
            }

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
