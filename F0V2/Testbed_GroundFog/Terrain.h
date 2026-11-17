#pragma once

#include <memory>

#include "TY/Vector3D.h"

class Terrain
{
public:
    static constexpr float TerrainSize = 100.0f;
    static constexpr int VertexCountPerSide = 128;

    Terrain();
    ~Terrain();

    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    [[nodiscard]] float GetTerrainHeight(float x, float z) const;

    void Draw(const TY::Float3& eyePosition);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
