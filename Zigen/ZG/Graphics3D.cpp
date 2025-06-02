#include "pch.h"
#include "Graphics3D.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "detail/EngineCore.h"
#include "detail/EngineStackState.h"

namespace ZG
{
    using namespace detail;

    void Graphics3D::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        EngineStackState.SetViewMatrix(viewMatrix);
    }

    void Graphics3D::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        EngineStackState.SetProjectionMatrix(projectionMatrix);
    }

    void Graphics3D::DrawTriangles(const VertexBuffer_impl& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        vertexBuffer.commandSet();
        indexBuffer.commandSet();

        const auto commandList = EngineCore.GetCommandList();
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(indexBuffer.count(), 1, 0, 0, 0);
    }
}
