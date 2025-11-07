#include "pch.h"
#include "Graphics3D.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "detail/EngineCore.h"
#include "detail/RenderContext_singleton.h"
#include "detail/EngineStateContext.h"
#include "detail/SceneState3D_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    void drawInternal(
        const VertexBufferImpl& vertexBuffer,
        const IndexBuffer& indexBuffer,
        int indexCount,
        D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();

        if (not vertexBuffer.isEmpty())
        {
            vertexBuffer.commandSet();
            indexBuffer.commandSet();

            commandList->IASetPrimitiveTopology(topology);
            commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }
        else
        {
            commandList->IASetPrimitiveTopology(topology);
            commandList->DrawInstanced(indexCount, 1, 0, 0);
        }
    }
}

namespace TY
{
    using namespace detail;

    void Graphics3D::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        SceneState3D_singleton::SetViewMatrix(viewMatrix);
    }

    Mat4x4 Graphics3D::ViewMatrix()
    {
        return SceneState3D_singleton::GetViewMatrix();
    }

    void Graphics3D::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        SceneState3D_singleton::SetProjectionMatrix(projectionMatrix);
    }

    Mat4x4 Graphics3D::ProjectionMatrix()
    {
        return SceneState3D_singleton::GetProjectionMatrix();
    }

    Mat4x4 Graphics3D::WorldToProjection()
    {
        return SceneState3D_singleton::WorldToProjection();
    }

    Mat4x4 Graphics3D::WorldToScreen()
    {
        return SceneState3D_singleton::WorldToScreen();
    }

    void Graphics3D::DrawTriangles(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        DrawTriangles(vertexBuffer, indexBuffer, indexBuffer.count());
    }

    void Graphics3D::DrawTriangles(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Graphics3D::DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        DrawLines(vertexBuffer, indexBuffer, indexBuffer.count());
    }

    void Graphics3D::DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }
}
