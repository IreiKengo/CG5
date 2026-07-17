#include "Fullscreen.hlsli"


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float rand2dTo1d(float32_t2 value, float32_t2 dotDir = float32_t2(12.9898, 78.233))
{
    float32_t2 smallValue = sin(value);
    float32_t random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

struct Material
{
    float32_t time;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial: register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
  //乱数生成。引数にtexcoordを渡している
    //経過時間timeを掛けてSeed値にする
    float32_t random = rand2dTo1d(input.texcoord + gMaterial.time);
    //色にする
    output.color.rgb = textureColor.rgb * random;
    output.color.a = textureColor.a; // アルファ値は元のまま
    
    return output;
    
}