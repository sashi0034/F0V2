#include "pch.h"
#include "StageStaticCollider.h"

#include "TY/Grid.h"
#include "TY/Intersects3D.h"
#include "TY/Rect.h"
#include "Util/ImmediatePrint.h"

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
    Array<Array<GroundTriangleAttribute>> m_groundAttributes{};

    Array<TriangleBvh> m_gimmickBvh{};
    Array<Array<GimmickTriangleAttribute>> m_gimmickAttributes{};

    void Build(Array<CoursePolygoneCollider> coursePolygoneList)
    {
        if (coursePolygoneList.empty())
        {
            return;
        }

        for (int i = 0; i < coursePolygoneList.size(); ++i)
        {
            m_groundBvh.push_back(std::move(TriangleBvh(coursePolygoneList[i].groundTris)));
            m_groundAttributes.push_back(std::move(coursePolygoneList[i].groundAttrs));

            m_gimmickBvh.push_back(std::move(TriangleBvh(coursePolygoneList[i].gimmickTris)));
            m_gimmickAttributes.push_back(std::move(coursePolygoneList[i].gimmickAttrs));
        }

        // -----------------------------------------------

        // FIXME: m_gimmickBvh も考慮すべきかも
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

    std::optional<ground_hit> RayCastGround(const LineSegment3D& ray)
    {
        const std::unordered_set<int> indices = collectIndicesInRegion(ray.aabb());

        const Float3 startPoint = ray.p0;

        float bestDistSq = FLT_MAX;
        std::optional<std::pair<IndexedTriangle, GroundTriangleAttribute*>> bestTri{};
        Float3 hitPosition{};

#if defined(_DEBUG)
        int testCount{};
#endif

        for (const int index : indices)
        {
            Array<IndexedTriangle> candidates = m_groundBvh[index].queryHitsAndMerge(ray.aabb());

#if defined(_DEBUG)
            testCount += candidates.size();
#endif

            for (const auto& tri : candidates)
            {
                const float distSq = (tri.centroid() - startPoint).lengthSq();
                if (distSq >= bestDistSq)
                {
                    // これ以上は遠い
                    continue;
                }

                if (const auto hitPosition_ = IntersectsAt(ray, tri))
                {
                    bestTri = {tri, &m_groundAttributes[index][tri.id]};
                    bestDistSq = distSq;
                    hitPosition = *hitPosition_;
                }
            }
        }

#if defined(_DEBUG)
        ImmediatePrint(
            std::format("RayCastGround(): testCount: {}", testCount),
            Alignment9::BottomLeft);
#endif

        if (bestTri.has_value())
        {
            return ground_hit{
                .triangle = bestTri->first,
                .attribute = *(bestTri->second),
                .distSqFromStart = bestDistSq,
                .hitPosition = hitPosition
            };
        }
        else
        {
            return std::nullopt;
        }
    }

    Array<gimmick_hit> SphereCastGimmick(const Capsule3D& ray)
    {
        const std::unordered_set<int> indices = collectIndicesInRegion(ray.aabb());

        const Float3 startPoint = ray.p0;

#if defined(_DEBUG)
        int testCount{};
#endif

        Array<gimmick_hit> result{};
        for (const int index : indices)
        {
            Array<IndexedTriangle> candidates = m_gimmickBvh[index].queryHitsAndMerge(ray.aabb());

#if defined(_DEBUG)
            testCount += candidates.size();
#endif

            for (const auto& tri : candidates)
            {
                if (Intersects(ray, tri))
                {
                    const float distanceSq = (tri.centroid() - startPoint).lengthSq();
                    result.push_back({tri, m_gimmickAttributes[index][tri.id], distanceSq});
                }
            }
        }

#if defined(_DEBUG)
        ImmediatePrint(
            std::format("SphereCastGimmick(): testCount: {}", testCount),
            Alignment9::BottomLeft);
#endif

        // std::ranges::sort(
        //     result,
        //     {},
        //     &gimmick_hit::distSqFromStart);

        return result;
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

    std::unordered_set<int> collectIndicesInRegion(const Aabb3D& aabb)
    {
        std::unordered_set<int> indices;
        Rect region = mapRegionInStage(aabb);
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

        return indices;
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

    std::optional<StageStaticCollider::ground_hit> StageStaticCollider::rayCastGround(const LineSegment3D& ray) const
    {
        return p_impl->RayCastGround(ray);
    }

    Array<StageStaticCollider::gimmick_hit> StageStaticCollider::sphereCastGimmick(const Capsule3D& ray) const
    {
        return p_impl->SphereCastGimmick(ray);
    }
}
