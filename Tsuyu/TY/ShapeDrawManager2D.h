#pragma once
#include "Shape2D.h"

namespace TY
{
    class ShapeDrawManager2D
    {
    public:
        ShapeDrawManager2D();

        const ShapeDrawManager2D& push(const Shape2D::shape_type& shape) const;

        const ShapeDrawManager2D& operator<<(const Shape2D::shape_type& shape) const;

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
