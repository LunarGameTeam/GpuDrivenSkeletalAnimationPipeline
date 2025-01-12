StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<int> gParentPoint2Buffer : register(t2);
StructuredBuffer<uint4> instanceUniformBuffer : register(t3);//{x:boneIDoffset,y:parentIDOffset}
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u4);
RWStructuredBuffer<float4x4> PoseDataBufferOut2 : register(u5);
groupshared float4x4 tempMatrixMulResult0[64];
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
	uint parentFindBegin = curUniformData.y * AlignedJointNum;
	uint curParentDataOffset = (parentFindBegin + curBoneId) * 21;
	uint curPoseGlobelBegin = Gid.x * AlignedJointNum;
	for (int i = 0; i < 7; ++i)
	{
		int curParentId = gParentPointBuffer[curParentDataOffset + i] - curBlockOffset;
		if (curParentId < 0)
		{
			break;
		}
		curPoseMatrix = mul(LocalPoseDataBuffer[curPoseGlobelBegin + curParentId], curPoseMatrix);
	}
	tempMatrixMulResult0[GTid.x] = curPoseMatrix;

	GroupMemoryBarrierWithGroupSync();

	for (int i = 0; i < 7; ++i)
	{
		int curParentId = gParentPointBuffer[curParentDataOffset + 7 + i] - curBlockOffset;
		if (curParentId < 0)
		{
			break;
		}
		curPoseMatrix = mul(tempMatrixMulResult0[curParentId], curPoseMatrix);
	}
	PoseDataBufferOut[DTid.x] = curPoseMatrix;

	DeviceMemoryBarrierWithGroupSync();

	uint instanceBoneSampleBegin = (Gid.x - curUniformData.x) * AlignedJointNum;
	int cur_bone_parent = gParentPoint2Buffer[parentFindBegin + curBoneId];
	curPoseMatrix = PoseDataBufferOut[DTid.x];
	while (cur_bone_parent >= 0)
	{
		curPoseMatrix = mul(PoseDataBufferOut[instanceBoneSampleBegin + cur_bone_parent], curPoseMatrix);
		cur_bone_parent = gParentPoint2Buffer[parentFindBegin + cur_bone_parent];
	}
	PoseDataBufferOut2[DTid.x] = curPoseMatrix;
}