#pragma once
#include "GameObjectBase.h"
#include "TY/Array.h"

namespace TY
{
    class GameObjectHierarchy
    {
    public:
        using list_type = Array<std::shared_ptr<GameObjectBase>>;

        void push(const std::shared_ptr<GameObjectBase>& obj);

        const list_type& list() const
        {
            return m_list;
        }

    private:
        list_type m_list{};
    };

    inline GameObjectHierarchy GlobalGameObjectHierarchy{};
}
