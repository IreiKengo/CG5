#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Matrix4x4.h"
#include "Vector2.h"
#include "Vector3.h"

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
		kRadialBlur,
		kDissolve,
		kRandom,

		Count//エフェクトの総数
	};

	template <typename T>
	struct ConstantBufferInfo {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource; // バッファリソース
		T* data = nullptr;// バッファリソース内のデータを指すポインタ
	};

	struct Vignette
	{
		float scale;
		float power;
		float padding[2];
		
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

	struct RadialBlur
	{
		Vector2 center;
		float blurWidth;
		float padding;
	};

	struct Dissolve
	{
		float threshold;
		float edgeRange;
		float padding[2];
		Vector3 edgeColor;
		float padding2;
	};

	void Initialize(DirectXCommon* dxCommon,Camera*camera, std::string textureFilePath);
	void Draw();
	void DebugUpdate();

	

private:

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	//PSO（ゲームでエフェクトを複数使用するために配列にする）
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(EffectType::Count)];

	//選択されているエフェクト
	EffectType currentEffect_ = EffectType::kVignetting;

	ConstantBufferInfo<Vignette> vignette;
	ConstantBufferInfo<GaussianFilter> gaussian;
	ConstantBufferInfo<Outline> lumOutline;
	ConstantBufferInfo<Outline> depthOutline;
	ConstantBufferInfo<Material> material;
	ConstantBufferInfo<RadialBlur> radialBlur;
	ConstantBufferInfo<Dissolve> dissolve;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	RtvManager* rtvManager_ = nullptr;
	
	Camera* camera_ = nullptr;
	//Dissolve用
	uint32_t maskTextureSrvIndex_ = 0;

	//ルートシグネチャの作成
	void CreateRootSignature();
	//グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

	void SetCBV(UINT rootParameterIndex, ID3D12Resource* resource);
	void ChangeSetCBV();
	void TransitionBarrier(ID3D12Resource* resource,D3D12_RESOURCE_STATES stateBefore,D3D12_RESOURCE_STATES stateAfter);
	void ChangeRenderTargetState(bool writable);


};