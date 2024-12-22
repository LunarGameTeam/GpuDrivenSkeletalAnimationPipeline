#include"matrix.hlsl"
StructuredBuffer<float4x4> LocalPoseDataBuffer : register(t0);
StructuredBuffer<uint> gParentPointBuffer : register(t1);
StructuredBuffer<uint> instanceUniformBuffer : register(t2);
RWStructuredBuffer<float4x4> PoseDataBufferOut : register(u3);
groupshared float4x4 tempMatrixMulResult[64];
static const uint AlignedJointNum = 64;
[numthreads(64, 1, 1)]
void CSMain(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex
)
{
	uint cur_output_index = DTid.x;
	uint cur_parent_index_begin = cur_output_index * 6;
	uint cur_parent_index_0 = gParentPointBuffer[cur_parent_index_begin];
	tempMatrixMulResult[GTid.x] = mul(LocalPoseDataBuffer[DTid.x], LocalPoseDataBuffer[cur_parent_index_0]);
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult[GTid.x] = mul(tempMatrixMulResult[GTid.x], LocalPoseDataBuffer[cur_parent_index_0 + 1]);
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult[GTid.x] = mul(tempMatrixMulResult[GTid.x], LocalPoseDataBuffer[cur_parent_index_0 + 2]);
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult[GTid.x] = mul(tempMatrixMulResult[GTid.x], LocalPoseDataBuffer[cur_parent_index_0 + 3]);
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult[GTid.x] = mul(tempMatrixMulResult[GTid.x], LocalPoseDataBuffer[cur_parent_index_0 + 4]);
	GroupMemoryBarrierWithGroupSync();
	tempMatrixMulResult[GTid.x] = mul(tempMatrixMulResult[GTid.x], LocalPoseDataBuffer[cur_parent_index_0 + 5]);
	PoseDataBufferOut[cur_output_index] = tempMatrixMulResult[GTid.x];
}
