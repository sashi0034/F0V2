#pragma once
#include "CommandList.h"
#include "IEngineDrawer.h"
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
        constexpr int FrameBufferCount = 2;

        void Init();

        void NewFrame();

        void Render();

        void Shutdown();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        /// TODO: obsolete
        [[nodiscard]]
        ScopedDefer ScopedCommandTarget(CommandListType type);

        /// TODO: obsolete
        [[nodiscard]]
        CommandListType ActiveCommandTarget();

        /// TODO: obsolete
        [[nodiscard]]
        ID3D12GraphicsCommandList* ActiveCommandList();

        void FlushComputeCommand();

        void RequestFrameBufferSize(Size frameBufferSize);

        [[nodiscard]]
        Size FrameBufferSize();

        [[nodiscard]]
        Mat3x2 WindowToFrameBuffer();

        [[nodiscard]]
        ConstantBufferUploader<SceneState3D_b0> GetSceneState3D_CB0();

        void MarkDrawerUntilFlush(const std::shared_ptr<IEngineDrawer>& drawer);

        size_t GetFlushTimestamp();
    }
}
