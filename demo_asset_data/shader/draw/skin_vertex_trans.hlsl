#include"globel_header.hlsl"
BaseFragment VSMain(BaseVertex input, uint instanceID : SV_InstanceID,uint vertId : SV_VertexID)
{
   BaseFragment output;
   BaseVertex newVert = input;
   uint skinOffset = vertId * 3;
   float4 value1 = SkinResultVertex[skinOffset];
   float4 value2 = SkinResultVertex[skinOffset + 1];
   float4 value3 = SkinResultVertex[skinOffset + 2];
   newVert.position = value1.xyz;
   newVert.normal.x = value1.w;
   newVert.normal.yz = value2.xy;
   newVert.tangent.xy = value2.zw;
   newVert.tangent.zw = value3.xy;
   TransformVertexBasic(newVert,instanceID,output);
   return output;
}
