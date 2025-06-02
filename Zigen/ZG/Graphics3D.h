#pragma once
#include "Mat4x4.h"

namespace ZG
{
    class VertexBuffer_impl;
    class IndexBuffer;

    namespace Graphics3D
    {
        void SetViewMatrix(const Mat4x4& viewMatrix);

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);

        void DrawTriangles(const VertexBuffer_impl& vertexBuffer, const IndexBuffer& indexBuffer);
    }
}
