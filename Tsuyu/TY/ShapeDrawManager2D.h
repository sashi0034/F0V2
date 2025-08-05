#pragma once
#include "Shape2D.h"

namespace TY
{
    class ShapeDrawManager2D
    {
    public:
        ShapeDrawManager2D();

        ShapeDrawManager2D& push(const Shape2D::shape_type& shape);

        void draw();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
