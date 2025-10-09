#pragma once
#include "Race/Common/CourseData.h"
#include "Race/Common/CourseTriangleAttribute.h"
#include "TY/TriangleBvh.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class StageManager : public GameObjectHandle
    {
    public:
        StageManager();

        void init() override;

        TriangleBvh& staticBvh();
        const TriangleBvh& staticBvh() const;

        const CourseTriangleAttribute& fetchTriangleAttribute(uint64_t index) const;

        Array<CourseSegment>& courseSegments();
        const Array<CourseSegment>& courseSegments() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
