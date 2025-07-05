#include "pch.h"
#include "KeyboardMouseInput.h"

#include "Array.h"
#include "InlineComponent.h"
#include "System.h"
#include "detail/EngineKeyboardMouse.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    class AllInputsCache : public IInlineComponent
    {
    public:
        const Array<KeyboardMouseInput>& Fetch()
        {
            if (System::FrameCount() == m_lastTimestamp)
            {
                return m_allInputs;
            }

            m_lastTimestamp = System::FrameCount();

            m_allInputs.clear();
            for (const auto code : EngineKeyboardMouse::ChangedCodes())
            {
                m_allInputs.push_back(KeyboardMouseInput{code});
            }

            return m_allInputs;
        }

    private:
        Array<KeyboardMouseInput> m_allInputs{};
        uint64_t m_lastTimestamp{};
    };

    InlineComponent<AllInputsCache> s_allInputs{};
}

namespace TY
{
    using namespace detail;

    bool KeyboardMouseInput::down() const
    {
        return EngineKeyboardMouse::KeyDown(m_code);
    }

    bool KeyboardMouseInput::pressed() const
    {
        return EngineKeyboardMouse::KeyPressed(m_code);
    }

    bool KeyboardMouseInput::up() const
    {
        return EngineKeyboardMouse::KeyUp(m_code);
    }

    uint8_t KeyboardMouseInput::code() const
    {
        return m_code;
    }

    const Array<KeyboardMouseInput>& KeyboardMouse::GetAllInputs()
    {
        return s_allInputs->Fetch();
    }
}
