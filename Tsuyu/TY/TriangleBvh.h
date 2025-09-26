#pragma once
#include "Array.h"
#include "PrimitiveTypes3D.h"
#include "Variant.h"

namespace TY
{
    class TriangleBvh
    {
    public:
        TriangleBvh() = default;

        TriangleBvh(const Array<Triangle3D>& tris);

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
            Array<Triangle3D> tris{};

            explicit Leaf(const Aabb3D& aabb, const Array<Triangle3D>& tris);
        };

        struct Node : Variant<Branch, Leaf>
        {
            using Variant::Variant;

            void forEachTriangle(const std::function<void(const Triangle3D&)>& func) const;
        };

        struct NodeList
        {
            Array<Node> list{};

            void forEachTriangle(const std::function<void(const Triangle3D&)>& func) const;
        };

        NodeList queryHits(const Aabb3D& aabb) const;

    private:
        class Internal;

        std::unique_ptr<Node> m_root;
    };
}
