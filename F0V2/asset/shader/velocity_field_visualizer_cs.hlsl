Texture2D<float4> g_velocityField : register(t0);

RWTexture2D<float4> g_output : register(u0);

float3 HSVToRGB(float3 hsv)
{
    const float3 p = abs(frac(hsv.xxx + float3(0.0f, 2.0f / 3.0f, 1.0f / 3.0f)) * 6.0f - 3.0f);
    return hsv.z * lerp(1.0f.xxx, saturate(p - 1.0f), hsv.y);
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint outputWidth;
    uint outputHeight;
    g_output.GetDimensions(outputWidth, outputHeight);

    const uint2 pixel = dispatchThreadID.xy;
    if (pixel.x >= outputWidth || pixel.y >= outputHeight)
    {
        return;
    }

    const float2 velocity = g_velocityField.Load(int3(pixel, 0)).rg;
    const float speed = length(velocity);

    // 速度の方向を色相、速度の大きさを明度へ変換する。
    static const float TwoPi = 6.28318530718f;
    const float hue = frac(atan2(velocity.y, velocity.x) / TwoPi + 1.0f);
    const float normalizedSpeed = 1.0f - exp(-speed * 1.5f);

    // 速度 0 は灰色にし、低速域でも方向を確認できるようにする。
    const float saturation = smoothstep(0.0f, 0.05f, speed) * 0.9f;
    const float brightness = lerp(0.25f, 1.0f, normalizedSpeed);
    const float3 velocityColor = HSVToRGB(float3(hue, saturation, brightness));

    g_output[pixel] = float4(velocityColor, 1.0f);
}
