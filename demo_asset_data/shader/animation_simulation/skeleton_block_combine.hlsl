#include"matrix.hlsl"
StructuredBuffer<float4x4> InputMatrixBuffer : register(t0);
StructuredBuffer<uint> gNeedUpdateIndexBuffer : register(t1);
RWStructuredBuffer<float4x4> OutputMatrixBuffer : register(u2);
[numthreads(64, 1, 1)]
void CSMain(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex
)
{
	uint cur_output_index = SV_DispatchThreadID.x;
	uint cur_parent_index_begin = cur_output_index * 6;
	uint cur_parent_index_0 = gParentPointBuffer[cur_parent_index_begin];
	tempMatrixMulResult[GTid.x] = mul(LocalPoseDataBuffer[SV_DispatchThreadID.x],LocalPoseDataBuffer[cur_parent_index_0]);
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
	PoseDataBufferOut[cur_output_index] = tempMatrixMulResult[GTid.x]
}