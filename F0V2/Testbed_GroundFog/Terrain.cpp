#include "pch.h"
#include "Terrain.h"

#include "TY/Color.h"
#include "TY/ComputeDispatcher.h"
#include "TY/DynamicBinding.h"
#include "TY/DynamicTexture.h"
#include "TY/GraphicsOptions.h"
#include "TY/Image.h"
#include "TY/ModelData.h"
#include "TY/ModelDrawer.h"
#include "TY/RenderTargetTexture.h"
#include "TY/Shader.h"

using namespace TY;

namespace
{
    constexpr float HeightScale = 12.0f;
    constexpr float NoiseFrequency = 0.018f;

    struct alignas(16) VelocityFieldParams_b0
    {
        Float2 textureResolution{};
        float elapsedTime{};
        float terrainSize{};
    };

    uint32_t HashLattice(int x, int y)
    {
        uint32_t hash = static_cast<uint32_t>(x) * 0x8da6b343u;
        hash ^= static_cast<uint32_t>(y) * 0xd8163841u;
        hash ^= hash >> 13;
        hash *= 0x85ebca6bu;
        return hash ^ (hash >> 16);
    }

    float GradientDot(int latticeX, int latticeY, float x, float y)
    {
        constexpr std::array<Float2, 8> gradients{
            Float2{1.0f, 0.0f},
            Float2{-1.0f, 0.0f},
            Float2{0.0f, 1.0f},
            Float2{0.0f, -1.0f},
            Float2{0.70710678f, 0.70710678f},
            Float2{-0.70710678f, 0.70710678f},
            Float2{0.70710678f, -0.70710678f},
            Float2{-0.70710678f, -0.70710678f},
        };

        const Float2 gradient = gradients[HashLattice(latticeX, latticeY) % gradients.size()];
        return gradient.x * x + gradient.y * y;
    }

    float SimplexNoise2D(float x, float y)
    {
        constexpr float F2 = 0.3660254037844386f;
        constexpr float G2 = 0.2113248654051871f;

        const float skew = (x + y) * F2;
        const int i = static_cast<int>(std::floor(x + skew));
        const int j = static_cast<int>(std::floor(y + skew));

        const float unskew = static_cast<float>(i + j) * G2;
        const float x0 = x - (static_cast<float>(i) - unskew);
        const float y0 = y - (static_cast<float>(j) - unskew);

        const int i1 = x0 > y0 ? 1 : 0;
        const int j1 = x0 > y0 ? 0 : 1;

        const float x1 = x0 - static_cast<float>(i1) + G2;
        const float y1 = y0 - static_cast<float>(j1) + G2;
        const float x2 = x0 - 1.0f + 2.0f * G2;
        const float y2 = y0 - 1.0f + 2.0f * G2;

        const auto corner = [](int latticeX, int latticeY, float cornerX, float cornerY)
        {
            float contribution = 0.5f - cornerX * cornerX - cornerY * cornerY;
            if (contribution <= 0.0f)
            {
                return 0.0f;
            }

            contribution *= contribution;
            return contribution * contribution * GradientDot(latticeX, latticeY, cornerX, cornerY);
        };

        return 70.0f * (
            corner(i, j, x0, y0)
            + corner(i + i1, j + j1, x1, y1)
            + corner(i + 1, j + 1, x2, y2));
    }

    float TerrainHeight(float x, float z)
    {
        float amplitude = 1.0f;
        float frequency = NoiseFrequency;
        float height = 0.0f;
        float amplitudeSum = 0.0f;

        // 4 octave fBm.
        for (int octave = 0; octave < 4; ++octave)
        {
            height += SimplexNoise2D(x * frequency, z * frequency) * amplitude;
            amplitudeSum += amplitude;
            frequency *= 2.0f;
            amplitude *= 0.5f;
        }

        return height / amplitudeSum * HeightScale;
    }

    float NormalizedFBm(float x, float z, float baseFrequency, const Float2& offset)
    {
        float amplitude = 1.0f;
        float frequency = baseFrequency;
        float value = 0.0f;
        float amplitudeSum = 0.0f;

        for (int octave = 0; octave < 4; ++octave)
        {
            value += SimplexNoise2D(
                (x + offset.x) * frequency,
                (z + offset.y) * frequency) * amplitude;
            amplitudeSum += amplitude;
            frequency *= 2.0f;
            amplitude *= 0.5f;
        }

        return std::clamp(value / amplitudeSum * 0.5f + 0.5f, 0.0f, 1.0f);
    }

    ColorF32 TerrainColor(float height)
    {
        constexpr ColorF32 lowland{0.12f, 0.24f, 0.08f};
        constexpr ColorF32 grass{0.28f, 0.42f, 0.16f};
        constexpr ColorF32 rock{0.38f, 0.35f, 0.31f};
        constexpr ColorF32 snow{0.91f, 0.93f, 0.92f};

        if (height < -2.0f)
        {
            return lowland.lerp(grass, std::clamp((height + 7.0f) / 5.0f, 0.0f, 1.0f));
        }
        if (height < 5.0f)
        {
            return grass.lerp(rock, std::clamp((height + 2.0f) / 7.0f, 0.0f, 1.0f));
        }

        return rock.lerp(snow, std::clamp((height - 5.0f) / 5.0f, 0.0f, 1.0f));
    }

