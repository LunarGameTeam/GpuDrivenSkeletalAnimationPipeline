#include"globel_header.hlsl"
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
void TransformSkinVertex(inout float4 position,inout float3 normal,inout float3 tangent,uint4 ref0,uint4 ref1,float4 weight0,float4 weight1)
{
	float4 positonLegency = position;
	position = float4(0.0f, 0.0f, 0.0f,1.0f);
	float4 normalLegency = float4(normal,0.0f);
	normal = float3(0.0f, 0.0f, 0.0f);
	float4 tangentLegency = float4(tangent,0.0f);
	tangent = float3(0.0f, 0.0f, 0.0f);

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref0.x]).xyz * weight0.x;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref0.y]).xyz * weight0.y;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref0.z]).xyz * weight0.z;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;
	normal += mul(normalLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref0.w]).xyz * weight0.w;
	
	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref1.x]).xyz * weight1.x;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref1.y]).xyz * weight1.y;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref1.z]).xyz * weight1.z;

	position.xyz += mul(positonLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;
	normal += mul(normalLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;
	tangent += mul(tangentLegency, SkinMatrixBuffer[ref1.w]).xyz * weight1.w;
}
BaseFragment VSMain(BaseVertex input, uint instanceID : SV_InstanceID)
{
    BaseFragment output;
	// Change the position vector to be 4 units for proper matrix calculations.
    float4 position = float4(input.position, 1.0);
	float3 normal = input.normal;
	float3 tangent = input.tangent;
	matrix worldMatrix = RoWorldMatrixDataBuffer[instanceID];
	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(position, worldMatrix);
	output.viewDir = normalize(cCamPos.xyz - output.position.xyz);
    output.position = mul(output.position, cViewMatrix);
   	output.position = mul(output.position, cProjectionMatrix);
	
	// Calculate the position of the vertice as viewed by the light source.
    output.worldPosition = mul(position, worldMatrix);

	// Store the texture coordinates for the pixel shader.
	output.uv = input.uv;
    output.normal = mul(normal, (float3x3)worldMatrix);
    output.normal = normalize(output.normal);

    return output;
}
