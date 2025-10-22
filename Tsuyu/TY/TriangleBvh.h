#pragma once
#include "Array.h"
#include "IndexedTriangle.h"
#include "PrimitiveTypes3D.h"
#include "Variant.h"

namespace TY
{
    class TriangleBvh
    {
        struct Impl;

    public:
        TriangleBvh() = default;

        explicit TriangleBvh(const Array<IndexedTriangle>& tris);

        struct Node
        {
            Aabb3D aabb{};
            uint32_t leftIndex{};
            uint32_t rightIndex{};
            uint32_t triOffset{};
            uint32_t triCount{};
            bool isLeaf{};

            bool isBranch() const
            {
                return not isLeaf;
            }

            void forEachTriangle(const std::function<void(const IndexedTriangle&)>& func, const Impl* p_impl) const;
        };

        struct BranchNodeReference;

        struct LeafNodeReference;

        struct NodeReference
        {
            const Node* node{};
            std::shared_ptr<Impl> p_impl{};

            operator bool() const;

            [[nodiscard]]
            Aabb3D aabb() const;

            [[nodiscard]]
            BranchNodeReference asBranch() const;

            [[nodiscard]]
            LeafNodeReference asLeaf() const;
        };

        struct BranchNodeReference : NodeReference
        {
            [[nodiscard]]
            NodeReference left() const;

            [[nodiscard]]
            NodeReference right() const;
        };

        struct LeafNodeReference : NodeReference
        {
            [[nodiscard]]
            size_t triCount() const;
        };

        struct NodeList
        {
            Array<const Node*> list{};
            std::shared_ptr<Impl> p_impl{};

            void forEachTriangle(const std::function<void(const IndexedTriangle&)>& func) const;
        };

        [[nodiscard]]
        Aabb3D aabb() const;

        [[nodiscard]]
        NodeReference root() const;

        [[nodiscard]]
        NodeList queryHits(const Aabb3D& aabb) const;

        Array<IndexedTriangle> queryHitsAndMerge(const Aabb3D& aabb) const;

        [[nodiscard]]
        std::optional<IndexedTriangle> rayCast(const LineSegment3D& segment) const;

        [[nodiscard]]
        std::optional<IndexedTriangle> sphereCast(const Capsule3D& capsule) const;

    private:
        std::shared_ptr<Impl> p_impl;
    };
}
