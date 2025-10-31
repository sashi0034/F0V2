#pragma once
#include "Race/Common/CourseModelBuilder.h"
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

        struct ground_hit
        {
            IndexedTriangle triangle;
            GroundTriangleAttribute attribute;
            float distSqFromStart;
            Float3 hitPosition;
        };

        struct gimmick_hit
        {
            IndexedTriangle triangle;
            GimmickTriangleAttribute attribute;
            float distSqFromStart;
        };

        [[nodiscard]]
        std::optional<ground_hit> rayCastGround(const LineSegment3D& ray) const;

        [[nodiscard]]
        Array<gimmick_hit> sphereCastGimmick(const Capsule3D& ray) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
