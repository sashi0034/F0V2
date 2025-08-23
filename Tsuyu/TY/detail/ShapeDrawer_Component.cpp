#include "pch.h"
#include "ShapeDrawer_Component.h"

namespace TY::ShapeDrawer_detail
{
    bool ShapeDrawerComponent::init()
    {
        assert(not Instance);

        Instance = this;

        return true;
    }

    ShapeDrawerComponent::~ShapeDrawerComponent()
    {
        if (Instance == this)
        {
            Instance = nullptr;
        }
    }

    void ShapeDrawerComponent::beforeFlush()
    {
        for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
        {
            it->get()->beforeFlush();

            if (it->get()->m_shouldRemove)
            {
                it = m_subscribableList.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ShapeDrawerComponent::afterPresent()
    {
        for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
        {
            it->get()->afterPresent();
            ++it;
        }
    }
}
