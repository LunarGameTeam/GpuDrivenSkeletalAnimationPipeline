StructuredBuffer<float4x4> BindMatrixBuffer : register(t0);
StructuredBuffer<uint> gIndexRefBuffer : register(t1);
StructuredBuffer<float4x4> JointMatrixBuffer : register(t2);
StructuredBuffer<uint4> instanceUniformBuffer : register(t3);//{x:boneIDoffset,y:parentIDOffset}
RWStructuredBuffer<float4x4> OutputMatrixBuffer : register(u4);
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

	uint curBindJointBoneId = curBlockOffset + GTid.x;

	uint curStaticSampleBegin = curUniformData.y * AlignedJointNum;

	float4x4 curBindMatrix = BindMatrixBuffer[curStaticSampleBegin + curBindJointBoneId];

	uint animationBoneIndex = gIndexRefBuffer[curStaticSampleBegin + curBindJointBoneId];

	uint curDynamicSampleBegin = Gid.x * AlignedJointNum - curBlockOffset;

	float4x4 curAnimationMatrix = JointMatrixBuffer[curDynamicSampleBegin + animationBoneIndex];

	float4x4 finalMatrix = mul(curAnimationMatrix, curBindMatrix);

	OutputMatrixBuffer[DTid.x] = transpose(finalMatrix);
}