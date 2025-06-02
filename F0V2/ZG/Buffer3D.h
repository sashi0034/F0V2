#pragma once
#include <DirectXMath.h>

#include "Array.h"

namespace ZG
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 uv;
    };

    struct Buffer3DParams
    {
        Array<Vertex> vertexes;
        Array<uint16_t> indices;
    };

    struct Buffer3D_impl;

    // obsolete
    // TODO: remove
    class Buffer3D
    {
    public:
        Buffer3D(const Buffer3DParams& params);

        void Draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
