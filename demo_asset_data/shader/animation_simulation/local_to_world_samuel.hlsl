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
	float4x4 curPoseMatrix = LocalPoseDataBuffer[DTid.x];
	uint curBlockOffset = curUniformData.x * AlignedJointNum;
	uint curBoneId = curBlockOffset + GTid.x;
	uint curSkeletonAssetDataOffset = (curUniformData.y) * AlignedJointNum;
	uint curParentDataOffset = curSkeletonAssetDataOffset + curBoneId;
	int curParentId = gParentPointBuffer[curParentDataOffset];
	uint curPoseGlobelBegin = (Gid.x- curUniformData.x) * AlignedJointNum;
	int layer = 0;
	while(curParentId >= 0)
	{
		layer += 1;
		curPoseMatrix = mul(LocalPoseDataBuffer[curPoseGlobelBegin + curParentId], curPoseMatrix);
		curParentDataOffset = curSkeletonAssetDataOffset + curParentId;
		curParentId = gParentPointBuffer[curParentDataOffset];
	}
	PoseDataBufferOut[DTid.x] = curPoseMatrix;
}