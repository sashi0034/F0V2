#include "pch.h"
#include "TriangleBvh.h"

#include "Intersects3D.h"
#include "Variant.h"

using namespace TY;

namespace
{
    int longestAxis(const Aabb3D& aabb)
    {
        const Float3 size = aabb.max - aabb.min;
        if (size.x > size.y && size.x > size.z) return 0;
        if (size.y > size.z) return 1;
        return 2;
    }

    Aabb3D computeAabb(const Array<IndexedTriangle>& tris)
    {
        Aabb3D result{};
        if (tris.empty()) return result;

        result.min = {FLT_MAX, FLT_MAX, FLT_MAX};
        result.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (const auto& tri : tris)
        {
            result.min = MinVector3(result.min, tri.aabb().min);
            result.max = MaxVector3(result.max, tri.aabb().max);
        }
        return result;
    }
}

struct TriangleBvh::Impl
{
    Array<IndexedTriangle> m_tris{};
    Array<Node> m_nodes{};

    uint32_t BuildRecursive(const Array<IndexedTriangle>& tris)
    {
        const uint32_t nodeIndex = static_cast<uint32_t>(m_nodes.size());
        m_nodes.push_back({});
        m_nodes[nodeIndex].aabb = computeAabb(tris);

        if (tris.size() <= 4)
        {
            m_nodes[nodeIndex].isLeaf = true;
            m_nodes[nodeIndex].triOffset = static_cast<uint32_t>(m_tris.size());
            m_nodes[nodeIndex].triCount = static_cast<uint32_t>(tris.size());
            m_tris.insert(m_tris.end(), tris.begin(), tris.end());
            return nodeIndex;
        }

        // 最大軸の中央で分割
        const int axis = longestAxis(m_nodes[nodeIndex].aabb);
        const float mid = 0.5f * (m_nodes[nodeIndex].aabb.min.elem(axis) + m_nodes[nodeIndex].aabb.max.elem(axis));

        Array<IndexedTriangle> left{}, right{};
        left.reserve(tris.size());
        right.reserve(tris.size());
        for (const auto& t : tris)
        {
            const float centroid = t.centroid().elem(axis);

            if (centroid < mid)
            {
                left.push_back(t);
            }
            else
            {
                right.push_back(t);
            }
        }

        if (left.empty() || right.empty())
        {
            // 分割失敗 --> leaf
            m_nodes[nodeIndex].isLeaf = true;
            m_nodes[nodeIndex].triOffset = static_cast<uint32_t>(m_tris.size());
            m_nodes[nodeIndex].triCount = static_cast<uint32_t>(tris.size());
            m_tris.insert(m_tris.end(), tris.begin(), tris.end());
            return nodeIndex;
        }

        // 左右ノードを追加
        m_nodes[nodeIndex].leftIndex = BuildRecursive(left);
        m_nodes[nodeIndex].rightIndex = BuildRecursive(right);

        return nodeIndex;
    }

    void QueryHits(const Aabb3D& aabb, Array<const Node*>& outHits) const
    {
        if (m_nodes.empty())
        {
            return;
        }

        std::stack<uint32_t> stack{};
        stack.push(0);

        while (not stack.empty())
        {
            const uint32_t i = stack.top();
            stack.pop();
            const Node& node = m_nodes[i];

            if (not Intersects(aabb, node.aabb))
            {
                continue;
            }

            if (node.isLeaf)
            {
                outHits.push_back(&node);
            }
            else
            {
                stack.push(node.leftIndex);
                stack.push(node.rightIndex);
            }
        }
    }

    static Array<IndexedTriangle> QueryHitsAndMerge(const TriangleBvh& bvh, const Aabb3D& aabb)
    {
        Array<IndexedTriangle> candidates = {};
        candidates.reserve(64);
        const auto hits = bvh.queryHits(aabb);
        hits.forEachTriangle([&](const IndexedTriangle& tri)
        {
            candidates.push_back(tri);
        });

        return candidates;
    }

    template <typename T>
    // using T = LineSegment3D; // for debug
    static std::optional<IndexedTriangle> RayCast(const TriangleBvh& bvh, const T& ray)
    {
        Array<IndexedTriangle> candidates = QueryHitsAndMerge(bvh, ray.aabb());

        const Float3 startPoint = ray.p0;

        float bestDistSq = std::numeric_limits<float>::max();
        std::optional<IndexedTriangle> bestTri{};
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
                bestTri = tri;
                bestDistSq = distSq;
            }
        }

        return bestTri;
    }
};

