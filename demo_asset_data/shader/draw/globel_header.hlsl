struct BaseVertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};
struct BaseFragment
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
	float3 normal : NORMAL;	
    float4 worldPosition : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
};
cbuffer ViewBuffer : register(b0)
{
	matrix cViewMatrix;
	matrix cProjectionMatrix;
	float4 cCamPos;
};
StructuredBuffer<float4x4> RoWorldMatrixDataBuffer : register(t0);
StructuredBuffer<float4x4> SkinMatrixBuffer : register(t1);
TextureCube _SkyTex : register(t2);
TextureCube _SkyDiffuseTex : register(t3);
Texture2D   _brdfTex : register(t4);
Texture2D   baseColorTex : register(t5);
Texture2D   normalTex : register(t6);
Texture2D   metallicTex : register(t7);
Texture2D   roughnessTex : register(t8);
SamplerState _ClampSampler : register(s0);
struct InstanceBufferOffsetDesc
{
	uint4 mOffset;
};
cbuffer mInstanceOffset: register(b1)
{
    uint4 worldInstanceOffset;
}

void TransformVertexBasic(BaseVertex input, uint instanceID,out BaseFragment output)
{
    // Change the position vector to be 4 units for proper matrix calculations.
    float4 position = float4(input.position, 1.0);
	float3 normal = input.normal;
	float4 tangent = input.tangent;
    uint globelInstanceId = worldInstanceOffset.x + instanceID;
	matrix worldMatrix = RoWorldMatrixDataBuffer[globelInstanceId];
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
}