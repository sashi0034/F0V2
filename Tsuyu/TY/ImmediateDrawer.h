#pragma once
#include "Shape2D.h"
#include "Shape3D.h"

namespace TY
{
    class ImmediateDrawer
    {
    public:
        ImmediateDrawer();

        const ImmediateDrawer& push(const Shape2D::shape_type& shape) const;

        const ImmediateDrawer& push(const Shape3D::shape_type& shape) const;

        const ImmediateDrawer& operator <<(const Shape2D::shape_type& shape) const;

        const ImmediateDrawer& operator <<(const Shape3D::shape_type& shape) const;

        friend void operator >>(const Shape2D::shape_type& shape, const ImmediateDrawer& drawer);

        friend void operator >>(const Shape3D::shape_type& shape, const ImmediateDrawer& drawer);

        void draw() const;

        static ImmediateDrawer& Global();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
