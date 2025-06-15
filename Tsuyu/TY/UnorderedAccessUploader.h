#pragma once

namespace TY
{
    struct UnorderedAccessUploaderParams
    {
        int elementCount;
        int elementStride;
    };

    class UnorderedAccessUploader
    {
    public:
        UnorderedAccessUploader() = default;

        UnorderedAccessUploader(const UnorderedAccessUploaderParams& params);

        void upload(const void* src);

        int elementCount() const;

        int elementStride() const;

        ID3D12Resource* getBuffer() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
