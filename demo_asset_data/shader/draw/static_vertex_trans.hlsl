#include"globel_header.hlsl"
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
