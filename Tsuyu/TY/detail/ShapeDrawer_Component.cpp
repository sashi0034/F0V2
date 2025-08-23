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
}
