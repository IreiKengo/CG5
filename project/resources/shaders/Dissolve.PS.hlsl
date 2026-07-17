#include "Fullscreen.hlsli"




struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct Dissolve
{
    float32_t threshold;
    float32_t edgeRange;
    float32_t2 padding;
    float32_t3 edgeColor;
    float32_t padding2;
};

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);
ConstantBuffer<Dissolve> gDissolve : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
  
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord);
    //maskの値が0.5（閾値）以下の場合はdiscardして抜く
    if (mask <= gDissolve.threshold)
    {
        discard;
    }
    
    //Edgeっぽさを算出
    float32_t edge = 1.0f - smoothstep(gDissolve.threshold, gDissolve.threshold + gDissolve.edgeRange, mask);
    output.color = gTexture.Sample(gSampler, input.texcoord);
    //Wdgeっぽいほど指定した色を加算
    output.color.rgb += edge * gDissolve.edgeColor;
    return output;
    
}