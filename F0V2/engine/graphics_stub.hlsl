struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VS(float4 position : POSITION)
{
    PSInput result;
    result.position = position;
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    return float4(1, 1, 0, 1);
}
