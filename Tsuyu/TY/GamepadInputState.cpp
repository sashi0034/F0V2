#include "pch.h"
#include "GamepadInputState.h"

namespace TY
{
    Array<int> GamepadInputState::getDownButtonIndexes() const
    {
        Array<int> downButtons{};
        for (int i = 0; i < buttons.size(); ++i)
        {
            if (buttons[i].down)
            {
                downButtons.push_back(i);
            }
        }

        return downButtons;
    }

    Array<int> GamepadInputState::getActiveAxisIndexes(float threshold) const
    {
        Array<int> activeAxes{};
        for (int i = 0; i < axes.size(); ++i)
        {
            if (std::abs(axes[i]) >= threshold)
            {
                activeAxes.push_back(i);
            }
        }

        return activeAxes;
    }
}
