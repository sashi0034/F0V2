#pragma once
#include "Shape2D.h"

namespace TY
{
    class ShapeDrawer2D
    {
    public:
        ShapeDrawer2D();

        const ShapeDrawer2D& push(const Shape2D::shape_type& shape) const;

        const ShapeDrawer2D& operator <<(const Shape2D::shape_type& shape) const;

        void draw() const;

        static ShapeDrawer2D& Global();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
