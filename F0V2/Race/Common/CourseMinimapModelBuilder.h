#pragma once
#include "TY/ModelBuffer.h"

namespace Race
{
    struct CourseMinimapFace
    {
        Float3 position{};
        Float3 normal{};
    };

    /// @brief ミニマップ用の軽量なコースモデルを構築
    class CourseMinimapModelBuilder
    {
    public:
        void pushGroundQuad(
            const CourseMinimapFace& l0,
            const CourseMinimapFace& r0,
            const CourseMinimapFace& l1,
            const CourseMinimapFace& r1);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        ModelBuffer build() const;

    private:
        Array<ModelVertex> m_vertices{};
        Array<uint16_t> m_indices{};
    };
}
