#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Matrix4x4.h"

class DirectXCommon;
class SrvManager;
class RtvManager;
class Camera;

class PostEffect
{
public:

	enum EffectType {
		kGrayscale,
		kVignetting,
		kSmoothing,
		kGaussian,
		kLumBasedOutline,
		kDepthBasedOutline,

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
		float padding[2];
	};

	struct Outline
	{
		float edgeWeight;
		float padding[3];
	};

	struct Material
	{
		Matrix4x4 projectionInverse;
	};

	void Initialize(DirectXCommon* dxCommon,std::string textureFilePath,Camera*camera);
	void Draw();
	void DebugUpdate();

private:

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	//PSO（ゲームでエフェクトを複数使用するために配列にする）
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(EffectType::Count)];

	//選択されているエフェクト
	EffectType currentEffect_ = EffectType::kVignetting;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;
	// バッファリソース内のデータを指すポインタ
	Vignette* vignetteData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResource;
	// バッファリソース内のデータを指すポインタ
	GaussianFilter* gaussianData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> lumOutlineResource;
	// バッファリソース内のデータを指すポインタ
	Outline* lumOutlineData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> depthOutlineResource;
	// バッファリソース内のデータを指すポインタ
	Outline* depthOutlineData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	// バッファリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	RtvManager* rtvManager_ = nullptr;
	
	Camera* camera_ = nullptr;


	//ルートシグネチャの作成
	void CreateRootSignature();
	//グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

};