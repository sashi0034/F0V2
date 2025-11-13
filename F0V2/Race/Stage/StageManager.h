#pragma once
#include "StageStaticCollider.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/CourseTriangleAttribute.h"
#include "Race/Machine/LapProgress.h"
#include "TY/TriangleBvh.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class StageManager : public GameObjectHandle
    {
    public:
        StageManager();

        void init() override;

        float courseLength() const;

        StageStaticCollider& stageStaticCollider();
        const StageStaticCollider& stageStaticCollider() const;

        Array<CourseSegment>& courseSegments();
        const Array<CourseSegment>& courseSegments() const;

        struct start_position
        {
            Float3 position;
            Float3 forward;
            Float3 up;
        };

        start_position getStartPosition(int machineId) const;

        float getDistanceFromStart(const SegmentAndStrip& pos) const;
        float getDistanceFromStart(const LapProgress& pos) const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
