#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Matrix4x4.h"
#include "Vector2.h"
#include "Vector3.h"
#include <algorithm>

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

	struct Random
	{
		float time;
	};

	void Initialize(DirectXCommon* dxCommon,Camera*camera, std::string textureFilePath);
	void Update();
	void Draw();
	void DebugUpdate();

	void SetEffectType(EffectType type) { currentEffect_ = type; }

	EffectType GetEffectType()const {return currentEffect_;}

	void AddVignetteScale(float delta) { if (vignette.data) vignette.data->scale = (std::max)(0.0f, vignette.data->scale + delta); }
	void AddVignettePower(float delta) { if (vignette.data) vignette.data->power = (std::max)(0.0f, vignette.data->power + delta); }
	float GetVignetteScale() const { return vignette.data ? vignette.data->scale : 0.0f; }
	float GetVignettePower() const { return vignette.data ? vignette.data->power : 0.0f; }

	// --- 2. Gaussian ---
	void AddGaussianKernel(int delta) {
		if (gaussian.data) {
			int current = static_cast<int>(gaussian.data->kernel) + (delta * 2);
			gaussian.data->kernel = static_cast<uint32_t>(std::clamp(current, 3, 9));
		}
	}
	void AddGaussianSigma(float delta) { if (gaussian.data) gaussian.data->sigma = (std::max)(0.1f, gaussian.data->sigma + delta); }
	uint32_t GetGaussianKernel() const { return gaussian.data ? gaussian.data->kernel : 3; }
	float GetGaussianSigma() const { return gaussian.data ? gaussian.data->sigma : 0.1f; }

	// --- 3. LumBasedOutline ---
	void AddLumOutlineEdgeWeight(float delta) { if (lumOutline.data) lumOutline.data->edgeWeight = std::clamp(lumOutline.data->edgeWeight + delta, 0.0f, 10.0f); }
	float GetLumOutlineEdgeWeight() const { return lumOutline.data ? lumOutline.data->edgeWeight : 0.0f; }

	// --- 4. DepthBasedOutline ---
	void AddDepthOutlineEdgeWeight(float delta) { if (depthOutline.data) depthOutline.data->edgeWeight = std::clamp(depthOutline.data->edgeWeight + delta, 0.0f, 10.0f); }
	float GetDepthOutlineEdgeWeight() const { return depthOutline.data ? depthOutline.data->edgeWeight : 0.0f; }

	// --- 5. RadialBlur ---
	void AddRadialBlurWidth(float delta) { if (radialBlur.data) radialBlur.data->blurWidth = std::clamp(radialBlur.data->blurWidth + delta, 0.0f, 10.0f); }
	void AddRadialBlurCenter(float dx, float dy) {
		if (radialBlur.data) {
			radialBlur.data->center.x = std::clamp(radialBlur.data->center.x + dx, 0.0f, 1.0f);
			radialBlur.data->center.y = std::clamp(radialBlur.data->center.y + dy, 0.0f, 1.0f);
		}
	}
	float GetRadialBlurWidth() const { return radialBlur.data ? radialBlur.data->blurWidth : 0.0f; }
	Vector2 GetRadialBlurCenter() const { return radialBlur.data ? radialBlur.data->center : Vector2{ 0.5f, 0.5f }; }

	// --- 6. Dissolve ---
	void AddDissolveThreshold(float delta) { if (dissolve.data) dissolve.data->threshold = std::clamp(dissolve.data->threshold + delta, 0.0f, 1.0f); }
	void AddDissolveEdgeRange(float delta) { if (dissolve.data) dissolve.data->edgeRange = std::clamp(dissolve.data->edgeRange + delta, 0.0f, 0.3f); }
	float GetDissolveThreshold() const { return dissolve.data ? dissolve.data->threshold : 0.0f; }
	float GetDissolveEdgeRange() const { return dissolve.data ? dissolve.data->edgeRange : 0.0f; }

private:

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	//PSO（ゲームでエフェクトを複数使用するために配列にする）
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(EffectType::Count)];

	//選択されているエフェクト
	EffectType currentEffect_ = EffectType::kGrayscale;

	ConstantBufferInfo<Vignette> vignette;
	ConstantBufferInfo<GaussianFilter> gaussian;
	ConstantBufferInfo<Outline> lumOutline;
	ConstantBufferInfo<Outline> depthOutline;
	ConstantBufferInfo<Material> material;
	ConstantBufferInfo<RadialBlur> radialBlur;
	ConstantBufferInfo<Dissolve> dissolve;
	ConstantBufferInfo<Random> random;

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