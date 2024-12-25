#pragma once
#include"sky_box.h"
#include"gpu_resource_helper.h"
#define JointNumAligned 64
struct MeshRenderParameter
{
    DirectX::XMFLOAT4X4 mTransformMatrix;
    int32_t mAnimationindex = 0;
    float curPlayTime = 0.0f;
};
struct SkeletonParentIdLayer 
{
    int32_t mBlockCount = 0;
    std::vector<int32_t> mParentIdData;
};
class SkeletalMeshRenderBatch
{
    SimpleStaticMeshRenderer mRenderer;

    SimpleSkeletalMeshData* mSkeletalMeshPointer;

    std::vector<MeshRenderParameter> mAllInstance;

    size_t mInstaceDataOffset = 0;

    size_t mVertexDataOffset = 0;
    //skeleton data offset info
    std::vector<int32_t> mSkeletonParentDataGlobelOffset;

    //animation data offset info
    std::vector<int32_t> mAnimationDataGlobelId;

    std::vector<int32_t> mAnimationDataGlobelOffset;
public:
    void CreateOnCmdListOpen(
        const int32_t meshIndex,
        const std::string& meshFile,
        const std::string& skeletonFile,
        const std::vector<std::string>& animationFileList,
        const std::string& materialName
    );

    void UpdateSkinValue(
        size_t& globelSkinVertNum,
        size_t& globelSkinMatrixNum,
        size_t& globelIndirectArgNum,
        SimpleBufferStaging& SkeletonAnimationResultCpu,
        SimpleBufferStaging& skinIndirectArgBufferCpu,
        float delta_time
    );

    void UpdateInstanceOffset(std::vector<DirectX::XMFLOAT4X4>& worldMatrixArray);

    void Draw(const std::unordered_map<size_t, size_t>& allBindPoint, ID3D12PipelineState* curPipeline);

    void AddInstance(
        DirectX::XMFLOAT4X4 transformMatrix,
        int32_t animationindex,
        float stTime
    );

    size_t ComputeSkinResultBufferCount();

    size_t ComputeSkeletalMatrixBufferCount();

    int32_t ComputeAssetAlignedSkeletalBlockCount() const;

    int32_t ComputeAlignedSkeletalBlockCount() const;

    void CollectAnimationData(
        std::vector<DirectX::XMUINT4>& animationMessagePack,
        std::vector<DirectX::XMFLOAT4> &animationDataPack
    );

    void CollectSkeletonHierarchyData(std::vector<SkeletonParentIdLayer>& skeletonMessagePack);

    void CollectLocalToWorldUniformMessage(int32_t index,std::vector<DirectX::XMUINT4>& skeletonMessagePack, int32_t& globelBlockOffset);

    void UpdateSkinValueGpu(
        size_t& globelSkinVertNum,
        size_t& globelSkinMatrixNum,
        size_t& globelIndirectArgNum,
        SimpleBufferStaging& skinIndirectArgBufferCpu
    );

    void CollectAnimationSimulationData(
        std::vector<DirectX::XMFLOAT4> &animationSimulationUniform,
        int32_t& outputOffset,
        float delta_time
    );
private:

    void LocalPoseToModelPose(const std::vector<SimpleSkeletonJoint>& localPose, std::vector<DirectX::XMMATRIX>& modelPose);
};

struct GpuAnimSimulation
{
    int32_t allGpuSimulationBlock = 0;

    SimpleBufferStaging animationStagingBuffer;

    SimpleReadOnlyBuffer animationClipMessage;       //save all skeletal animation descriptor info data

    SimpleReadOnlyBuffer animationClipInput;         //save all skeletal animation curve clip data

    size_t animationUniformCount = 0;

    SimpleBufferStaging animationUniformCpu[3];

    SimpleReadOnlyBuffer animationUniformGpu;         //save all skeletal animation curve clip data

    SimpleReadWriteBuffer animationResultOutput;     ///save the skeletal animation graph simulation result(local space skeleton joint pose)

