#include"matrix.hlsl"
StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<uint> instanceUniformBuffer : register(t2);
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u3);
static const uint AlignedJointNum = 64;
[numthreads(64, 1, 1)]
void CSMain(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex
)
{
	uint4 curUniformData = instanceUniformBuffer[Gid.x];
	//bone matrix
	uint curSampleBegin = curUniformData.z * AlignedJointNum;
	uint globelSampleIndex = curSampleBegin + GTid.x;
	float4x4 curPoseMatrix = LocalPoseDataBuffer[curSampleBegin + GTid.x];
	//relative bone id
	uint curBlockOffset = curUniformData.x * AlignedJointNum;
	uint curPoseGlobelBegin = curPoseMatrix - curBlockOffset;
	int cur_bone_parent = gParentPointBuffer[DTid.x];
	if(cur_bone_parent > 0)
	{
		curPoseMatrix = mul(curPoseMatrix,LocalPoseDataBuffer[curPoseGlobelBegin + cur_bone_parent]);
	}
	PoseDataBufferOut[globelSampleIndex] = curPoseMatrix;
}
