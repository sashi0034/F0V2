﻿#include "pch.h"
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

    void Graphics3D::DrawTriangles(const VertexBuffer_impl& vertexBuffer, const IndexBuffer& indexBuffer)
    {
        vertexBuffer.commandSet();
        indexBuffer.commandSet();

        const auto commandList = EngineRenderContext::GetCommandList();
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(indexBuffer.count(), 1, 0, 0, 0);
    }
}
