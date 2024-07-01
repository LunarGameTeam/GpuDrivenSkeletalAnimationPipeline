#include"globel_header.hlsl"
BaseFragment VSMain(BaseVertex input, uint instanceID : SV_InstanceID)
{
   BaseFragment output;
   TransformVertexBasic(input,instanceID,output);
   return output;
}
