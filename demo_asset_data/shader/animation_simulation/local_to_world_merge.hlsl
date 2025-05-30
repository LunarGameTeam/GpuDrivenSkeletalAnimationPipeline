StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<int> gParentPointBuffer : register(t1);
StructuredBuffer<uint4> instanceUniformBuffer : register(t2);
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
	uint parentFindBegin = curUniformData.y * AlignedJointNum;
	uint instanceBoneSampleBegin = (Gid.x - curUniformData.x) * AlignedJointNum;
	int cur_bone_id = curUniformData.x * AlignedJointNum + GTid.x;
	int cur_bone_parent = gParentPointBuffer[parentFindBegin + cur_bone_id];
	float4x4 curPoseMatrix = LocalPoseDataBuffer[DTid.x];
	while (cur_bone_parent >= 0)
	{
		curPoseMatrix = mul(LocalPoseDataBuffer[instanceBoneSampleBegin + cur_bone_parent], curPoseMatrix);
		cur_bone_parent = gParentPointBuffer[parentFindBegin + cur_bone_parent];
	}
	PoseDataBufferOut[DTid.x] = curPoseMatrix;
}
