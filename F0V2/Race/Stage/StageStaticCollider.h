#pragma once
#include "Race/Common/CourseBuilder.h"
#include "Race/Common/CourseTriangleAttribute.h"
#include "TY/Array.h"
#include "TY/IndexedTriangle.h"

namespace Race
{
    class StageStaticCollider
    {
    public:
        StageStaticCollider();

        void build(Array<CoursePolygoneCollider> coursePolygoneList);

        struct hit_type
        {
            IndexedTriangle triangle;
            CourseTriangleAttribute attribute;
        };

        [[nodiscard]]
        std::optional<hit_type> rayCast(const LineSegment3D& ray) const;

        [[nodiscard]]
        std::optional<hit_type> sphereCast(const Capsule3D& ray) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
