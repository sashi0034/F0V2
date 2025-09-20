#include "pch.h"
#include "GameObjectHierarchy.h"

namespace TY
{
    void GameObjectHierarchy::push(const std::shared_ptr<GameObjectBase>& obj)
    {
        if (obj)
        {
            m_list.push_back(obj);
        }
    }
}
