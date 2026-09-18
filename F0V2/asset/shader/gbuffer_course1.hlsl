// CourseTextureKind.h でも定義
#define CourseTextureCount 8

// TODO: いずれ Texture2D<float4> g_textures[] (unbounded) にしたい
Texture2D<float4> g_textures[CourseTextureCount] : register(t0);

SamplerState g_sampler0 : register(s0);

cbuffer SceneState : register(b0)
{
    column_major float4x4 g_projectionMatrix;
    column_major float4x4 g_viewMatrix;
}

cbuffer ModelState : register(b1)
{
    column_major float4x4 g_worldMatrix;
}

// NOTE: マテリアルパラメータは不要
// cbuffer ModelMaterial : register(b2)
// {
// }

// CourseFaceType.h でも定義
enum FaceType
{
    FaceType_Default, // テクスチャをそのまま出す面

    FaceType_RoadTop,
    FaceType_RoadBottom,
    FaceType_RoadSide,

    FaceType_PipeEntryExitTop,
    FaceType_PipeEntryExitBottom,
    FaceType_PipeEntryExitSide,
    FaceType_PipeInner,
    FaceType_PipeOuter,
    FaceType_PipeCap,

    FaceType_CylinderEntryExitTop,
    FaceType_CylinderEntryExitBottom,
    FaceType_CylinderEntryExitSide,
    FaceType_CylinderEntryExitCap,
    FaceType_CylinderOuter,

    FaceType_BarrierTop,
    FaceType_BarrierSide,
    FaceType_BarrierBottom,
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float viewDistance : TEXCOORD0; // [near, far]
    float2 uv : TEXCOORD1;
    nointerpolation uint faceType : TEXCOORD2;
    nointerpolation uint textureIndex : TEXCOORD4;
    float metadata : TEXCOORD3; // faceType ごとに意味が異なる // TODO: 事情が変わったので消してもいいかも
};

PSInput VS(
    float4 position : POSITION,
    float4 normal : NORMAL,
    float2 uv : TEXCOORD0,
    uint faceType : TEXCOORD1,
    uint textureIndex : TEXCOORD2,
    float metadata : TEXCOORD3)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);
    result.position = mul(g_viewMatrix, result.position);

    result.viewDistance = length(result.position);

    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normalize(mul((float3x3)g_worldMatrix, normal.xyz));

    result.uv = uv;

    result.faceType = faceType;

    result.textureIndex = textureIndex;

    result.metadata = metadata;

    return result;
}

struct PSOutput
{
    float4 albedoBuffer : SV_TARGET0;
    float4 normalBuffer : SV_TARGET1;
    float viewDistanceBuffer : SV_TARGET2;
};

// -----------------------------------------------

float3 srgbToLinear(float3 c)
{
    return lerp(c / 12.92, pow(max((c + 0.055) / 1.055, 0.0), 2.4), step(0.04045, c));
}

float3 shadeDefaultFace(PSInput input)
{
    return g_textures[NonUniformResourceIndex(input.textureIndex)].Sample(g_sampler0, input.uv).rgb;
}

float3 shadeRoadTop(PSInput input)
{
    static const float forwardRepeat = 100.0;

    // TODO: CourseModelBuilder 側で v の累積値を考慮しないとミラーリピートにならない...
    // TODO: 手計算でミラーさせるのをやめてサンプラー側で対応
    const float forward = input.uv.y / forwardRepeat;
    const float mirroredForward = 1.0 - abs(frac(forward * 0.5) * 2.0 - 1.0);
    const float2 uv = float2(input.uv.x * 0.5 + 0.5, mirroredForward);

    return g_textures[NonUniformResourceIndex(input.textureIndex)].Sample(g_sampler0, uv).rgb;
}

float3 shadeRoadBottom(PSInput input)
{
    static const float forwardRepeat = 100.0;
    const float2 uv = float2(input.uv.x * input.metadata * 0.5, input.uv.y / forwardRepeat);
    return g_textures[NonUniformResourceIndex(input.textureIndex)].Sample(g_sampler0, uv).rgb;
}

float3 shadeRoadSide(PSInput input)
{
    return g_textures[NonUniformResourceIndex(input.textureIndex)].Sample(g_sampler0, input.uv).rgb;
}

float3 shadePipeEntryExitTop(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadePipeEntryExitBottom(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.1, 0.1, 0.1));
}

float3 shadePipeEntryExitSide(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadePipeInner(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadePipeOuter(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.1, 0.1, 0.1));
}

float3 shadePipeCap(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.1, 0.1, 0.1));
}

float3 shadeCylinderEntryExitTop(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadeCylinderEntryExitBottom(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.1, 0.1, 0.1));
}

float3 shadeCylinderEntryExitSide(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadeCylinderEntryExitCap(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.1, 0.1, 0.1));
}

float3 shadeCylinderOuter(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.5, 0.5, 0.5));
}

float3 shadeBarrierTop(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.93, 0.65, 0.96));
}

float3 shadeBarrierSide(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.93, 0.65, 0.96));
}

float3 shadeBarrierBottom(PSInput input)
{
    // TODO
    return srgbToLinear(float3(0.67, 0.52, 0.78));
}

PSOutput PS(PSInput input)
{
    float3 baseColor;

    switch (input.faceType)
    {
    case FaceType_Default:
        baseColor = shadeDefaultFace(input);
        break;
    case FaceType_RoadTop:
        baseColor = shadeRoadTop(input);
        break;
    case FaceType_RoadBottom:
        baseColor = shadeRoadBottom(input);
        break;
    case FaceType_RoadSide:
        baseColor = shadeRoadSide(input);
        break;
    case FaceType_PipeEntryExitTop:
        baseColor = shadePipeEntryExitTop(input);
        break;
    case FaceType_PipeEntryExitBottom:
        baseColor = shadePipeEntryExitBottom(input);
        break;
    case FaceType_PipeEntryExitSide:
        baseColor = shadePipeEntryExitSide(input);
        break;
    case FaceType_PipeInner:
        baseColor = shadePipeInner(input);
        break;
    case FaceType_PipeOuter:
        baseColor = shadePipeOuter(input);
        break;
    case FaceType_PipeCap:
        baseColor = shadePipeCap(input);
        break;
    case FaceType_CylinderEntryExitTop:
        baseColor = shadeCylinderEntryExitTop(input);
        break;
    case FaceType_CylinderEntryExitBottom:
        baseColor = shadeCylinderEntryExitBottom(input);
        break;
    case FaceType_CylinderEntryExitSide:
        baseColor = shadeCylinderEntryExitSide(input);
        break;
    case FaceType_CylinderEntryExitCap:
        baseColor = shadeCylinderEntryExitCap(input);
        break;
    case FaceType_CylinderOuter:
        baseColor = shadeCylinderOuter(input);
        break;
    case FaceType_BarrierTop:
        baseColor = shadeBarrierTop(input);
        break;
    case FaceType_BarrierSide:
        baseColor = shadeBarrierSide(input);
        break;
    case FaceType_BarrierBottom:
        baseColor = shadeBarrierBottom(input);
        break;
    default:
        // 未知の faceType
        baseColor = shadeDefaultFace(input);
        break;
    }

    PSOutput output;

    output.albedoBuffer = float4(baseColor, 1.0);

    output.normalBuffer = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);

    output.viewDistanceBuffer = input.viewDistance;

    return output;
}
