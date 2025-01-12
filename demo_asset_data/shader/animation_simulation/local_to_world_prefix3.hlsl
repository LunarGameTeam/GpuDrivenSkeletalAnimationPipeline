cbuffer mInstanceOffset : register(b0)
{
	uint4 skeletonGroupCount;
}
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
	uint4 curUniformData = instanceUniformBuffer[DTid.x];
	if (DTid.x > skeletonGroupCount.x)
	{
		return;
	}
	uint currentBeginIndex = DTid.x * AlignedJointNum;
	uint curBlockOffset = curUniformData.x * AlignedJointNum;
	uint curSkeletonAssetDataOffset = curUniformData.y * AlignedJointNum;
	for (int boneOffset = 0; boneOffset < AlignedJointNum; ++boneOffset)
	{
		uint curBoneId = curBlockOffset + boneOffset;
		int curParentId = gParentPointBuffer[curSkeletonAssetDataOffset + curBoneId] - curBlockOffset;
		float4x4 curPoseMatrix = LocalPoseDataBuffer[currentBeginIndex + boneOffset];
		if (curParentId >= 0)
		{
			curPoseMatrix = mul(PoseDataBufferOut[currentBeginIndex + curParentId], curPoseMatrix);
		}
		PoseDataBufferOut[currentBeginIndex + boneOffset] = curPoseMatrix;
	}
}