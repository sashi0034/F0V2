#include "pch.h"
#include "ImmediateDrawer_Component.h"

namespace TY::ImmediateDrawer_detail
{
    bool ImmediateDrawerComponent::init()
    {
        assert(not Instance);

        Instance = this;

        return true;
    }

    ImmediateDrawerComponent::~ImmediateDrawerComponent()
    {
        if (Instance == this)
        {
            Instance = nullptr;
        }
    }
}
