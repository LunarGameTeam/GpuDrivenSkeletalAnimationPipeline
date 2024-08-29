#pragma once
#include"globel_device_value.h"
#include"model_mesh.h"
namespace GpuResourceUtil
{
    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelDrawInputRootParam;

    extern Microsoft::WRL::ComPtr<ID3D12RootSignature> globelGpuSkinInputRootParam;

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

    void DrawMeshData(ID3D12PipelineState* pipeline, std::unordered_map<size_t, size_t> bindPoint, SimpleStaticMesh* mesh, UINT globelInstanceOffset);

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

    void GenerateGpuSkinPipeline(Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineOut);

    struct GlobelPipelineManager
    {
        //pipeline
        Microsoft::WRL::ComPtr<ID3D12PipelineState> floorDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> meshDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> skinDrawPipeline;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> GpuSkinDispatchPipeline;
    };
    struct computePassIndirectCommand
    {
        DirectX::XMUINT4 constInput;
        D3D12_DISPATCH_ARGUMENTS drawArguments;
    };
    void GenerateComputeShaderIndirectArgument(UINT constantParamRootIndex,ID3D12RootSignature* rootSignatureIn,Microsoft::WRL::ComPtr<ID3D12CommandSignature>& m_commandSignature);

};