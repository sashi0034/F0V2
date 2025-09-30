#pragma once
#include "Mat4x4.h"

namespace TY
{
    class VertexBufferCore;
    class IndexBuffer;

    namespace Graphics3D
    {
        void SetViewMatrix(const Mat4x4& viewMatrix);

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);

        Mat4x4 WorldToProjection();

        Mat4x4 WorldToScreen();

        void DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer);

        void DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount);

        void DrawLines(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer);

        void DrawLines(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount);
    }
}
