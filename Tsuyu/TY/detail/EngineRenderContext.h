#pragma once
#include "CommandList.h"
#include "PipelineType.h"
#include "TY/ConstantBufferUploader.h"
#include "TY/Mat3x2.h"
#include "TY/Mat4x4.h"
#include "TY/RenderTarget.h"
#include "TY/Vector2D.h"

namespace TY::detail
{
    struct SceneState3D_b0
    {
        Mat4x4 projectionMatrix;
        Mat4x4 viewMatrix;
    };

    namespace EngineRenderContext
    {
        void Init();

        void NewFrame();

        void Render();

        void Shutdown();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ScopedDefer ScopedCommandTarget(CommandListType type);

        [[nodiscard]]
        CommandListType ActiveCommandTarget();

        [[nodiscard]]
        ID3D12GraphicsCommandList* ActiveCommandList();

        void FlushActiveCommandList();

        void RequestFrameBufferSize(Size frameBufferSize);

        [[nodiscard]]
        Size FrameBufferSize();

        [[nodiscard]]
        Mat3x2 WindowToFrameBuffer();

        [[nodiscard]]
        ConstantBufferUploader<SceneState3D_b0> GetSceneState3D_CB0();
    }
}
