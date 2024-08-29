#pragma once
#include"gpu_resource_helper.h"
#include"camera.h"
#include"globel_device_value.h"
#include "../nlohmann_json/json.hpp"
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
    void GetSkyBindPoint(std::unordered_map<size_t, size_t>& bindPoint);
};

class SimpleMaterial
{
    Microsoft::WRL::ComPtr<ID3D12Resource> baseColor;
    Microsoft::WRL::ComPtr<ID3D12Resource> normalColor;
    Microsoft::WRL::ComPtr<ID3D12Resource> metallicColor;
    Microsoft::WRL::ComPtr<ID3D12Resource> roughnessColor;
    size_t baseColorOffset;
public:
    void Create(const std::string &mtlName);
    size_t GetBaseOffset() { return baseColorOffset; };
private:
    void GenerateTexture(
        nlohmann::json& curmaterialJson,
        const std::string& texName,
        Microsoft::WRL::ComPtr<ID3D12Resource>& texTureRes,
        size_t& srvOffset
    );
};

class SimpleStaticMeshRenderer
{
    DirectX::XMFLOAT4X4 transformMatrix;
    std::shared_ptr<SimpleStaticMesh> curMesh;
    SimpleMaterial curMat;
public:
    void CreateOnCmdListOpen(const std::string &meshName, const std::string& materialName);
    void CreateOnCmdListOpen(
        const std::string& meshFile,
        const std::string& skeletonFile,
        const std::vector<std::string>& animationFileList,
        const std::string& materialName
    );
    void Draw(const std::unordered_map<size_t, size_t>& viewBindPoint, ID3D12PipelineState* curPipeline, UINT globelInstanceOffset);
    void Update(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 rotation, DirectX::XMFLOAT4 scale);
    const DirectX::XMFLOAT4X4& GetTransForm() { return transformMatrix; };
    SimpleStaticMesh* GetCurrentMesh() { return curMesh.get(); };
};