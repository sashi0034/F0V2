#pragma once
#include "TextureHandle.h"
#include "UnifiedString.h"

namespace TY
{
    class DiskTexture
    {
    public:
        DiskTexture() = default;

        DiskTexture(const UnifiedString& path);

        [[nodiscard]]
        operator TextureHandle() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
