#pragma once
#include"gpu_resource_helper.h"
#include"camera.h"
class SimpleSkyBox
{
    SimpleStaticMesh skyMesh;
    Microsoft::WRL::ComPtr<ID3D12Resource> skySpecular;
    size_t skySpecularDescriptorOffset;
    Microsoft::WRL::ComPtr<ID3D12Resource> skyDiffuse;
    size_t skyDiffuseDescriptorOffset;
    Microsoft::WRL::ComPtr<ID3D12Resource> brdf;
    size_t brdfDescriptorOffset;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> skyPipeline;
public:
    void Create();
    void CreateOnCmdListOpen();
    void Draw(const std::unordered_map<size_t, size_t> &viewBindPoint);
};