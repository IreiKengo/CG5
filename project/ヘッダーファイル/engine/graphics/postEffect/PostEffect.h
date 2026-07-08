#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>

class DirectXCommon;
class SrvManager;
class RtvManager;

class PostEffect
{
public:

	enum EffectType {
		Grayscale,
		Vignetting,
		Smoothing,
		Gaussian,

		Count//エフェクトの総数
	};

	struct Vignette
	{
		float scale;
		float power;
		float Padding[62];
	};

	struct GaussianFilter
	{
		uint32_t kernel;
		float sigma;
	};


	void Initialize(DirectXCommon* dxCommon,std::string textureFilePath);
	void Draw();
	void DebugUpdate();

private:

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	//PSO（ゲームでエフェクトを複数使用するために配列にする）
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(EffectType::Count)];

	//選択されているエフェクト
	EffectType currentEffect_ = EffectType::Vignetting;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;

	// バッファリソース内のデータを指すポインタ
	Vignette* vignetteData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResource;

	// バッファリソース内のデータを指すポインタ
	GaussianFilter* gaussianData = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	RtvManager* rtvManager_ = nullptr;
	
	//ルートシグネチャの作成
	void CreateRootSignature();
	//グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

};