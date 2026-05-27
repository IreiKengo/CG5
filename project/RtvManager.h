#pragma once
#include <d3d12.h>
#include <cstdint>
#include <wrl.h>
#include <array>
#include <cassert>

class DirectXCommon;

//RTVの管理
class RtvManager
{
public:

	void Initialize(DirectXCommon* dxCommon);


	//最大RTV数（最大テクスチャ枚数）
	static const uint32_t kMaxRTVCount;


	uint32_t Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	


	//RTV生成（テクスチャ用）
	void CreateRTVforTexture2D(uint32_t rtvIndex, ID3D12Resource* pResource, DXGI_FORMAT format);
	

	bool CanAllocate();
	
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSwapChainHandle(uint32_t index)const { assert(index < 2); return swapChainHandles_[index]; }
	
	void CreateSwapChain(uint32_t swapChainIndex, ID3D12Resource* resource, DXGI_FORMAT format);

	ID3D12DescriptorHeap*GetDescriptorHeap() const{return descriptorHeap.Get();}



private:

	DirectXCommon* dxCommon_ = nullptr;

	//デスクリプタサイズ
	uint32_t descriptorSize = 0;

	//RTVのデスクリプターヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	//次に使用するRTVインデックス
	uint32_t useIndex = 0;

	//RTVを２つ作るのでディスクリプタを２つ用意 
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> swapChainHandles_{};

};
