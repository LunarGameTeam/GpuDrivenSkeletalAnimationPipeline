#include"globel_header.hlsl"
BaseFragment VSMain(BaseVertex input, uint inst : SV_InstanceID)
{
    BaseFragment output;	    	
    output.position = float4(input.position*3000 + cCamPos.xyz, 1.0);
    output.position = mul(output.position, cViewMatrix);
    output.position = mul(output.position, cProjectionMatrix);
    output.normal = input.normal;
    return output;
}

float4 PSMain(BaseFragment input) : SV_TARGET
{
    float3 texColor = _SkyTex.SampleLevel(_ClampSampler, normalize(-input.normal),0).rgb;
    return float4(texColor, 1.0f);
}