#include "pch.h"
#include "MachineModelBuilder.h"

namespace
{
    struct FaceVertex
    {
        Float3 pos{};
        Float3 normal{};
    };

    void pushGroundFaces(
        Array<ModelVertex>& vertices,
        Array<uint16_t>& indices,
        int& v_offset,
        int& i_offset,
        const FaceVertex& l0,
        const FaceVertex& r0,
        const FaceVertex& l1,
        const FaceVertex& r1)
    {
        vertices[v_offset] = ModelVertex{l1.pos, l1.normal, Float2{}};
        vertices[v_offset + 1] = ModelVertex{r1.pos, r1.normal, Float2{1, 0}};
        vertices[v_offset + 2] = ModelVertex{l0.pos, l0.normal, Float2{0, 1}};
        vertices[v_offset + 3] = ModelVertex{r0.pos, r0.normal, Float2{1, 1}};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 2;
        indices[i_offset + 2] = v_offset + 1;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 2;
        indices[i_offset + 5] = v_offset + 3;

        v_offset += 4;
        i_offset += 6;

        vertices[v_offset] = ModelVertex{l1.pos, -l1.normal, Float2{}};
        vertices[v_offset + 1] = ModelVertex{r1.pos, -r1.normal, Float2{1, 0}};
        vertices[v_offset + 2] = ModelVertex{l0.pos, -l0.normal, Float2{0, 1}};
        vertices[v_offset + 3] = ModelVertex{r0.pos, -r0.normal, Float2{1, 1}};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 2;

        v_offset += 4;
        i_offset += 6;
    }
}

namespace Race
{
    Array<ModelShape> BuildMachineShapes()
    {
        ModelShape shape{};
        int i_offset{};
        int v_offset{};

        constexpr uint16_t sliceCount = 32; // 外周の分割数
        constexpr uint16_t stackCount = 16; // チューブ断面の分割数

        constexpr float radius = 1.0f;
        constexpr float cylinderHeight = 2.0f;

        // TODO: 上下カプセル?
        return {};
    }
}
