#pragma once
#include "Mat4x4.h"

namespace TY
{
    class VertexBufferImpl;
    class IndexBuffer;

    namespace Graphics3D
    {
        void SetViewMatrix(const Mat4x4& viewMatrix);

        [[nodiscard]]
        Mat4x4 ViewMatrix();

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);

        [[nodiscard]]
        Mat4x4 ProjectionMatrix();

        [[nodiscard]]
        Mat4x4 WorldToProjection();

        [[nodiscard]]
        Mat4x4 WorldToScreen();

        void DrawTriangles(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer);

        void DrawTriangles(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount);

        void DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer);

        void DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount);
    }
}