    std::vector<float> MakeHeightMapData()
    {
        std::vector<float> heightMap(Terrain::HeightMapSize * Terrain::HeightMapSize);
        constexpr float halfSize = Terrain::TerrainSize * 0.5f;

        for (int y = 0; y < Terrain::HeightMapSize; ++y)
        {
            const float z = -halfSize
                + Terrain::TerrainSize * static_cast<float>(y) / (Terrain::HeightMapSize - 1);
            for (int x = 0; x < Terrain::HeightMapSize; ++x)
            {
                const float worldX = -halfSize
                    + Terrain::TerrainSize * static_cast<float>(x) / (Terrain::HeightMapSize - 1);
                heightMap[y * Terrain::HeightMapSize + x] = TerrainHeight(worldX, z);
            }
        }

        return heightMap;
    }

    std::vector<Float2> MakeColumnTextureData()
    {
        std::vector<Float2> columnMap(Terrain::HeightMapSize * Terrain::HeightMapSize);
        constexpr float halfSize = Terrain::TerrainSize * 0.5f;

        for (int y = 0; y < Terrain::HeightMapSize; ++y)
        {
            const float z = -halfSize
                + Terrain::TerrainSize * static_cast<float>(y) / (Terrain::HeightMapSize - 1);
            for (int x = 0; x < Terrain::HeightMapSize; ++x)
            {
                const float worldX = -halfSize
                    + Terrain::TerrainSize * static_cast<float>(x) / (Terrain::HeightMapSize - 1);

                const float v = NormalizedFBm(worldX, z, 0.012f, Float2{31.0f, -17.0f});
                const float n = NormalizedFBm(worldX, z, 0.035f, Float2{-43.0f, 29.0f});
                const float height = 1.4f * v * n;
                const float weight = height * 0.8f;

                columnMap[y * Terrain::HeightMapSize + x] = Float2{height, weight};
            }
        }

        return columnMap;
    }

    Image MakeTerrainTexture(const std::vector<float>& heightMap)
    {
        Image image{Size{Terrain::HeightMapSize, Terrain::HeightMapSize}};

        for (int y = 0; y < Terrain::HeightMapSize; ++y)
        {
            for (int x = 0; x < Terrain::HeightMapSize; ++x)
            {
                const float height = heightMap[y * Terrain::HeightMapSize + x];
                image[Point{x, y}] = TerrainColor(height).sRGBToLinear().toColorU8();
            }
        }

        return image;
    }

    Image MakeHeightMapDebugTexture(const std::vector<float>& heightMap)
    {
        Image image{Size{Terrain::HeightMapSize, Terrain::HeightMapSize}};

        for (int y = 0; y < Terrain::HeightMapSize; ++y)
        {
            for (int x = 0; x < Terrain::HeightMapSize; ++x)
            {
                const float height = heightMap[y * Terrain::HeightMapSize + x];
                const float normalizedHeight = std::clamp(
                    (height + HeightScale) / (HeightScale * 2.0f),
                    0.0f,
                    1.0f);
                image[Point{x, y}] = ColorF32{normalizedHeight}.toColorU8();
            }
        }

        return image;
    }

    ModelData MakeTerrainModel(const TextureHandle& texture)
    {
        ModelData model{};

        ModelMaterialParameters materialParameters{};
        materialParameters.albedo = Float3{1.0f};
        model.materials.push_back({"Terrain", materialParameters, texture});

        ModelShape shape{};
        shape.materialIndex = 0;

        constexpr int vertexCount = Terrain::VertexCountPerSide;
        constexpr float halfSize = Terrain::TerrainSize * 0.5f;
        constexpr float vertexSpacing = Terrain::TerrainSize / static_cast<float>(vertexCount - 1);

        shape.vertexBuffer.reserve(vertexCount * vertexCount);
        shape.indexBuffer.reserve((vertexCount - 1) * (vertexCount - 1) * 6);

        for (int zIndex = 0; zIndex < vertexCount; ++zIndex)
        {
            const float z = -halfSize + vertexSpacing * static_cast<float>(zIndex);
            for (int xIndex = 0; xIndex < vertexCount; ++xIndex)
            {
                const float x = -halfSize + vertexSpacing * static_cast<float>(xIndex);
                const float height = TerrainHeight(x, z);

                const float left = TerrainHeight(x - vertexSpacing, z);
                const float right = TerrainHeight(x + vertexSpacing, z);
                const float back = TerrainHeight(x, z - vertexSpacing);
                const float front = TerrainHeight(x, z + vertexSpacing);
                const Float3 normal = Float3{
                    left - right,
                    vertexSpacing * 2.0f,
                    back - front,
                }.normalized();

                const Float2 uv{
                    static_cast<float>(xIndex) / static_cast<float>(vertexCount - 1),
                    static_cast<float>(zIndex) / static_cast<float>(vertexCount - 1),
                };
                shape.vertexBuffer.push_back({Float3{x, height, z}, normal, uv});
            }
        }

        for (int z = 0; z < vertexCount - 1; ++z)
        {
            for (int x = 0; x < vertexCount - 1; ++x)
            {
                const uint16_t i0 = static_cast<uint16_t>(z * vertexCount + x);
                const uint16_t i1 = static_cast<uint16_t>(i0 + 1);
                const uint16_t i2 = static_cast<uint16_t>(i0 + vertexCount);
                const uint16_t i3 = static_cast<uint16_t>(i2 + 1);

                shape.indexBuffer.push_back(i0);
                shape.indexBuffer.push_back(i2);
                shape.indexBuffer.push_back(i1);

                shape.indexBuffer.push_back(i1);
                shape.indexBuffer.push_back(i2);
                shape.indexBuffer.push_back(i3);
            }
        }

        model.shapes.push_back(std::move(shape));
        return model;
    }
}

