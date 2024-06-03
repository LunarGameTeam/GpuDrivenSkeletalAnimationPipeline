#pragma once
#include"gpu_resource_helper.h"
namespace GpuResourceUtil
{
    Microsoft::WRL::ComPtr<ID3D12RootSignature> globelDrawInputRootParam = nullptr;
    std::shared_ptr<DirectX::ResourceUploadBatch> globelBatch = nullptr;
    void InitGlobelBatch()
    {
        globelBatch = std::make_shared<DirectX::ResourceUploadBatch>(g_pd3dDevice);
        globelBatch->Begin();
    }

    void FreeGlobelBatch()
    {
        globelBatch->End(g_pd3dCommandQueue);
    }

    ID3DBlob* CreateShaderByFile(
        std::wstring filename,
        std::string entryPointFunc,
        std::string shaderType
    )
    {
        //| D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#else
        UINT compileFlags = 0 | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#endif
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* error_message = nullptr;
        HRESULT hr = D3DCompileFromFile(filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPointFunc.c_str(), shaderType.c_str(), compileFlags, 0, &shaderBlob, &error_message);
        if (FAILED(hr))
        {

            char data[1000];
            if (error_message != NULL)
            {
                memcpy(data, error_message->GetBufferPointer(), error_message->GetBufferSize());
            }
            printf(data);
        }
        return shaderBlob;
    }

    void GenerateGraphRootSignature()
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[6];
        //view matrix
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
        //world matrix
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //skin matrix
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //environment texture
        ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //material texture
        ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 5, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //sampler
        ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);                                            // 2 static samplers.

        CD3DX12_ROOT_PARAMETER1 rootParameters[6];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[5].InitAsDescriptorTable(1, &ranges[5], D3D12_SHADER_VISIBILITY_ALL);
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* signature;
        ID3DBlob* error;
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
        HRESULT hr=  g_pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&globelDrawInputRootParam));
    }

    void GenerateGraphicPipelineByShader(
        ID3D12RootSignature* inputParameterLayout,
        const std::wstring& shaderVertexName,
        const std::wstring& shaderPixelName,
        bool ifStatic,
        bool cullFront,
        Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineOut
    )
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_out = {};
        desc_out.pRootSignature = inputParameterLayout;
        //pipeline state desc
        if(cullFront)
        {
            desc_out.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_FRONT;
        }
        else
        {
            desc_out.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
        }
        desc_out.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
        desc_out.BlendState.AlphaToCoverageEnable = false;
        desc_out.BlendState.IndependentBlendEnable = false;
        D3D12_RENDER_TARGET_BLEND_DESC out_desc = {};
        out_desc.BlendEnable = false;
        out_desc.LogicOpEnable = false;
        out_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc_out.BlendState.RenderTarget[0] = out_desc;
        desc_out.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc_out.DepthStencilState.DepthEnable = true;
        desc_out.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
        desc_out.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
        desc_out.DepthStencilState.StencilEnable = false;
        desc_out.DepthStencilState.StencilReadMask = 0xFF;
        desc_out.DepthStencilState.StencilWriteMask = 0xFF;
        desc_out.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
        desc_out.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
        desc_out.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
        desc_out.SampleMask = 0xFFFFFFFF;
        desc_out.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc_out.NumRenderTargets = 1;
        desc_out.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc_out.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc_out.SampleDesc.Count = 1;
        desc_out.SampleDesc.Quality = 0;

        //shader and inputelement
        ID3DBlob* vertexShader = CreateShaderByFile(shaderVertexName.c_str(), "VSMain", "vs_5_0");
        ID3DBlob* pixelShader = CreateShaderByFile(shaderPixelName.c_str(), "PSMain", "ps_5_0");

        desc_out.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
        desc_out.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_desc_array;
        D3D12_INPUT_ELEMENT_DESC mInputLayoutStatic[] =
        {
            //语义，语义索引，格式，槽，偏移值，数据类型，是否实例化
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_INPUT_ELEMENT_DESC mInputLayoutSkin[] =
        {
            //语义，语义索引，格式，槽，偏移值，数据类型，是否实例化
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDEX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        if (ifStatic)
        {
            desc_out.InputLayout.NumElements = 4;
            desc_out.InputLayout.pInputElementDescs = mInputLayoutStatic;
        }
        else
        {
            desc_out.InputLayout.NumElements = 6;
            desc_out.InputLayout.pInputElementDescs = mInputLayoutSkin;
        }
        HRESULT hr = g_pd3dDevice->CreateGraphicsPipelineState(&desc_out, IID_PPV_ARGS(&pipelineOut));
    }

    void GenerateTexture2DSRV(
        std::wstring filename,
        Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        size_t &descriptorOffsetOut
    )
    {
        DirectX::CreateDDSTextureFromFile(g_pd3dDevice, *globelBatch, filename.c_str(), &texture);
        auto curDesc = texture->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC textureViewDesc = {};
        textureViewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
        textureViewDesc.Format = curDesc.Format;
        textureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        textureViewDesc.Texture2D.MipLevels = curDesc.MipLevels;
        if (curDesc.DepthOrArraySize == 6)
        {
            textureViewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE;
            textureViewDesc.TextureCube.MipLevels = curDesc.MipLevels;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = cpuHandleStart;
        size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srvCpuHandle.ptr += per_descriptor_size * globelDescriptorOffset;
        g_pd3dDevice->CreateShaderResourceView(texture.Get(), &textureViewDesc, srvCpuHandle);
        descriptorOffsetOut = globelDescriptorOffset;
        globelDescriptorOffset += 1;
    }

    void BindDescriptorToPipeline(size_t rootTableId, size_t descriptorOffset)
    {
        if (rootTableId != 5)
        {
            size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
            gpuHandleStart.ptr += per_descriptor_size * descriptorOffset;
            g_pd3dCommandList->SetGraphicsRootDescriptorTable(rootTableId, gpuHandleStart);
        }
        else
        {
            size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart = g_pd3dSamplerDescHeap->GetGPUDescriptorHandleForHeapStart();
            gpuHandleStart.ptr += per_descriptor_size * descriptorOffset;
            g_pd3dCommandList->SetGraphicsRootDescriptorTable(rootTableId, gpuHandleStart);
        }
    }

    void DrawMeshData(ID3D12PipelineState* pipeline, std::unordered_map<size_t, size_t> bindPoint, SimpleStaticMesh* mesh)
    {
        g_pd3dCommandList->SetPipelineState(pipeline);
        for (auto& eachKey : bindPoint)
        {
            BindDescriptorToPipeline(eachKey.first, eachKey.second);
        }
        g_pd3dCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pd3dCommandList->IASetVertexBuffers(0,1, &mesh->vbView);
        g_pd3dCommandList->IASetIndexBuffer(&mesh->ibView);


        g_pd3dCommandList->DrawIndexedInstanced((UINT)mesh->mSubMesh[0].mIndexData.size(), 1, mesh->mIbOffset, mesh->mVbOffset, 0);
        
    };
}