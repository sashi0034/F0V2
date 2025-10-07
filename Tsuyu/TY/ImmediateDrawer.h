#pragma once
#include "Immediate2D.h"
#include "Immediate3D.h"

namespace TY
{
    class ImmediateDrawer
    {
    public:
        ImmediateDrawer();

        const ImmediateDrawer& push(const Immediate2D::shape_type& shape) const;

        const ImmediateDrawer& push(const Immediate3D::shape_type& shape) const;

        const ImmediateDrawer& operator <<(const Immediate2D::shape_type& shape) const;

        const ImmediateDrawer& operator <<(const Immediate3D::shape_type& shape) const;

        friend void operator >>(const Immediate2D::shape_type& shape, const ImmediateDrawer& drawer);

        friend void operator >>(const Immediate3D::shape_type& shape, const ImmediateDrawer& drawer);

        void draw() const;

        static ImmediateDrawer& Global();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
