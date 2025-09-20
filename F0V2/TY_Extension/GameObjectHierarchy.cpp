#include "pch.h"
#include "GameObjectHierarchy.h"

#include "TY/InlineComponent.h"

namespace
{
    struct GlobalGameObjectHierarchyState : IInlineComponent
    {
        GameObjectHierarchy instance{};
    };

    InlineComponent<GlobalGameObjectHierarchyState> s_globalGameObjectHierarch{};
}

namespace TY
{
    void GameObjectHierarchy::push(const std::shared_ptr<GameObjectBase>& obj)
    {
        if (obj)
        {
            m_list.push_back(obj);
        }
    }

    GameObjectHierarchy& GlobalGameObjectHierarchy()
    {
        return s_globalGameObjectHierarch->instance;
    }
}
