#pragma once
#include <d3d12.h>
#include "d3dcompiler.h"
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <d3d12sdklayers.h>
#include <wrl/client.h>
#include "../directx_help/d3dx12.h"
#include "../directx_help/DDSTextureLoader.h"
#include "../directx_help/ResourceUploadBatch.h"
#include <fstream>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
size_t SizeAligned2Pow(const size_t& size_in, const size_t& size_aligned_in);

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
extern int const                    NUM_FRAMES_IN_FLIGHT;
extern FrameContext                 g_frameContext[3];
extern UINT                         g_frameIndex;

extern int const                    NUM_BACK_BUFFERS;
extern ID3D12Device* g_pd3dDevice;
extern ID3D12DescriptorHeap* g_pd3dRtvDescHeap;
extern ID3D12DescriptorHeap* g_pd3dSamplerDescHeap;
extern ID3D12DescriptorHeap* g_pd3dDsvDescHeap;
extern ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
extern ID3D12CommandQueue* g_pd3dCommandQueue;
extern ID3D12GraphicsCommandList* g_pd3dCommandList;
extern ID3D12GraphicsCommandList* g_eachFrameCommandList[3];
extern ID3D12Fence* g_fence;
extern HANDLE                       g_fenceEvent;
extern UINT64                       g_fenceLastSignaledValue;
extern IDXGISwapChain3* g_pSwapChain;
extern HANDLE                       g_hSwapChainWaitableObject;
extern ID3D12Resource* g_mainRenderTargetResource[3];
extern D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[3];
extern D3D12_CPU_DESCRIPTOR_HANDLE  g_mainGUIRenderTargetDescriptor[3];
extern uint32_t g_windowWidth;
extern uint32_t g_windowHeight;
extern size_t globelDescriptorOffset;
extern size_t globelDsvDescriptorOffset;
extern size_t globelSamplerDescriptorOffset;
extern size_t globelRtvDescriptorOffset;
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
size_t CreateSamplerByDesc(D3D12_SAMPLER_DESC samplerDesc);
FrameContext* WaitForNextFrameResources();


class SimpleBuffer
{
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> mBufferResource;
    D3D12_RESOURCE_DESC mBufferDesc = {};
public:
    void Create(size_t bufferSize);
protected:
    size_t InitBuffer(
        size_t bufferSize,
        bool enableCpuWrite,
        bool needCBV
    );
};

class SimpleBufferStaging : public SimpleBuffer
{
public:
    char* mapPointer = nullptr;
public:
    void Create(size_t bufferSize);
};

class SimpleUniformBuffer : public SimpleBuffer
{
public:
    size_t mDescriptorOffsetCBV = (size_t)(-1);
public:
    void Create(size_t bufferSize);
};

class SimpleReadOnlyBuffer : public SimpleBuffer
{
public:
    size_t mDescriptorOffsetSRV = (size_t)(-1);
public:
    void Create(size_t bufferSize, size_t elementSize);
protected:
    void InitSRV(size_t bufferAlinedSize, size_t elementSize);
};

class SimpleReadWriteBuffer : public SimpleReadOnlyBuffer
{
public:
    size_t mDescriptorOffsetUAV = (size_t)(-1);
public:
    void Create(size_t bufferSize, size_t elementSize);
private:
    void InitUAV(size_t bufferAlinedSize, size_t elementSize);
};

class SimpleTexture2D
{
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> mTextureResource;
    D3D12_RESOURCE_DESC mTextureDesc = {};
protected:
    void InitTexture(D3D12_RESOURCE_STATES curInitState,DXGI_FORMAT pixelFormat,size_t width, size_t height, bool need_uav, bool need_rtv, bool need_dsv);
};

class SimpleDepthStencilBuffer : public SimpleTexture2D
{
public:
    size_t mDescriptorOffsetDsv = (size_t)(-1);
public:
    void Create(size_t width, size_t height);
private:
    void InitDSV();
};

class SimpleRenderTargetBuffer : public SimpleTexture2D
{
public:
    size_t mDescriptorOffsetRtv = (size_t)(-1);
    DXGI_FORMAT mFormat;
public:
    void Create(DXGI_FORMAT pixelFormat, size_t width, size_t height);
private:
    void InitRTV();
};

extern SimpleDepthStencilBuffer g_mainDepthStencil[3];
bool LoadFileToVectorBinary(const std::string& configFile, std::vector<char>& buffer);