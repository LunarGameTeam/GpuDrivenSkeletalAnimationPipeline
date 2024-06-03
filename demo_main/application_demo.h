#pragma once
#include"sky_box.h"
#include"gpu_resource_helper.h"
class AnimationSimulateDemo
{
    size_t samplerDescriptor;
    SimpleCamera baseView;
    SimpleSkyBox baseSkyBox;

    SimpleReadOnlyBuffer animationClipInput;         //save all skeletal animation curve clip data
    SimpleReadOnlyBuffer bindposeMatrixInput;        //save the bindposeMatrix for each mesh
    SimpleReadOnlyBuffer skeletonHierarchyInput;     //save the parent index(1,2,4,8,16,32,64) for each joint of skeleton
    SimpleReadWriteBuffer SkeletonAnimationResult;    //save the skeletal animation graph simulation result(local space skeleton joint pose)

    //save the world space skeleton joint pose
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap0;
    SimpleReadWriteBuffer worldSpaceSkeletonResultMap1;

    //save the vertex skin result
    SimpleReadWriteBuffer skinVertexResult;

    std::vector<SimpleModelMeshData> meshValueList;

    //world matrix
    SimpleReadOnlyBuffer worldMatrixBufferGpu;
    SimpleBufferStaging worldMatrixBufferCpu;
    //view buffer
    SimpleUniformBuffer viewBufferGpu;
    SimpleBufferStaging viewBufferCpu;


    SimpleStaticMesh floorMesh;
    bool inited = false;

    //pipeline
    Microsoft::WRL::ComPtr<ID3D12PipelineState> meshDrawPipeline;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> skinDrawPipeline;
    //depthstencil
    SimpleDepthStencilBuffer mDepthStencil[3];
public:
    SimpleCamera* GetCamera() { return &baseView; }
    void Create();
    void CreateOnCmdListOpen(const std::string& configFile);
    void UpdateResultBufferCpu();
    bool Inited() { return inited; }
    void DrawDemoData();
};