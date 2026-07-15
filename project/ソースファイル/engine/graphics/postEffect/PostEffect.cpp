#include "PostEffect.h"
#include "Logger.h"
#include "StringUtility.h"
#include <iostream>
#include <assert.h>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <TextureManager.h>
#include "ImguiManager.h"
#include "Matrix4x4Math.h"
#include "Camera.h"


using namespace StringUtility;
using namespace Logger;
using namespace math;

void PostEffect::Initialize(DirectXCommon* dxCommon, std::string textureFilePath, Camera* camera)
{
	dxCommon_ = dxCommon;
	camera_ = camera;

	srvManager_ = dxCommon_->GetSrvManager();
	rtvManager_ = dxCommon_->GetRtvManager();

	vignetteResource = dxCommon_->CreateBufferResource(sizeof(Vignette));

	vignetteResource->Map(0, nullptr, reinterpret_cast<void**>(&vignetteData));

	vignetteData->scale = 16.0f;
	vignetteData->power = 0.8f;

	gaussianResource = dxCommon_->CreateBufferResource(sizeof(GaussianFilter));
	gaussianResource->Map(0, nullptr, reinterpret_cast<void**>(&gaussianData));

	gaussianData->kernel = 7;
	gaussianData->sigma = 2.0f;

	lumOutlineResource = dxCommon_->CreateBufferResource(sizeof(Outline));
	lumOutlineResource->Map(0, nullptr, reinterpret_cast<void**>(&lumOutlineData));

	lumOutlineData->edgeWeight = 6.0f;

	depthOutlineResource = dxCommon_->CreateBufferResource(sizeof(Outline));
	depthOutlineResource->Map(0, nullptr, reinterpret_cast<void**>(&depthOutlineData));

	depthOutlineData->edgeWeight = 6.0f;

	materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	materialData->projectionInverse = Inverse(camera_->GetProjectionMatrix());

	CreateGraphicsPipeline();



}

