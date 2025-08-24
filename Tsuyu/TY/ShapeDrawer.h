#pragma once
#include "Shape2D.h"
#include "Shape3D.h"

namespace TY
{
    class ShapeDrawer
    {
    public:
        ShapeDrawer();

        const ShapeDrawer& push(const Shape2D::shape_type& shape) const;

        const ShapeDrawer& push(const Shape3D::shape_type& shape) const;

        const ShapeDrawer& operator <<(const Shape2D::shape_type& shape) const;

        void draw() const;

        static ShapeDrawer& Global();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