namespace TY
{
    TriangleBvh::TriangleBvh(const Array<IndexedTriangle>& tris) : p_impl(std::make_shared<Impl>())
    {
        p_impl->BuildRecursive(tris);
    }

    void TriangleBvh::Node::forEachTriangle(
        const std::function<void(const IndexedTriangle&)>& func,
        const Impl* p_impl) const
    {
        if (not p_impl)
        {
            return;
        }

        if (p_impl->m_nodes.empty() || p_impl->m_tris.empty())
        {
            return;
        }

        // 明示的スタックを用いた非再帰走査
        Array<const Node*> stack;
        stack.reserve(64);
        stack.push_back(this);

        while (not stack.empty())
        {
            const Node* node = stack.back();
            stack.pop_back();

            if (node->isLeaf)
            {
                const uint32_t start = node->triOffset;
                const uint32_t end = start + node->triCount;
                for (uint32_t i = start; i < end; ++i)
                {
                    func(p_impl->m_tris[i]);
                }
            }
            else
            {
                // 子ノードを後入れ先出しでスタックへ
                if (node->rightIndex < p_impl->m_nodes.size())
                {
                    stack.push_back(&p_impl->m_nodes[node->rightIndex]);
                }
                if (node->leftIndex < p_impl->m_nodes.size())
                {
                    stack.push_back(&p_impl->m_nodes[node->leftIndex]);
                }
            }
        }
    }

    TriangleBvh::NodeReference::operator bool() const
    {
        return node && p_impl;
    }

    Aabb3D TriangleBvh::NodeReference::aabb() const
    {
        return node ? node->aabb : Aabb3D{};
    }

    TriangleBvh::BranchNodeReference TriangleBvh::NodeReference::asBranch() const
    {
        return node && node->isBranch() ? static_cast<BranchNodeReference>(*this) : BranchNodeReference{};
    }

    TriangleBvh::LeafNodeReference TriangleBvh::NodeReference::asLeaf() const
    {
        return node && node->isLeaf ? static_cast<LeafNodeReference>(*this) : LeafNodeReference{};
    }

    TriangleBvh::NodeReference TriangleBvh::BranchNodeReference::left() const
    {
        return node && p_impl && node->isBranch() && node->leftIndex < p_impl->m_nodes.size()
                   ? NodeReference{&p_impl->m_nodes[node->leftIndex], p_impl}
                   : NodeReference{};
    }

    TriangleBvh::NodeReference TriangleBvh::BranchNodeReference::right() const
    {
        return node && p_impl && node->isBranch() && node->rightIndex < p_impl->m_nodes.size()
                   ? NodeReference{&p_impl->m_nodes[node->rightIndex], p_impl}
                   : NodeReference{};
    }

    size_t TriangleBvh::LeafNodeReference::triCount() const
    {
        return node && node->isLeaf ? static_cast<size_t>(node->triCount) : 0;
    }

    void TriangleBvh::NodeList::forEachTriangle(const std::function<void(const IndexedTriangle&)>& func) const
    {
        for (const auto& node : list)
        {
            node->forEachTriangle(func, p_impl.get());
        }
    }

    Aabb3D TriangleBvh::aabb() const
    {
        if (not p_impl)
        {
            return Aabb3D{};
        }

        return p_impl->m_nodes.empty() ? Aabb3D{} : p_impl->m_nodes[0].aabb;
    }

    TriangleBvh::NodeReference TriangleBvh::root() const
    {
        if (not p_impl || p_impl->m_nodes.empty())
        {
            return {};
        }

        return {&p_impl->m_nodes[0], p_impl};
    }

    TriangleBvh::NodeList TriangleBvh::queryHits(const Aabb3D& aabb) const
    {
        if (not p_impl)
        {
            return {};
        }

        NodeList result{};
        p_impl->QueryHits(aabb, result.list);
        result.p_impl = p_impl;
        return result;
    }

    Array<IndexedTriangle> TriangleBvh::queryHitsAndMerge(const Aabb3D& aabb) const
    {
        if (not p_impl)
        {
            return {};
        }

        return Impl::QueryHitsAndMerge(*this, aabb);
    }

    std::optional<IndexedTriangle> TriangleBvh::rayCast(const LineSegment3D& segment) const
    {
        if (not p_impl)
        {
            return std::nullopt;
        }

        return Impl::RayCast(*this, segment);
    }

    std::optional<IndexedTriangle> TriangleBvh::sphereCast(const Capsule3D& capsule) const
    {
        if (not p_impl)
        {
            return std::nullopt;
        }

        return Impl::RayCast(*this, capsule);
    }
}
