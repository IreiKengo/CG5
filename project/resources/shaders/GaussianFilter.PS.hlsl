#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

static const float32_t PI = 3.14159265f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);

}

struct GaussianFilter
{
    uint32_t kernel; //カーネルサイズ(最大7)
    float32_t sigma; //シグマ値
    
};
ConstantBuffer<GaussianFilter> gGaussianFilter : register(b0);




PixelShaderOutput main(VertexShaderOutput input)
{
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float32_t) width), rcp((float32_t) height));
    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    
    float32_t weight = 0.0f;
    float32_t kernel[7][7];
    
    int32_t radius = (int32_t) (gGaussianFilter.kernel / 2);
    int32_t kernelSize = (int32_t) gGaussianFilter.kernel;
    
    
    for (int32_t x = 0; x < kernelSize; ++x)
    { 
        for (int32_t y = 0; y < kernelSize; y++)
        {
            float32_t offsetX = (float32_t) (x - radius);
            float32_t offsetY = (float32_t) (y - radius);
            
            kernel[x][y] = gauss(offsetX, offsetY, gGaussianFilter.sigma);
            weight += kernel[x][y];
        }
    }
    
    //畳み込み
    for (int32_t x = 0; x < kernelSize; ++x)
    { 
        for (int32_t y = 0; y < kernelSize; y++)
        {
            float32_t offsetX = (float32_t) (x - radius);
            float32_t offsetY = (float32_t) (y - radius);
            
            //現在のtexcoordを算出
            float32_t2 texcoord = input.texcoord + float32_t2(offsetX, offsetY) * uvStepSize;
            //色に1/9掛けて足す
            float32_t3 fechColor = gTexture.Sample(gSampler, texcoord).rgb;
            output.color.rgb += fechColor * kernel[x][y];
        }

    }
    //正規化
    output.color.rgb *= rcp(weight);
    return output;
}