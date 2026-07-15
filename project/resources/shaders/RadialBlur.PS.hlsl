#include "Fullscreen.hlsli"


struct RadialBlur
{
    float32_t2 center; //中心点。ここを基準に放射状にブラーがかかる
    float32_t blurWidth; //ぼかしの幅。
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};


Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<RadialBlur> gRadialBlur : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
  
    const int32_t kNumSamples = 10;//サンプリング数。多いほど滑らかだが重い
  
    //中心から現在のuvに対しての方向を計算。
    //普段方といえば、単位ベクトルだが、ここではあえて正規化せず、遠いほどより遠くをサンプリングする
    float32_t2 direction = input.texcoord - gRadialBlur.center;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);
    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        //現在のuvから先ほど計算した方向にサンプリング点を進めながらサンプリングしていく
        float32_t2 texcoord = input.texcoord + direction * gRadialBlur.blurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSampler, texcoord).rgb;
    }
    //平均化する
    outputColor.rgb *= rcp(kNumSamples);
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
    
}