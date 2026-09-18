#pragma once
#include "TY/TextureHandle.h"

namespace TY::detail
{
    /// @brief テクスチャの mip 0 を元に下位ミップを ComputeShader で生成する
    class MipmapGenerator
    {
    public:
        MipmapGenerator() = default;

        MipmapGenerator(const TextureHandle& texture);

        [[nodiscard]]
        bool isEmpty() const;

        void generate() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
