#include "pch.h"
#include "StageStaticCollider.h"

#include "TY/Grid.h"
#include "TY/Intersects3D.h"
#include "TY/Rect.h"

using namespace Race;

namespace
{
    constexpr int gridDivisions = 8;
}

struct StageStaticCollider::Impl
{
    // m_groundBvh などの要素番号を収納したマップ
    Grid<Array<int>> m_xzMap{gridDivisions, gridDivisions};
    SizeF m_xzCellSize{};

    Aabb3D m_stageAabb{};

    Array<TriangleBvh> m_groundBvh{};
    Array<Array<CourseTriangleAttribute>> m_groundAttributes{};

    void Build(Array<CoursePolygoneCollider> coursePolygoneList)
    {
        if (coursePolygoneList.empty())
        {
            return;
        }

        for (int i = 0; i < coursePolygoneList.size(); ++i)
        {
            m_groundBvh.push_back(TriangleBvh(coursePolygoneList[i].tris));
            m_groundAttributes.push_back(std::move(coursePolygoneList[i].attributes));
        }

        // -----------------------------------------------

        m_stageAabb = m_groundBvh[0].aabb();
        for (const auto& bvh : m_groundBvh)
        {
            m_stageAabb.min = MinVector3(m_stageAabb.min, bvh.aabb().min);
            m_stageAabb.max = MaxVector3(m_stageAabb.max, bvh.aabb().max);
        }

        m_xzCellSize = SizeF{
            (m_stageAabb.max.x - m_stageAabb.min.x) / static_cast<float>(gridDivisions),
            (m_stageAabb.max.z - m_stageAabb.min.z) / static_cast<float>(gridDivisions)
        };

        // -----------------------------------------------

        for (int i = 0; i < m_groundBvh.size(); ++i)
        {
            Rect region = mapRegionInStage(m_groundBvh[i].aabb());
            for (int y = region.topY(); y < region.bottomY(); ++y)
            {
                for (int x = region.leftX(); x < region.rightX(); ++x)
                {
                    m_xzMap[{x, y}].push_back(i);
                }
            }
        }
    }

    std::optional<hit_type> RayCast(const LineSegment3D& ray)
    {
        std::unordered_set<int> indices{};
        Rect region = mapRegionInStage(ray.aabb());
        for (int y = region.topY(); y < region.bottomY(); ++y)
        {
            for (int x = region.leftX(); x < region.rightX(); ++x)
            {
                for (int index : m_xzMap[{x, y}])
                {
                    indices.insert(index);
                }
            }
        }

        const Float3 startPoint = ray.p0;

        float bestDistSq = std::numeric_limits<float>::max();
        std::optional<std::pair<IndexedTriangle, CourseTriangleAttribute*>> bestTri{};

        for (const int index : indices)
        {
            Array<IndexedTriangle> candidates = m_groundBvh[index].queryHitsAndMerge(ray.aabb());

            for (const auto& tri : candidates)
            {
                const float distSq = (tri.centroid() - startPoint).lengthSq();
                if (distSq >= bestDistSq)
                {
                    // これ以上は遠い
                    continue;
                }

                if (Intersects(ray, tri))
                {
                    bestTri = {tri, &m_groundAttributes[index][tri.id]};
                    bestDistSq = distSq;
                }
            }
        }

        if (bestTri.has_value())
        {
            return hit_type{
                .triangle = bestTri->first,
                .attribute = *(bestTri->second)
            };
        }
        else
        {
            return std::nullopt;
        }
    }

private:
    Rect mapRegionInStage(const Aabb3D& targetAabb) const
    {
        Point relativeMin = ((targetAabb.min.xz() - m_stageAabb.min.xz()) / m_xzCellSize).asPoint();
        Point relativeMax = ((targetAabb.max.xz() - m_stageAabb.min.xz()) / m_xzCellSize).asPoint();

        relativeMin = MaxVector2(relativeMin, Point{});
        relativeMax = MinVector2(relativeMax, Point{gridDivisions - 1, gridDivisions - 1});
        return Rect{
            Point{relativeMin.x, relativeMin.y},
            Point{relativeMax.x - relativeMin.x + 1, relativeMax.y - relativeMin.y + 1}
        };
    }
};

namespace Race
{
    StageStaticCollider::StageStaticCollider() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void StageStaticCollider::build(Array<CoursePolygoneCollider> coursePolygoneList)
    {
        p_impl->Build(std::move(coursePolygoneList));
    }

    std::optional<StageStaticCollider::hit_type> StageStaticCollider::rayCast(const LineSegment3D& segment) const
    {
        return p_impl->RayCast(segment);
    }
}