struct Terrain::Impl
{
    std::vector<float> heightMapData{};
    std::vector<Float2> columnTextureData{};
    DynamicTexture heightMapTexture{};
    DynamicTexture heightMapDebugTexture{};
    DynamicTexture columnTexture{};
    UnorderedRenderTargetTexture velocityFieldTexture{};
    ComputeDispatcher velocityFieldDispatcher{};
    DynamicTexture texture{};
    GraphicsShader shader{GraphicsShader::VS_PS("asset/shader/ground_fog_gbuffer.hlsl")};
    ModelDrawer drawer{};

    Impl()
    {
        heightMapData = MakeHeightMapData();
        heightMapTexture = DynamicTexture{
            ImageView{
                reinterpret_cast<const uint8_t*>(heightMapData.data()),
                Size{HeightMapSize, HeightMapSize},
                heightMapData.size() * sizeof(float),
                DXGI_FORMAT_R32_FLOAT,
            }
        };

        columnTextureData = MakeColumnTextureData();
        columnTexture = DynamicTexture{
            ImageView{
                reinterpret_cast<const uint8_t*>(columnTextureData.data()),
                Size{HeightMapSize, HeightMapSize},
                columnTextureData.size() * sizeof(Float2),
                DXGI_FORMAT_R32G32_FLOAT,
            }
        };

        velocityFieldTexture = UnorderedRenderTargetTexture{
            RenderTargetTextureParams{}
            .setSize(Size{HeightMapSize, HeightMapSize})
            .setClearColor(ColorF32{0.0f, 0.0f, 0.0f, 1.0f})
            .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        };
        velocityFieldDispatcher = ComputeDispatcher{
            ComputeDispatcherParams{}
            .setCS(ComputeShader{ShaderParams::CS("asset/shader/velocity_field_cs.hlsl")})
            .setUav({velocityFieldTexture})
            .setDynamicCbvCount(1)
        };

        heightMapDebugTexture = DynamicTexture{MakeHeightMapDebugTexture(heightMapData)};
        texture = DynamicTexture{MakeTerrainTexture(heightMapData)};
        drawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(ModelBuffer{MakeTerrainModel(texture)})
            .setShader(shader)
            .setOptions(GraphicsOptions::Default3D().setRtvFormats({
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_R8G8B8A8_UNORM,
            }))
        };
    }
};

Terrain::Terrain() :
    m_impl(std::make_unique<Impl>())
{
}

Terrain::~Terrain() = default;

float Terrain::GetTerrainHeight(float x, float z) const
{
    return TerrainHeight(x, z);
}

TextureHandle Terrain::GetHeightMapTexture() const
{
    return m_impl->heightMapTexture;
}

TextureHandle Terrain::GetHeightMapDebugTexture() const
{
    return m_impl->heightMapDebugTexture;
}

TextureHandle Terrain::GetColumnTexture() const
{
    return m_impl->columnTexture;
}

TextureHandle Terrain::GetVelocityFieldTexture() const
{
    return static_cast<const RenderTargetTexture&>(m_impl->velocityFieldTexture);
}

void Terrain::UpdateVelocityField(float elapsedTime) const
{
    const VelocityFieldParams_b0 params{
        .textureResolution = Float2{HeightMapSize, HeightMapSize},
        .elapsedTime = elapsedTime,
        .terrainSize = TerrainSize,
    };
    DynamicBinding::SetDynamicCbv(0, params);
    m_impl->velocityFieldDispatcher.dispatch(
        (HeightMapSize + 7) / 8,
        (HeightMapSize + 7) / 8);
}

void Terrain::Draw() const
{
    m_impl->drawer.draw();
}
