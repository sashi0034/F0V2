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
    void drawStaticInternal(
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

    void drawDynamicInternal(
        const DynamicVertexBufferHandle& vertexBuffer,
        const DynamicIndexBufferHandle& indexBuffer,
        D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        if (vertexBuffer.address == 0 || vertexBuffer.sizeInBytes == 0 || vertexBuffer.strideInBytes == 0 ||
            (vertexBuffer.sizeInBytes % vertexBuffer.strideInBytes) != 0 ||
            indexBuffer.address == 0 || indexBuffer.sizeInBytes == 0 ||
            (indexBuffer.sizeInBytes % sizeof(uint16_t)) != 0)
        {
            return;
        }

        const D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
            .BufferLocation = vertexBuffer.address,
            .SizeInBytes = vertexBuffer.sizeInBytes,
            .StrideInBytes = vertexBuffer.strideInBytes,
        };
        const D3D12_INDEX_BUFFER_VIEW indexBufferView{
            .BufferLocation = indexBuffer.address,
            .SizeInBytes = indexBuffer.sizeInBytes,
            .Format = DXGI_FORMAT_R16_UINT,
        };

        const auto commandList = RenderContext_singleton::TargetCommandList();
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);
        commandList->IASetPrimitiveTopology(topology);
        commandList->DrawIndexedInstanced(
            static_cast<UINT>(indexBuffer.sizeInBytes / sizeof(uint16_t)), 1, 0, 0, 0);
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
        drawStaticInternal(vertexBuffer, indexBuffer, indexBuffer.count(), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Graphics3D::DrawTriangles(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawStaticInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Graphics3D::DrawTriangles(
        const DynamicVertexBufferHandle& vertexBuffer, const DynamicIndexBufferHandle& indexBuffer)
    {
        drawDynamicInternal(vertexBuffer, indexBuffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Graphics3D::DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        drawStaticInternal(vertexBuffer, indexBuffer, indexBuffer.count(), D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    void Graphics3D::DrawLines(const VertexBufferImpl& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        drawStaticInternal(vertexBuffer, indexBuffer, indexCount, D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    void Graphics3D::DrawLines(
        const DynamicVertexBufferHandle& vertexBuffer, const DynamicIndexBufferHandle& indexBuffer)
    {
        drawDynamicInternal(vertexBuffer, indexBuffer, D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }
}
