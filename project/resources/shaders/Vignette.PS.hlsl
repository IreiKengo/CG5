#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct Vignette
{
    float32_t scale;
    float32_t power;
};
ConstantBuffer<Vignette> gVignette : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    //周囲を0に、中心になるほど明るくなるように計算で調整
    float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    //Scaleで中心付近の明るさを調整
    float vignette = correct.x * correct.y * gVignette.scale;
    //Powerでビネットのカーブを調整
    vignette = saturate(pow(vignette, gVignette.power));
    //係数として乗算
    output.color.rgb *= vignette;
    
    return output;
    
}