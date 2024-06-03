#include "globel_device_value.h"
// Data
int const                    NUM_FRAMES_IN_FLIGHT = 3;
FrameContext                 g_frameContext[3] = {};
UINT                         g_frameIndex = 0;

int const                    NUM_BACK_BUFFERS = 3;
ID3D12Device* g_pd3dDevice = nullptr;
ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
ID3D12DescriptorHeap* g_pd3dSamplerDescHeap = nullptr;
ID3D12DescriptorHeap* g_pd3dDsvDescHeap = nullptr;
ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
ID3D12GraphicsCommandList* g_eachFrameCommandList[3] = {nullptr};
ID3D12Fence* g_fence = nullptr;
HANDLE                       g_fenceEvent = nullptr;
UINT64                       g_fenceLastSignaledValue = 0;
IDXGISwapChain3* g_pSwapChain = nullptr;
HANDLE                       g_hSwapChainWaitableObject = nullptr;
ID3D12Resource* g_mainRenderTargetResource[3] = {};
D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[3] = {};
D3D12_CPU_DESCRIPTOR_HANDLE  g_mainGUIRenderTargetDescriptor[3] = {};
uint32_t g_windowWidth = 0;
uint32_t g_windowHeight = 0;
size_t globelDescriptorOffset = 64;
size_t globelDsvDescriptorOffset = 0;
size_t globelSamplerDescriptorOffset = 0;
size_t globelRtvDescriptorOffset = 6;
SimpleDepthStencilBuffer g_mainDepthStencil[3] = {};
size_t SizeAligned2Pow(const size_t& size_in, const size_t& size_aligned_in)
{
    return (size_in + (size_aligned_in - 1)) & ~(size_aligned_in - 1);
}
// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#if defined(_DEBUG)
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif
    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#if defined(_DEBUG)
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 4096;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        D3D12_DESCRIPTOR_HEAP_DESC descDsv = {};
        descDsv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        descDsv.NumDescriptors = 4096;
        descDsv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        descDsv.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&descDsv, IID_PPV_ARGS(&g_pd3dDsvDescHeap)) != S_OK)
            return false;

        D3D12_DESCRIPTOR_HEAP_DESC descSampler = {};
        descSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        descSampler.NumDescriptors = 2048;
        descSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descSampler.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&descSampler, IID_PPV_ARGS(&g_pd3dSamplerDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainGUIRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
        
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 5000;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;
    
    for (int32_t i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
    {
        if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[i].CommandAllocator, nullptr, IID_PPV_ARGS(&g_eachFrameCommandList[i])) != S_OK ||
            g_eachFrameCommandList[i]->Close() != S_OK)
            return false;
    }

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        //g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        //g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    for (int32_t i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
    {
        if (g_eachFrameCommandList[i]) { g_eachFrameCommandList[i]->Release(); g_eachFrameCommandList[i] = nullptr; }
        
    }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dDsvDescHeap) { g_pd3dDsvDescHeap->Release(); g_pd3dDsvDescHeap = nullptr; }
    if (g_pd3dSamplerDescHeap) { g_pd3dSamplerDescHeap->Release(); g_pd3dSamplerDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    CD3DX12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, g_windowWidth, g_windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
        viewDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &viewDesc, g_mainRenderTargetDescriptor[i]);

        D3D12_RENDER_TARGET_VIEW_DESC viewDescGui = {};
        viewDescGui.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
        viewDescGui.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &viewDescGui, g_mainGUIRenderTargetDescriptor[i]);
        
        g_mainRenderTargetResource[i] = pBackBuffer;

        auto desc = pBackBuffer->GetDesc();
        g_windowWidth = desc.Width;
        g_windowHeight = desc.Height;
        
        g_mainDepthStencil[i].Create(g_windowWidth, g_windowHeight);
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
    }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

size_t CreateSamplerByDesc(D3D12_SAMPLER_DESC samplerDesc)
{
    size_t curOffset = globelSamplerDescriptorOffset;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dSamplerDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE samplerCpuHandle = cpuHandleStart;
    samplerCpuHandle.ptr += per_descriptor_size * curOffset;
    g_pd3dDevice->CreateSampler(&samplerDesc, samplerCpuHandle);
    globelSamplerDescriptorOffset += 1;
    return curOffset;
}
FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    g_pd3dCommandList = g_eachFrameCommandList[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        WaitForMultipleObjects(1, &g_fenceEvent, TRUE, INFINITE);
    }

    

    return frameCtx;
}