    void CreateOnCmdListOpen(std::vector<SkeletalMeshRenderBatch>& meshValueList);

    void OnUpdate(std::vector<SkeletalMeshRenderBatch>& meshValueList,uint32_t currentFrameIndex, float delta_time);

    void OnDispatch(GpuResourceUtil::GlobelPipelineManager& allPepelines);
};

struct GpuSkeletonTreeLocalToWorld 
{
    std::vector<SkeletonParentIdLayer> skeletonMessagePack;

    SimpleBufferStaging prefixUniformCpu[3];
    SimpleReadOnlyBuffer prefixUniformGpu;

    SimpleBufferStaging mergeUniformCpu[3];
    SimpleReadOnlyBuffer mergeUniformGpu;

    SimpleBufferStaging trieStagingBuffer;
    SimpleReadOnlyBuffer jointPrefixMessage;
    SimpleReadOnlyBuffer jointMergeInput;
    //save the world space skeleton joint pose
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap0;
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap1;

    std::vector<int32_t> dispathcCount;

    void CreateOnCmdListOpen(std::vector<SkeletalMeshRenderBatch>& meshValueList);

    void OnUpdate(std::vector<SkeletalMeshRenderBatch>& meshValueList, uint32_t currentFrameIndex, float delta_time);

    void OnDispatch(SimpleReadWriteBuffer animationResultOutput, GpuResourceUtil::GlobelPipelineManager& allPepelines);
};

enum class SimulationType
{
    SimulationCpu = 0,
    SimulationGpu
};
class AnimationSimulateDemo
{
    size_t samplerDescriptor;
    SimpleCamera baseView;
    SimpleSkyBox baseSkyBox;

    SimpleReadOnlyBuffer bindposeMatrixInput;        //save the bindposeMatrix for each mesh
    SimpleReadOnlyBuffer skeletonHierarchyInput;     //save the parent index(1,2,4,8,16,32,64) for each joint of skeleton
    SimpleReadWriteBuffer SkeletonAnimationResultGpu;    //save the skeletal animation graph simulation result(globel space skeleton joint pose)
    SimpleBufferStaging SkeletonAnimationResultCpu[3];

    SimulationType curType;
    GpuAnimSimulation gpuAnimationSimulationPass;
    GpuSkeletonTreeLocalToWorld gpuLocalToWorldPass;

    SimpleReadOnlyBuffer skinIndirectArgBufferGpu;    //save the skeletal animation graph simulation result(local space skeleton joint pose)
    SimpleBufferStaging skinIndirectArgBufferCpu[3];
    //save the world space skeleton joint pose
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap0;
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap1;

    //save the vertex skin result
    SimpleReadWriteBuffer skinVertexResult;

    std::vector<SkeletalMeshRenderBatch> meshValueList;

    //world matrix
    SimpleReadOnlyBuffer worldMatrixBufferGpu;
    SimpleBufferStaging worldMatrixBufferCpu[3];
    //view buffer
    SimpleUniformBuffer viewBufferGpu;
    SimpleBufferStaging viewBufferCpu[3];


    SimpleStaticMeshRenderer floorMeshRenderer;
    bool inited = false;

    GpuResourceUtil::GlobelPipelineManager mAllPipelines;
    //depthstencil
    SimpleDepthStencilBuffer mDepthStencil[3];
    
public:
    SimpleCamera* GetCamera() { return &baseView; }
    void Create();
    void CreateOnCmdListOpen(const std::string& configFile);
    void UpdateSkeletalMeshBatch(std::vector<DirectX::XMFLOAT4X4>& worldMatrixArray, uint32_t& indirectSimulationCount,uint32_t currentFrameIndex, float delta_time);
    bool Inited() { return inited; }
    void DrawDemoData(float delta_time);
private:
    void UpdateResultBufferCpu(uint32_t currentFrameIndex, float delta_time, uint32_t& indirectSimulationCount);
};