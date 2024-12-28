StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<uint4> instanceUniformBuffer : register(t2);//{x:boneIDoffset,y:parentIDOffset}
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u3);
groupshared float4x4 tempMatrixMulResult0[64];
groupshared float4x4 tempMatrixMulResult1[64];
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
	uint curParentDataOffset = (curUniformData.y * 6) * AlignedJointNum + curBoneId * 6;
	int curParentId = gParentPointBuffer[curParentDataOffset] - curBlockOffset;
	uint curPoseGlobelBegin = Gid.x * AlignedJointNum;
	tempMatrixMulResult0[GTid.x] = curPoseMatrix;
	if (curParentId >= 0)
	{
		tempMatrixMulResult0[GTid.x] = mul(LocalPoseDataBuffer[curPoseGlobelBegin + curParentId],curPoseMatrix);
	}
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult1[GTid.x] = tempMatrixMulResult0[GTid.x];
	curParentId = gParentPointBuffer[curParentDataOffset + 1] - curBlockOffset;
	if (curParentId >= 0)
	{
		tempMatrixMulResult1[GTid.x] = mul(tempMatrixMulResult0[curParentId], tempMatrixMulResult0[GTid.x]);
	}
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult0[GTid.x] = tempMatrixMulResult1[GTid.x];
	curParentId = gParentPointBuffer[curParentDataOffset + 2] - curBlockOffset;
	if (curParentId >= 0)
	{
		tempMatrixMulResult0[GTid.x] = mul(tempMatrixMulResult1[curParentId], tempMatrixMulResult1[GTid.x]);
	}
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult1[GTid.x] = tempMatrixMulResult0[GTid.x];
	curParentId = gParentPointBuffer[curParentDataOffset + 3] - curBlockOffset;
	if (curParentId >= 0)
	{
		tempMatrixMulResult1[GTid.x] = mul(tempMatrixMulResult0[curParentId],tempMatrixMulResult0[GTid.x]);
	}
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult0[GTid.x] = tempMatrixMulResult1[GTid.x];
	curParentId = gParentPointBuffer[curParentDataOffset + 4] - curBlockOffset;
	if (curParentId >= 0)
	{
		tempMatrixMulResult0[GTid.x] = mul(tempMatrixMulResult1[curParentId], tempMatrixMulResult1[GTid.x]);
	}
	GroupMemoryBarrierWithGroupSync();
	PoseDataBufferOut[DTid.x] = tempMatrixMulResult0[GTid.x];
	curParentId = gParentPointBuffer[curParentDataOffset + 5] - curBlockOffset;
	if (curParentId >= 0)
	{
		PoseDataBufferOut[DTid.x] = mul(tempMatrixMulResult0[curParentId], tempMatrixMulResult0[GTid.x]);
	}
}