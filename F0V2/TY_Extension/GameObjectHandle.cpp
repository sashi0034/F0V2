#include "pch.h"

#include "GameObjectHandle.h"

#include "GameObjectHierarchy.h"

namespace TY
{
    void GameObjectHandle::init()
    {
        if (const auto gameObject = asGameObject())
        {
            GlobalGameObjectHierarchy.push(gameObject);
        }
    }
}
