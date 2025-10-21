#pragma once
#include "Array.h"
#include "IndexedTriangle.h"
#include "PrimitiveTypes3D.h"
#include "Variant.h"

namespace TY
{
    class TriangleBvh
    {
    public:
        TriangleBvh() = default;

        TriangleBvh(const Array<IndexedTriangle>& tris);

        struct Node;

        struct Branch
        {
            Aabb3D aabb{};
            std::unique_ptr<Node> left{};
            std::unique_ptr<Node> right{};

            explicit Branch(const Aabb3D& aabb, std::unique_ptr<Node> left, std::unique_ptr<Node> right);
        };

        struct Leaf
        {
            Aabb3D aabb{};
            Array<IndexedTriangle> tris{};

            explicit Leaf(const Aabb3D& aabb, const Array<IndexedTriangle>& tris);
        };

        struct Node : Variant<Branch, Leaf>
        {
            using Variant::Variant;

            const Leaf* asLeaf() const;

            const Branch* asBranch() const;

            Aabb3D aabb() const;

            void forEachTriangle(const std::function<void(const IndexedTriangle&)>& func) const;
        };

        struct NodeList
        {
            Array<Node> list{};

            void forEachTriangle(const std::function<void(const IndexedTriangle&)>& func) const;
        };

        [[nodiscard]]
        Aabb3D aabb() const;

        [[nodiscard]]
        const std::shared_ptr<Node>& root() const;

        [[nodiscard]]
        NodeList queryHits(const Aabb3D& aabb) const;

        Array<IndexedTriangle> queryHitsAndMerge(const Aabb3D& aabb) const;

        [[nodiscard]]
        std::optional<IndexedTriangle> rayCast(const LineSegment3D& segment) const;

        [[nodiscard]]
        std::optional<IndexedTriangle> sphereCast(const Capsule3D& capsule) const;

    private:
        class Internal;

        std::shared_ptr<Node> m_root;
    };
}
