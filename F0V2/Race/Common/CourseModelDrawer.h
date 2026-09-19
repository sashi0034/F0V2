#pragma once
#include "CourseModelShape.h"
#include "TY/GenericModelDrawer.h"
#include "TY/GraphicsOptions.h"

namespace Race
{
    /// @brief gbuffer_course1.hlsl 固定
    class CourseModelDrawer
    {
    public:
        CourseModelDrawer() = default;

        CourseModelDrawer(const CourseModelData& modelData, const GraphicsOptions& options);

        void draw() const;

    private:
        GenericModelDrawer m_impl{};
    };
}
