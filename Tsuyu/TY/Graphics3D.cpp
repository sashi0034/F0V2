#include "pch.h"
#include "Graphics3D.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"
#include "detail/EngineStateContext.h"
#include "detail/SceneState_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    void drawInternal(
        const VertexBufferCore& vertexBuffer,
        const IndexBuffer& indexBuffer,
        int indexCount,
        D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        const auto commandList = EngineRenderContext::TargetCommandList();

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
        SceneState_singleton::SetViewMatrix(viewMatrix);
    }

    void Graphics3D::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        SceneState_singleton::SetProjectionMatrix(projectionMatrix);
    }

    Mat4x4 Graphics3D::WorldToProjection()
    {
        return SceneState_singleton::WorldToProjection();
    }

    Mat4x4 Graphics3D::WorldToScreen()
    {
        return SceneState_singleton::WorldToScreen();
    }

    void Graphics3D::DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        DrawTriangles(vertexBuffer, indexBuffer, indexBuffer.count());
    }

    void Graphics3D::DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Graphics3D::DrawLines(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        DrawLines(vertexBuffer, indexBuffer, indexBuffer.count());
    }

    void Graphics3D::DrawLines(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }
}
