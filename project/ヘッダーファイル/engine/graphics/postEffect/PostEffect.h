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

	struct Vignette
	{
		float scale;
		float power;
		float Padding[62];
	};

	void Initialize(DirectXCommon* dxCommon,std::string textureFilePath);
	void Draw();
	void DebugUpdate();

private:

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	//PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;

	// バッファリソース内のデータを指すポインタ
	Vignette* vignetteData = nullptr;


	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	RtvManager* rtvManager_ = nullptr;
	
	//ルートシグネチャの作成
	void CreateRootSignature();
	//グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

};