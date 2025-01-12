StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<uint4> instanceUniformBuffer : register(t2);//{x:boneIDoffset,y:parentIDOffset}
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u3);
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
	uint curParentDataOffset = (curUniformData.y * 21) * AlignedJointNum + curBoneId * 21;
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
	PoseDataBufferOut[DTid.x] = curPoseMatrix * 0.99;
}