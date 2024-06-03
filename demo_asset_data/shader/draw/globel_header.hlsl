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
	float depthLinear : POSITION1;
    float4 color : COLOR0;
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