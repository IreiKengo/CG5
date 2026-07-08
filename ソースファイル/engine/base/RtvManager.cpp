#include "RtvManager.h"
#include "DirectXCommon.h"



const uint32_t RtvManager::kMaxRTVCount = 512;


void RtvManager::Initialize(DirectXCommon* dxCommon)
{

	//引数で受け取ってメンバ変数に記録する
	dxCommon_ = dxCommon;


	descriptorSize = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//デスクリプタヒープの生成
	descriptorHeap = dxCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kMaxRTVCount, false);



}

uint32_t RtvManager::Allocate()
{

	//上限に達していないかチェックしてassert
	assert(useIndex < kMaxRTVCount);


	//return する番号をいったん記録しておく
	uint32_t index = useIndex;
	//次回のために番号を1進める
	useIndex++;
	//上で記録した番号をreturn
	return  index;



}

D3D12_CPU_DESCRIPTOR_HANDLE RtvManager::GetCPUDescriptorHandle(uint32_t index)
{

	assert(index < kMaxRTVCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
		descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;

}

void RtvManager::CreateSwapChain(
    uint32_t swapChainIndex,
    ID3D12Resource* resource,
    DXGI_FORMAT format)
{
    assert(swapChainIndex < 2);
    assert(resource != nullptr);

    uint32_t rtvIndex = Allocate();

    CreateRTVforTexture2D(
        rtvIndex,
        resource,
        format
    );

    swapChainHandles_[swapChainIndex] =
        GetCPUDescriptorHandle(rtvIndex);
}


void RtvManager::CreateRTVforTexture2D(uint32_t rtvindex, ID3D12Resource* pResource, DXGI_FORMAT format)
{

	D3D12_RENDER_TARGET_VIEW_DESC  rtvDesc{};

	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2dテクスチャとして書き込む


	dxCommon_->GetDevice()->CreateRenderTargetView(pResource, &rtvDesc, GetCPUDescriptorHandle(rtvindex));

}



bool RtvManager::CanAllocate()
{
	//テクスチャ枚数上限チェック
	return useIndex < kMaxRTVCount;
}
