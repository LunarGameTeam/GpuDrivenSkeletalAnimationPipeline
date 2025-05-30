﻿#pragma once
#include"globel_device_value.h"
#include"model_mesh.h"
namespace GpuResourceUtil
{
    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelDrawInputRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuSkinInputRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationSumulationInputRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationLocalToWorldPrefixRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationLocalToWorldPrefix2RootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationLocalToWorldPrefix3RootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationLocalToWorldPrefix4RootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationLocalToWorldMergeRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuAnimationPoseGenRootParam;
    //skin indirect Shader
    extern Microsoft::WRL::ComPtr<ID3D12CommandSignature> skinPassIndirectSignature;

    extern std::shared_ptr<DirectX::ResourceUploadBatch> globelBatch;

    void InitGlobelBatch();

    void FreeGlobelBatch();

    void GenerateTexture2DSRV(
        std::wstring filename,
        Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        size_t& descriptorOffsetOut
    );

    ID3DBlob* CreateShaderByFile(
        std::wstring filename,
        std::string entryPointFunc,
        std::string shaderType
    );

    void DrawMeshData(ID3D12PipelineState* pipeline, std::unordered_map<size_t, size_t> bindPoint, SimpleStaticMesh* mesh, UINT globelVertexOffset, UINT globelInstanceOffset, UINT instanceCount);

    void BindDescriptorToPipeline(size_t rootTableId, size_t descriptorOffset);

    void BindDescriptorToPipelineCS(size_t rootTableId, size_t descriptorOffset);

    void GenerateGraphRootSignature();

    void GenerateGraphicPipelineByShader(
        ID3D12RootSignature* inputParameterLayout,
        const std::wstring& shaderVertexName,
        const std::wstring& shaderPixelName,
        bool ifStatic,
        bool cullFront,
        Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineOut
    );

    void GenerateGpuSkinRootSignature();

    void GenerateGpuAnimationSimulationRootSignature();

    void GenerateGpuAnimationLocalToWorldRootSignature();

    void GenerateGpuAnimationPoseGenSignature();

    void GenerateComputeShaderPipeline(
        ID3D12RootSignature* RootSignature,
        Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineOut,
        const std::wstring& shaderName
    );

    struct GlobelPipelineManager
    {
        //pipeline
        Microsoft::WRL::ComPtr<ID3D12PipelineState> floorDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> meshDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> skinDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuSkinDispatchPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationSimulationPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldPrefixPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldPrefix2Pipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldPrefix3Pipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldPrefix4Pipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldSamuelPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldKiyavashPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationLocalToWorldMergePipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuAnimationPoseGenPipeline;
    };
    struct computePassIndirectCommand
    {
        DirectX::XMUINT4 constInput1;
        DirectX::XMUINT4 constInput2;
        D3D12_DISPATCH_ARGUMENTS drawArguments;
    };
    void GenerateComputeShaderIndirectArgument(
        UINT constantLocalCountParamRootIndex,
        UINT constantGlobelCountParamRootIndex,
        ID3D12RootSignature* rootSignatureIn,
        Microsoft::WRL::ComPtr<ID3D12CommandSignature>& m_commandSignature
    );

};