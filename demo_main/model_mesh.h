#pragma once
#include "skeletal_animation.h"
extern SimpleBufferStaging g_gpuStageBuffer;
extern SimpleBuffer g_gpuSceneStaticVertexBuffer;
extern SimpleBuffer g_gpuSceneSkinVertexBuffer;
extern SimpleBuffer g_gpuSceneIndexBuffer;
extern size_t stageOffset;
extern size_t staticVbOffset;
extern size_t ibOffset;
extern size_t skinVbOffset;
void InitGpuSceneMeshBuffer();
struct BaseVertex
{
    DirectX::XMFLOAT3 pos;
    float empty1;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT3 normal;
    float empty2;
    DirectX::XMFLOAT4 tangent;
    DirectX::XMFLOAT2 uv[4];
};
struct SkinVertex
{
    uint32_t mRefBone[4];
    uint32_t mWeight[4];
};

struct SimpleSubMesh
{
    std::vector<BaseVertex>  mVertexData;
    std::vector<SkinVertex>  mSkinData;
    std::vector<uint32_t>    mIndexData;
    std::vector<std::string> mRefBoneName;
    std::vector<DirectX::XMFLOAT4X4> mRefBonePose;
};

class SimpleStaticMesh
{
public:
    std::vector<SimpleSubMesh> mSubMesh;
    size_t mVbOffset;
    size_t mIbOffset;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;
public:
    void Create(const std::string& meshFile);
private:
    void ReadVertexData(size_t idx, const byte*& ptr);
};

class SimpleModelMeshData
{
public:
    std::vector<SimpleSubMesh> mSubMesh;
    SimpleSkeletonData mSkeleton;
    std::vector<SimpleAnimationData> mAnimationList;
    size_t mVbOffset;
    size_t mIbOffset;
public:
    void Create(
        const std::string& meshFile,
        const std::string& skeletonFile,
        const std::vector<std::string>& animationFileList
    );
private:
    void ReadVertexDataBase(size_t idx, const byte*& ptr);
    void ReadVertexData(size_t idx, const byte*& ptr);
};