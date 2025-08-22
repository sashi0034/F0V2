#include "pch.h"
#include "FreeTypeContext.h"

#include "EngineComponent.h"
#include "TY/IComponent.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct FreeTypeContextComponent;

    FreeTypeContextComponent* s_freeTypeContext{};

    struct FreeTypeContextComponent : IComponent
    {
        FT_Library m_library{};

        bool init() override
        {
            assert(not s_freeTypeContext);
            s_freeTypeContext = this;

            if (FT_Init_FreeType(&m_library))
            {
                LogError("FreeTypeContext: Failed FT_Init_FreeType()");
                return false;
            }

            return true;
        }

        ~FreeTypeContextComponent()
        {
            if (m_library)
            {
                FT_Done_FreeType(m_library);
                m_library = nullptr;
            }

            if (s_freeTypeContext == this)
            {
                s_freeTypeContext = nullptr;
            }
        }
    };
}

namespace TY::detail
{
    void InitFreeTypeContextComponent()
    {
        EngineComponent::Register<FreeTypeContextComponent>("FreeTypeContextComponent");
    }

    FT_Library GetFreeType()
    {
        assert(s_freeTypeContext->m_library);
        return s_freeTypeContext->m_library;
    }
}
