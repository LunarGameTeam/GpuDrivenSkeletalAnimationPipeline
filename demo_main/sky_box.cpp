#pragma once
#include"sky_box.h"
#include <string>
#include <codecvt>
#include <locale>
using json = nlohmann::json;
void SimpleSkyBox::CreateOnCmdListOpen()
{
    skyMesh.Create("demo_asset_data/skybox/skyball.lmesh");
}

void SimpleSkyBox::Create()
{
	GpuResourceUtil::GenerateGraphicPipelineByShader(GpuResourceUtil::globelDrawInputRootParam.Get(), L"demo_asset_data/shader/draw/sky_draw.hlsl", L"demo_asset_data/shader/draw/sky_draw.hlsl", true,false, skyPipeline);
    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/Cubemap.dds",
        skySpecular,
        skySpecularDescriptorOffset
    );

    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/IrradianceMap.dds",
        skyDiffuse,
        skyDiffuseDescriptorOffset
    );

    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/brdf.dds",
        brdf,
        brdfDescriptorOffset
    );
}

void SimpleSkyBox::Draw(const std::unordered_map<size_t, size_t>& viewBindPoint)
{
    std::unordered_map<size_t, size_t> skyBindPoint = viewBindPoint;
    GetSkyBindPoint(skyBindPoint);
    GpuResourceUtil::DrawMeshData(skyPipeline.Get(), skyBindPoint, &skyMesh,0,1);
}

void SimpleSkyBox::GetSkyBindPoint(std::unordered_map<size_t, size_t>& bindPoint)
{
    bindPoint.insert({ 3,skySpecularDescriptorOffset });
}

std::wstring string_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

void SimpleMaterial::GenerateTexture(json& curmaterialJson,const std::string &texName, Microsoft::WRL::ComPtr<ID3D12Resource>& texTureRes, size_t &srvOffset)
{
    json pbrColor = curmaterialJson[texName];
    bool hasColor = pbrColor["hasTexture"];
    if (hasColor)
    {
        std::string texName = pbrColor["Parameter"];
        std::wstring wTexName = string_to_wstring(texName);
        GpuResourceUtil::GenerateTexture2DSRV(
            wTexName.c_str(),
            texTureRes,
            srvOffset
        );
    }
}

void SimpleMaterial::Create(const std::string& mtlName)
{
    json curmaterialJson;
    std::ifstream(mtlName.c_str()) >> curmaterialJson;
    GenerateTexture(curmaterialJson, "PbrBaseColor", baseColor, baseColorOffset);
    size_t normalOffset, metallicOffset, roughnessOffset;
    GenerateTexture(curmaterialJson, "PbrNormalColor", normalColor, normalOffset);
    GenerateTexture(curmaterialJson, "PbrMetallicColor", metallicColor, metallicOffset);
    GenerateTexture(curmaterialJson, "PbrRoughnessColor", roughnessColor, roughnessOffset);
}

void SimpleStaticMeshRenderer::CreateOnCmdListOpen(const std::string& meshName, const std::string& materialName)
{
    curMesh = std::make_shared<SimpleStaticMesh>();
    curMesh->Create(meshName);
    curMat.Create(materialName);
}

void  SimpleStaticMeshRenderer::CreateOnCmdListOpen(
    const std::string& meshFile,
    const std::string& skeletonFile,
    const std::vector<std::string>& animationFileList,
    const std::string& materialName
)
{
    std::shared_ptr<SimpleSkeletalMeshData> skelMesh = std::make_shared<SimpleSkeletalMeshData>();
    skelMesh->Create(meshFile, skeletonFile, animationFileList);
    curMesh = skelMesh;
    curMat.Create(materialName);
}

void SimpleStaticMeshRenderer::Draw(
    const std::unordered_map<size_t, size_t>& viewBindPoint,
    ID3D12PipelineState* curPipeline,
    UINT globelInstanceOffset,
    UINT instanceCount
)
{
    std::unordered_map<size_t, size_t> matBindPoint = viewBindPoint;
    matBindPoint.insert({ 4,curMat.GetBaseOffset()});
    GpuResourceUtil::DrawMeshData(curPipeline, matBindPoint, curMesh.get(), globelInstanceOffset, instanceCount);
}