size_t SimpleBuffer::InitBuffer(
    size_t bufferSize,
    bool enableCpuWrite,
    bool needUav
)
{
    size_t bufferAlinedSize = SizeAligned2Pow(bufferSize, 65536);
    mBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    mBufferDesc.Alignment = 0;
    mBufferDesc.Width = bufferAlinedSize;
    mBufferDesc.Height = 1;
    mBufferDesc.DepthOrArraySize = 1;
    mBufferDesc.MipLevels = 1;
    mBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    mBufferDesc.SampleDesc.Count = 1;
    mBufferDesc.SampleDesc.Quality = 0;
    mBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    mBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_PROPERTIES curHeapType = {};
    curHeapType.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    curHeapType.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    curHeapType.CreationNodeMask = 1;
    curHeapType.VisibleNodeMask = 1;
    D3D12_RESOURCE_STATES initState;
    if (!enableCpuWrite)
    {
        if (needUav)
        {
            mBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        curHeapType.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
        initState = D3D12_RESOURCE_STATE_COMMON;
    }
    else
    {
        curHeapType.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
        initState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    HRESULT succeed = g_pd3dDevice->CreateCommittedResource(&curHeapType, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &mBufferDesc, initState, nullptr, IID_PPV_ARGS(&mBufferResource));
    return bufferAlinedSize;
}

void SimpleBuffer::Create(size_t bufferSize)
{
    InitBuffer(bufferSize, false, false);
}

void SimpleBufferStaging::Create(size_t bufferSize)
{
    size_t bufferAlinedSize = InitBuffer(bufferSize, true, false);
    D3D12_RANGE mapRange;
    mapRange.Begin = 0;
    mapRange.End = bufferAlinedSize;
    mBufferResource->Map(0, &mapRange, (void**)&mapPointer);
}

void SimpleUniformBuffer::Create(size_t bufferSize)
{
    size_t bufferAlinedSize = InitBuffer(bufferSize, false, false);
    mDescriptorOffsetCBV = globelDescriptorOffset;
    //generate cbv
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = mBufferResource->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = bufferAlinedSize;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = cpuHandleStart;
    srvCpuHandle.ptr += per_descriptor_size * mDescriptorOffsetCBV;
    g_pd3dDevice->CreateConstantBufferView(&cbvDesc, srvCpuHandle);
    globelDescriptorOffset += 1;
}

void SimpleReadOnlyBuffer::Create(size_t bufferSize, size_t elementSize)
{
    size_t bufferAlinedSize = InitBuffer(bufferSize, false, false);
    InitSRV(bufferAlinedSize, elementSize);
}

void SimpleReadOnlyBuffer::InitSRV(size_t bufferAlinedSize, size_t elementSize)
{
    mDescriptorOffsetSRV = globelDescriptorOffset;
    //generate srv
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.NumElements = bufferAlinedSize / elementSize;
    srvDesc.Buffer.StructureByteStride = elementSize;
    srvDesc.Buffer.FirstElement = 0;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = cpuHandleStart;
    srvCpuHandle.ptr += per_descriptor_size * mDescriptorOffsetSRV;
    g_pd3dDevice->CreateShaderResourceView(mBufferResource.Get(), &srvDesc, srvCpuHandle);
    globelDescriptorOffset += 1;
}

void SimpleReadWriteBuffer::Create(size_t bufferSize, size_t elementSize)
{
    size_t bufferAlinedSize = InitBuffer(bufferSize, false, true);
    InitSRV(bufferAlinedSize, elementSize);
    InitUAV(bufferAlinedSize, elementSize);
}

void SimpleReadWriteBuffer::InitUAV(size_t bufferAlinedSize, size_t elementSize)
{
    mDescriptorOffsetUAV = globelDescriptorOffset;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_NONE;
    uavDesc.Buffer.NumElements = bufferAlinedSize / elementSize;
    uavDesc.Buffer.StructureByteStride = elementSize;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle = cpuHandleStart;
    uavCpuHandle.ptr += per_descriptor_size * mDescriptorOffsetUAV;
    g_pd3dDevice->CreateUnorderedAccessView(mBufferResource.Get(), nullptr, &uavDesc, uavCpuHandle);
    globelDescriptorOffset += 1;
}

void SimpleTexture2D::InitTexture(D3D12_RESOURCE_STATES curInitState, DXGI_FORMAT pixelFormat, size_t width, size_t height, bool need_uav, bool need_rtv, bool need_dsv)
{
    D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_CLEAR_VALUE newValue;

    D3D12_CLEAR_VALUE* clear_data = nullptr;
    if (need_uav)
    {
        resFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (need_rtv)
    {
        resFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (need_dsv)
    {
        resFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        newValue.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
        newValue.DepthStencil.Depth = 1;
        newValue.DepthStencil.Stencil = 0;
        clear_data = &newValue;
    }
    mTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(pixelFormat, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_HEAP_PROPERTIES curHeapType = {};
    curHeapType.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    curHeapType.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    curHeapType.CreationNodeMask = 1;
    curHeapType.VisibleNodeMask = 1;
    curHeapType.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
    HRESULT succeed = g_pd3dDevice->CreateCommittedResource(&curHeapType, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &mTextureDesc, curInitState, clear_data, IID_PPV_ARGS(&mTextureResource));
}

void SimpleDepthStencilBuffer::Create(size_t width, size_t height)
{
    InitTexture(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, false, false, true);
    InitDSV();
}

void SimpleDepthStencilBuffer::InitDSV()
{
    mDescriptorOffsetDsv = globelDsvDescriptorOffset;
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
    dsvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dDsvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = cpuHandleStart;
    dsvCpuHandle.ptr += per_descriptor_size * mDescriptorOffsetDsv;
    g_pd3dDevice->CreateDepthStencilView(mTextureResource.Get(), &dsvDesc, dsvCpuHandle);
    globelDsvDescriptorOffset += 1;
}

void SimpleRenderTargetBuffer::Create(DXGI_FORMAT pixelFormat, size_t width, size_t height)
{
    mFormat = pixelFormat;
    InitTexture(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, pixelFormat, width, height, false, true, false);
    InitRTV();
}

void SimpleRenderTargetBuffer::InitRTV()
{
    mDescriptorOffsetRtv = globelRtvDescriptorOffset;
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = mFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle = cpuHandleStart;
    rtvCpuHandle.ptr += per_descriptor_size * mDescriptorOffsetRtv;
    g_pd3dDevice->CreateRenderTargetView(mTextureResource.Get(), &rtvDesc, rtvCpuHandle);
    globelRtvDescriptorOffset += 1;
}

bool LoadFileToVectorBinary(const std::string& configFile, std::vector<char>& buffer)
{
    std::ifstream file(configFile, std::ios_base::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(size);
    if (!file.read(buffer.data(), size))
    {
        return false;
    }
    file.close();
    return true;
}