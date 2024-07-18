#include"globel_header.hlsl"
BaseFragment VSMain(BaseVertex input, uint instanceID : SV_InstanceID)
{
    BaseFragment output;
	TransformVertexBasic(input,instanceID,output);
	// Store the texture coordinates for the pixel shader.
	output.uv = output.uv*6000;
    return output;
}
