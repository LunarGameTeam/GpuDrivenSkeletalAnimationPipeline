StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<uint4> instanceUniformBuffer : register(t2);//{x:boneIDoffset,y:parentIDOffset}
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
	uint curBlockOffset = curUniformData.x * AlignedJointNum;
	uint curBoneFilterId = curBlockOffset + GTid.x;
	uint curSkeletonAssetDataOffset = (curUniformData.y) * AlignedJointNum;
	uint curParentDataSearchOffset = (curSkeletonAssetDataOffset + curBoneFilterId) * 2;
	uint curParentDataCount = gParentPointBuffer[curParentDataSearchOffset];
	uint curParentDataOffset = gParentPointBuffer[curParentDataSearchOffset + 1];
	uint curPoseGlobelBegin = curUniformData.z * AlignedJointNum;
	if (curParentDataCount == 0)
	{
		return;
	}
	int curParentIndex = gParentPointBuffer[curParentDataOffset];
	int curGlobelParentIndex = curPoseGlobelBegin + curParentIndex;
	float4x4 curPoseMatrix = LocalPoseDataBuffer[curGlobelParentIndex];
	PoseDataBufferOut[curGlobelParentIndex] = curPoseMatrix;
	for (int i = 1; i < curParentDataCount; ++i)
	{
		curParentIndex = gParentPointBuffer[curParentDataOffset + i];
		curGlobelParentIndex = curPoseGlobelBegin + curParentIndex;
		curPoseMatrix = mul(curPoseMatrix,LocalPoseDataBuffer[curGlobelParentIndex]);
		PoseDataBufferOut[curGlobelParentIndex] = curPoseMatrix;
	}
}