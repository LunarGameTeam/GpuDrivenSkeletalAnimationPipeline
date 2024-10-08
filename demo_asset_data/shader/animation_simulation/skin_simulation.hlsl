cbuffer mInstanceOffset : register(b0)
{
	uint4 VertexSkeletonCount;
}
cbuffer mInstanceOffset : register(b1)
{
	uint4 VertexSkeletonOffset;
}

StructuredBuffer<float4x4> SkinMatrixBuffer : register(t0);

StructuredBuffer<float4> SkinBaseVertex : register(t1);

StructuredBuffer<uint4> RefBoneIndexAndWeight : register(t2);

RWStructuredBuffer<float4> SkinOutVertex : register(u3);

void SplitUint32ToUint16(in uint4 data, out uint4 a, out uint4 b)
{
	a.x = (data.x >> 16) & 0xFFFF;
	a.y = data.x & 0xFFFF;
	a.z = (data.y >> 16) & 0xFFFF;
	a.w = data.y & 0xFFFF;

	b.x = (data.z >> 16) & 0xFFFF;
	b.y = data.z & 0xFFFF;
	b.z = (data.w >> 16) & 0xFFFF;
	b.w = data.w & 0xFFFF;
}

void SplitUint32ToNFloat16(in uint4 data, out float4 a, out float4 b)
{
	a.x = (float)((data.x >> 16) & 0xFFFF);
	a.y = (float)(data.x & 0xFFFF);
	a.z = (float)((data.y >> 16) & 0xFFFF);
	a.w = (float)(data.y & 0xFFFF);

	b.x = (float)((data.z >> 16) & 0xFFFF);
	b.y = (float)(data.z & 0xFFFF);
	b.z = (float)((data.w >> 16) & 0xFFFF);
	b.w = (float)(data.w & 0xFFFF);

	a = a / 65535.0f;
	b = b / 65535.0f;
}

void TransformSkinVertex(
	inout float3 position,
	inout float3 normal,
	inout float4 tangent,
	uint instanceIndex,
	uint4 ref0In,
	uint4 ref1In,
	float4 weight0,
	float4 weight1
)
{
	uint globelSkeletonOffset = VertexSkeletonOffset.y + instanceIndex * VertexSkeletonCount.y;
	uint4 ref0 = ref0In + uint4(globelSkeletonOffset, globelSkeletonOffset, globelSkeletonOffset, globelSkeletonOffset);
	uint4 ref1 = ref1In + uint4(globelSkeletonOffset, globelSkeletonOffset, globelSkeletonOffset, globelSkeletonOffset);

	float4 positonLegency = float4(position,1.0f);
	position = float3(0.0f, 0.0f, 0.0f);
	float4 normalLegency = float4(normal, 0.0f);
	normal = float3(0.0f, 0.0f, 0.0f);
	float4 tangentLegency = tangent;
	float3 tangent_out = float3(0.0f, 0.0f, 0.0f);

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;
	tangent_out += mul(tangentLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;

	tangent.xyz = tangent_out;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 threadIndex : SV_DispatchThreadID)
{
	uint vertex_index = threadIndex.x;
	uint vertex_num_per_mesh = VertexSkeletonCount.x;
	uint instance_index = threadIndex.y;
    if(vertex_index >= vertex_num_per_mesh)
    {
        return;
    }
	uint g_vertex_read_offset = VertexSkeletonCount.z;
	uint g_skin_read_offset = VertexSkeletonCount.w;
	//read skin Message
	uint curSkinReadOffset = (g_skin_read_offset + vertex_index) * 2;
	uint4 curBlendIndexCode = RefBoneIndexAndWeight[curSkinReadOffset];
	uint4 curBlendWeightCode = RefBoneIndexAndWeight[curSkinReadOffset + 1];
	uint4 blendIndexA, blendIndexB;
	float4 blendweightA, blendweightB;
	SplitUint32ToUint16(curBlendIndexCode, blendIndexA, blendIndexB);
	SplitUint32ToNFloat16(curBlendWeightCode, blendweightA, blendweightB);
	//read Vertex Message
	uint curVertexReadOffset = (g_vertex_read_offset + vertex_index) * 3;
	float4 value1 = SkinBaseVertex[curVertexReadOffset];
	float4 value2 = SkinBaseVertex[curVertexReadOffset + 1];
	float4 value3 = SkinBaseVertex[curVertexReadOffset + 2];
	float3 position,normal;
	float4 tangent;
	position = value1.xyz;
	normal.x = value1.w;
	normal.yz = value2.xy;
	tangent.xy = value2.zw;
	tangent.zw = value3.xy;
	//GpuSkin
	TransformSkinVertex(position, normal, tangent, instance_index, blendIndexA, blendIndexB, blendweightA, blendweightB);
	//Write Skin Result
	value1.xyz = position;
	value1.w = normal.x;
	value2.xy = normal.yz;
	value2.zw = tangent.xy;
	value3.xy = tangent.zw;
	uint curVertexWriteOffset = (VertexSkeletonOffset.x + instance_index * vertex_num_per_mesh + vertex_index) * 3;
	SkinOutVertex[curVertexWriteOffset] = value1;
	SkinOutVertex[curVertexWriteOffset + 1] = value2;
	SkinOutVertex[curVertexWriteOffset + 2] = value3;
}