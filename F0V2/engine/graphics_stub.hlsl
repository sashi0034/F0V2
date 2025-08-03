struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    result.position.zw = float2(0, 1); // z = 0, w = 1

    const uint n0 = 1 + id / 6;
    const uint n1 = n0 + 1;

    const uint vid = id % 6;

    [flatten]
    if (vid == 0)
    {
        result.position.xy = float2(0.5f / n0, 0.5f / n0); // up[0]
    }
    else if (vid == 1)
    {
        result.position.xy = float2(1.0f / n1, 0.0); // up[1]
    }
    else if (vid == 2)
    {
        result.position.xy = float2(1.0f / n0, 0.0); // up[2]
    }
    else if (vid == 3)
    {
        result.position.xy = float2(0.5f / n0, 0.5f / n0); // down[0]
    }
    else if (vid == 4)
    {
        result.position.xy = float2(0.0, 1.0f / n0); // down[1]
    }
    else if (vid == 5)
    {
        result.position.xy = float2(0.0, 1.0f / n1); // down[2]
    }

    if (n0 % 2 == 1)
    {
        result.color = float4(1, 1, 0, 1); // yellow
    }
    else
    {
        result.color = float4(1, 0, 1, 1); // purple
    }

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    return input.color;
}
