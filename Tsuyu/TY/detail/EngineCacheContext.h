#pragma once

namespace TY
{
    namespace detail
    {
        struct IGpgpuBuffer;
    }

    class StructuredBufferUploader;

    class StructuredBufferTransfer;
}

namespace TY::detail
{
    namespace EngineCacheContext
    {
        void Update();

        void Shutdown();

        StructuredBufferUploader FetchStructuredBufferUploader(const std::shared_ptr<IGpgpuBuffer>& key);

        StructuredBufferTransfer FetchStructuredBufferTransfer(const std::shared_ptr<IGpgpuBuffer>& key);
    }
}
