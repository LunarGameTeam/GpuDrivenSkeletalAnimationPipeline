StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<int> gParentPoint2Buffer : register(t2);
StructuredBuffer<uint4> objectUniformBuffer : register(t4);//{x:boneIDoffset,y:parentIDOffset}
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u5);
#ifndef MaxBoneCount
#define MaxBoneCount 320
#endif
groupshared float4x4 tempMatrixMulResult0[MaxBoneCount];
static const uint AlignedJointNum = 64;

[numthreads(MaxBoneCount, 1, 1)]
void CSMain(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex
)
{
	uint4 objectUniform = objectUniformBuffer[Gid.x];
	uint groupCount = objectUniform.y;
	uint groupOffset = objectUniform.z;
	uint curGid64Rel = GTid.x / AlignedJointNum;
	uint curGid64 = groupOffset + curGid64Rel;

	uint curMatSamplePos = 0;
	float4x4 curPoseMatrix = 0;
	uint curBlockOffset = 0;
	uint curBoneId =0;
	uint parentFindBegin = 0;
	uint curParentDataOffset = 0;
	if (GTid.x < groupCount * AlignedJointNum)
	{
		curMatSamplePos = groupOffset * AlignedJointNum + GTid.x;
		curPoseMatrix = LocalPoseDataBuffer[curMatSamplePos];

		curBlockOffset = curGid64Rel * AlignedJointNum;
		curBoneId = GTid.x;
		parentFindBegin = objectUniform.x * AlignedJointNum;
		curParentDataOffset = (parentFindBegin + curBoneId) * 21;
		uint curPoseGlobelBegin = curGid64 * AlignedJointNum;
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
	}
	GroupMemoryBarrierWithGroupSync();
	if (GTid.x < groupCount * AlignedJointNum)
	{
		for (int i = 0; i < 7; ++i)
		{
			int curParentId = gParentPointBuffer[curParentDataOffset + 7 + i] - curBlockOffset;
			if (curParentId < 0)
			{
				break;
			}
			curPoseMatrix = mul(tempMatrixMulResult0[curBlockOffset + curParentId], curPoseMatrix);
		}
	}
	GroupMemoryBarrierWithGroupSync();
	if (GTid.x < groupCount * AlignedJointNum)
	{
		tempMatrixMulResult0[GTid.x] = curPoseMatrix;
	}
	GroupMemoryBarrierWithGroupSync();
	if (GTid.x < groupCount * AlignedJointNum)
	{
		uint instanceBoneSampleBegin = groupOffset * AlignedJointNum;
		int cur_bone_parent = gParentPoint2Buffer[parentFindBegin + curBoneId];
		while (cur_bone_parent >= 0)
		{
			curPoseMatrix = mul(tempMatrixMulResult0[cur_bone_parent], curPoseMatrix);
			cur_bone_parent = gParentPoint2Buffer[parentFindBegin + cur_bone_parent];
		}
		PoseDataBufferOut[curMatSamplePos] = curPoseMatrix;
	}
}