#include"globel_header.hlsl"
float4 PSMain(BaseFragment input) : SV_TARGET
{
    float3 skyReflectionColor = _SkyTex.Sample(_ClampSampler, float3(input.uv,0)).rgb;
    float3 skyDiffuseColor = _SkyDiffuseTex.Sample(_ClampSampler, float3(input.uv,0)).rgb;
    float3 brdfColor = _brdfTex.Sample(_ClampSampler, input.uv).rgb;
    float3 texColor = baseColorTex.Sample(_ClampSampler, input.uv).rgb;
    float3 normalColor = normalTex.Sample(_ClampSampler, input.uv).rgb;
    float3 metallicColor = metallicTex.Sample(_ClampSampler, input.uv).rgb;
    float3 roughnessColor = roughnessTex.Sample(_ClampSampler, input.uv).rgb;
    texColor += 0.001f * (skyReflectionColor + skyDiffuseColor + brdfColor + normalColor + metallicColor + roughnessColor);
    return float4(texColor, 1.0f);
}