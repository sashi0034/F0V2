#include "pch.h"
#include "Graphics3D.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"
#include "detail/EngineStateContext.h"

namespace TY
{
    using namespace detail;

    void Graphics3D::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        EngineStateContext::SetViewMatrix(viewMatrix);
    }

    void Graphics3D::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        EngineStateContext::SetProjectionMatrix(projectionMatrix);
    }

    void Graphics3D::DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        DrawTriangles(vertexBuffer, indexBuffer, indexBuffer.count());
    }

    void Graphics3D::DrawTriangles(const VertexBufferCore& vertexBuffer, const IndexBuffer& indexBuffer, int indexCount)
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);

        if (not vertexBuffer.isEmpty())
        {
            vertexBuffer.commandSet();
            indexBuffer.commandSet();

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }
        else
        {
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->DrawInstanced(indexCount, 1, 0, 0);
        }
    }
}
