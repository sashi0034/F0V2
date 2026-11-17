#pragma once

#include <memory>

#include "TY/TextureHandle.h"
#include "TY/Vector3D.h"

class Terrain
{
public:
    static constexpr float TerrainSize = 100.0f;
    static constexpr int VertexCountPerSide = 128;
    static constexpr int HeightMapSize = 512;

    Terrain();
    ~Terrain();

    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    [[nodiscard]] float GetTerrainHeight(float x, float z) const;

    // World XZ [-TerrainSize / 2, TerrainSize / 2] maps to UV [0, 1].
    [[nodiscard]] TextureHandle GetHeightMapTexture() const;

    [[nodiscard]] TextureHandle GetHeightMapDebugTexture() const;

    [[nodiscard]] TextureHandle GetColumnTexture() const;

    [[nodiscard]] TextureHandle GetVelocityFieldTexture() const;

    void UpdateVelocityField(float elapsedTime) const;

    void Draw() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