void PostEffect::Draw()
{
	D3D12_RESOURCE_BARRIER barriers[2] = {};
	UINT barrierCount = 0;

	//カラー
	barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers[barrierCount].Transition.pResource = dxCommon_->GetRenderTextureResource();
	barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierCount++;

	//深度バッファ
	if (currentEffect_ == EffectType::kDepthBasedOutline)
	{
		barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barriers[barrierCount].Transition.pResource = dxCommon_->GetDepthResource();
		barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrierCount++;
	}

	dxCommon_->GetCommandList()->ResourceBarrier(barrierCount, barriers);

	UINT backBufferIndex = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE swapChainHandle = rtvManager_->GetSwapChainHandle(backBufferIndex);

	D3D12_RESOURCE_BARRIER swapChainBarrier{};
	swapChainBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	swapChainBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// ※GetSwapChainResourceについては下の補足を読んでね
	swapChainBarrier.Transition.pResource = dxCommon_->GetSwapChainResource(backBufferIndex);
	swapChainBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	swapChainBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	swapChainBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	dxCommon_->GetCommandList()->ResourceBarrier(1, &swapChainBarrier);



	// 画面に対して描画を設定（ポストエフェクトなので深度バッファは nullptr でOK）
	dxCommon_->GetCommandList()->OMSetRenderTargets(1, &swapChainHandle, FALSE, nullptr);

	// ビューポートとシザー矩形を通常の画面サイズに設定し直す
	D3D12_VIEWPORT viewport = dxCommon_->GetViewport();
	D3D12_RECT scissor = dxCommon_->GetScissorRect();
	dxCommon_->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon_->GetCommandList()->RSSetScissorRects(1, &scissor);

	// デスクリプタヒープのエラーを防ぐためにSRVマネージャーを再バインド
	srvManager_->PreDraw();

	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineStates_[static_cast<size_t>(currentEffect_)].Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	srvManager_->SetGraphicsRootDescriptorTable(0, dxCommon_->GetRenderTextureSrvIndex());

	if (currentEffect_ == EffectType::kDepthBasedOutline)
	{
		srvManager_->SetGraphicsRootDescriptorTable(1, dxCommon_->GetRenderTextureSrvIndex());
	}

	if (currentEffect_ == EffectType::kVignetting)
	{
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, vignetteResource->GetGPUVirtualAddress());
	} else if (currentEffect_ == EffectType::kGaussian)
	{
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, gaussianResource->GetGPUVirtualAddress());
	} else if (currentEffect_ == EffectType::kLumBasedOutline)
	{
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, lumOutlineResource->GetGPUVirtualAddress());
	} else if (currentEffect_ == EffectType::kDepthBasedOutline)
	{
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, depthOutlineResource->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, materialResource->GetGPUVirtualAddress());

	} else
	{
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, vignetteResource->GetGPUVirtualAddress());
	}

	//頂点3つ描画
	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

	barriers[0] = {};
	barriers[1] = {};
	barrierCount = 0;

	//カラーバッファを戻す	
	barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers[barrierCount].Transition.pResource = dxCommon_->GetRenderTextureResource();
	barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierCount++;

	if (currentEffect_ == EffectType::kDepthBasedOutline)
	{
		//深度バッファを戻す
		barriers[barrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[barrierCount].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barriers[barrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barriers[barrierCount].Transition.pResource = dxCommon_->GetDepthResource();
		barriers[barrierCount].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barriers[barrierCount].Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrierCount++;
	}


	dxCommon_->GetCommandList()->ResourceBarrier(barrierCount, barriers);

}

void PostEffect::DebugUpdate()
{

#ifdef USE_IMGUI

	materialData->projectionInverse = Inverse(camera_->GetProjectionMatrix());

	const char* effectNames[] = { "Grayscale","Vignette","Smoothing","Gaussian","LuminanceBasedOutline","DepthBasedOutline" };
	int currentItem = static_cast<int>(currentEffect_);

	ImGui::Begin("PostEffectSettings");
	if (ImGui::Combo("Effect", &currentItem, effectNames, IM_ARRAYSIZE(effectNames))) {
		currentEffect_ = static_cast<EffectType>(currentItem);
	}

	if (currentEffect_ == EffectType::kVignetting)
	{
		ImGui::DragFloat("Scale", &vignetteData->scale, 0.1f);
		ImGui::DragFloat("Power", &vignetteData->power, 0.01f);
	} else if (currentEffect_ == EffectType::kGaussian)
	{
		int kernelSize = static_cast<int>(gaussianData->kernel);
		if (ImGui::SliderInt("kernel", &kernelSize, 3, 7))
		{
			if (kernelSize % 2 == 0) {
				kernelSize += 1;
			}
		}
		ImGui::DragFloat("sigma", &gaussianData->sigma, 0.01f);
		gaussianData->kernel = static_cast<uint32_t>(kernelSize);
	} else if (currentEffect_ == EffectType::kLumBasedOutline)
	{
		ImGui::DragFloat("edgeWeight", &lumOutlineData->edgeWeight, 0.1f, 0.0f, 10.0f);
	} else if (currentEffect_ == EffectType::kDepthBasedOutline)
	{
		ImGui::DragFloat("edgeWeight", &depthOutlineData->edgeWeight, 0.1f, 0.0f, 10.0f);
	}

	ImGui::End();

#endif 

}

void PostEffect::CreateRootSignature()
{

	std::ostream& logStream = std::cerr;

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0から始まる
	descriptorRange[0].NumDescriptors = 1;//数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//offsetを自動計算

	//Degth用のDescriptorRange
	D3D12_DESCRIPTOR_RANGE depthDescriptorRange[1] = {};
	depthDescriptorRange[0].BaseShaderRegister = 1;
	depthDescriptorRange[0].NumDescriptors = 1;//数は1つ
	depthDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	depthDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//offsetを自動計算

	//Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;//レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderを使う

	staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//ポイントフィルタ
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipmapを使う
	staticSamplers[1].ShaderRegister = 1;//レジスタ番号1を使う
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderを使う

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);


	//RootParameter作成。複数設定できるので配列。PixelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//register(t0)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//Pixelを使う
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	//register(t1)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//Pixelを使う
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = depthDescriptorRange;
	//register(b0)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShdaderで使う
	rootParameters[2].Descriptor.ShaderRegister = 0;//レジスタ番号0を使う
	//register(b1)
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShdaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う


	descriptionRootSignature.pParameters = rootParameters;//ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);//配列の長さ

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr< ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成

	hr = dxCommon_->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

}

void PostEffect::CreateGraphicsPipeline()
{

	CreateRootSignature();

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//頂点には何もデータを入力しないので、InputLayoutは利用しない。
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;


	//RasterizerStateの設定	
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面（時計回り）を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//カリング（裏面も表示させる）
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Fullscreen.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);



	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//全画面に対してなにか処理を施したいだけだから、比較も書き込みも必要ないのでDepth自体が不要
	depthStencilDesc.DepthEnable = false;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	//PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();//RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;//InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };//VertexShader

	graphicsPipelineStateDesc.BlendState = blendDesc;//BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;//RasterizerState
	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むかの設定（気にしなくていい）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	HRESULT hr;

	std::wstring psPaths[static_cast<size_t>(EffectType::Count)] = {
		L"resources/shaders/Grayscale.PS.hlsl",
		L"resources/shaders/Vignette.PS.hlsl",
		L"resources/shaders/BoxFilter.PS.hlsl",
		L"resources/shaders/GaussianFilter.PS.hlsl",
		L"resources/shaders/LuminanceBasedOutline.PS.hlsl",
		L"resources/shaders/DepthBasedOutline.PS.hlsl",
	};

	for (size_t i = 0; i < static_cast<size_t>(EffectType::Count); i++)
	{
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(psPaths[i].c_str(),
			L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize() };//PixelShader

		//実際に生成
		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
			IID_PPV_ARGS(&graphicsPipelineStates_[i]));
	}



}
