#include "pch.h"
#include "TriangleBvh.h"

#include "Intersects3D.h"
#include "Variant.h"

using namespace TY;

namespace
{
}

class TriangleBvh::Internal
{
public:
    static std::unique_ptr<Node> BuildNode(Array<Triangle3D> tris, int depth = 0)
    {
        // AABBを計算
        Float3 min{FLT_MAX, FLT_MAX, FLT_MAX};
        Float3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (const auto& tri : tris)
        {
            min = MinVector3(min, tri.p0);
            min = MinVector3(min, tri.p1);
            min = MinVector3(min, tri.p2);

            max = MaxVector3(max, tri.p0);
            max = MaxVector3(max, tri.p1);
            max = MaxVector3(max, tri.p2);
        }

        if (tris.size() <= 4 || depth >= 32)
        {
            return std::make_unique<Node>(Leaf{Aabb3D{min, max}, std::move(tris)});
        }

        // 最も長い軸を見つける
        Float3 extent = max - min;
        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent.elem(axis)) axis = 2;

        // その軸でソート
        std::ranges::sort(
            tris,
            [axis](const Triangle3D& a, const Triangle3D& b)
            {
                return a.centroid().elem(axis) < b.centroid().elem(axis);
            });

        // 中央で分割
        size_t mid = tris.size() / 2;
        Array<Triangle3D> leftTris(tris.begin(), tris.begin() + mid);
        Array<Triangle3D> rightTris(tris.begin() + mid, tris.end());

        return std::make_unique<Node>(Branch{
            Aabb3D{min, max},
            BuildNode(std::move(leftTris), depth + 1),
            BuildNode(std::move(rightTris), depth + 1)
        });
    }

    static void QueryHits(const Node& node, const Aabb3D& aabb, Array<Node>& hits)
    {
        if (const auto leaf = node.asLeaf())
        {
            if (Intersects(aabb, leaf->aabb))
            {
                hits.push_back(*leaf);
            }

            return;
        }

        const auto branch = node.asBranch();
        assert(branch && branch->left && branch->right);

        if (not Intersects(aabb, branch->aabb))
        {
            return;
        }

        QueryHits(*branch->left.get(), aabb, hits);
        QueryHits(*branch->right.get(), aabb, hits);
    }
};

namespace TY
{
    TriangleBvh::TriangleBvh(const Array<Triangle3D>& tris)
        : m_root(Internal::BuildNode(tris))
    {
    }

    TriangleBvh::Branch::Branch(const Aabb3D& aabb, std::unique_ptr<Node> left, std::unique_ptr<Node> right)
        : aabb(aabb), left(std::move(left)), right(std::move(right))
    {
    }

    TriangleBvh::Leaf::Leaf(const Aabb3D& aabb, const Array<Triangle3D>& tris)
        : aabb(aabb), tris(tris)
    {
    }

    const TriangleBvh::Leaf* TriangleBvh::Node::asLeaf() const
    {
        return std::get_if<Leaf>(this);
    }

    const TriangleBvh::Branch* TriangleBvh::Node::asBranch() const
    {
        return std::get_if<Branch>(this);
    }

    Aabb3D TriangleBvh::Node::aabb() const
    {
        return std::visit([](const auto& node) { return node.aabb; }, *this);
    }

    void TriangleBvh::Node::forEachTriangle(const std::function<void(const Triangle3D&)>& func) const
    {
        if (const auto leaf = this->tryGet<Leaf>())
        {
            for (const auto& tri : leaf->tris)
            {
                func(tri);
            }

            return;
        }

        const auto branch = this->asBranch();
        assert(branch && branch->left && branch->right);

        branch->left->forEachTriangle(func);
        branch->right->forEachTriangle(func);
    }

    void TriangleBvh::NodeList::forEachTriangle(const std::function<void(const Triangle3D&)>& func) const
    {
        for (const auto& node : list)
        {
            node.forEachTriangle(func);
        }
    }

    const std::unique_ptr<TriangleBvh::Node>& TriangleBvh::root() const
    {
        return m_root;
    }

    TriangleBvh::NodeList TriangleBvh::queryHits(const Aabb3D& aabb) const
    {
        if (not m_root)
        {
            return {};
        }

        NodeList result{};
        Internal::QueryHits(*m_root.get(), aabb, result.list);
        return result;
    }

    std::optional<Triangle3D> TriangleBvh::sphereCast(const Capsule& capsule) const
    {
        if (not m_root)
        {
            return std::nullopt;
        }

        Array<Triangle3D> candidates{};
        const auto hits = queryHits(capsule.aabb());
        for (const auto& node : hits.list)
        {
            node.forEachTriangle([&](const Triangle3D& tri)
            {
                candidates.push_back(tri);
            });
        }

        const Float3 capsuleCenter = (capsule.p0 + capsule.p1) * 0.5f;
        std::ranges::sort(
            candidates,
            [&capsuleCenter](const Triangle3D& a, const Triangle3D& b)
            {
                return (a.centroid() - capsuleCenter).lengthSq() < (b.centroid() - capsuleCenter).
                    lengthSq();
            });

        for (const auto& tri : candidates)
        {
            if (Intersects(capsule, tri))
            {
                return tri;
            }
        }

        return std::nullopt;
    }
}
