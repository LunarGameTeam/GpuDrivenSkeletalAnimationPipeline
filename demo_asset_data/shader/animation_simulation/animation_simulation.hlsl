#include"matrix.hlsl"
StructuredBuffer<uint4> gAnimationInfoBuffer : register(t0);       //static{x: data_offset,y: animation length(by frame),z: animation fps,w: joint num}
StructuredBuffer<float4> gAnimationDataBuffer : register(t1);      //static
StructuredBuffer<float4> simulationParameterBuffer : register(t2); //dynamic(per instance){x: current animation play time,y: animation info globel id,z : joint output offset,w: joint index offset}
static const uint AlignedJointNum = 64;
RWStructuredBuffer<float4x4> LocalPoseDataOutBuffer : register(u3);

float4 quat_lerp(float4 rotation_quaternion_begin, float4 rotation_quaternion_end, float lerp_delta)
{
	float cosom = dot(rotation_quaternion_begin, rotation_quaternion_end);
	float if_change = sign(cosom);
	cosom *= if_change;
	rotation_quaternion_end = rotation_quaternion_end * if_change;
	float sclp, sclq;
	float real_count_cosom = min(cosom, 0.99999);
	float omega, sinom;
	omega = acos(real_count_cosom);
	sinom = sin(omega);
	sclp = sin((1.0f - lerp_delta) * omega) / sinom;
	sclq = sin(lerp_delta * omega) / sinom;
	return rotation_quaternion_begin * sclp + rotation_quaternion_end * sclq;
};

float4 Slerp(float4 Start, float4 End, float T)
{
	float DotProduct = dot(Start, End);
	float DotAbs = abs(DotProduct);

	float T0, T1;
	if (DotAbs > 0.9999f)
	{
		// Start and End are very close together, do linear interpolation.
		T0 = 1.0f - T;
		T1 = T;
	}
	else
	{
		float Omega = acos(DotAbs);
		float InvSin = 1.f / sin(Omega);
		T0 = sin((1.f - T) * Omega) * InvSin;
		T1 = sin(T * Omega) * InvSin;
	}

	T1 = DotProduct < 0 ? -T1 : T1;

	return (T0 * Start) + (T1 * End);
}

void SampleAnimationByIdAndTime(
	in uint cur_joint_id,
	in float cur_anim_time,
	in float4 cur_animation_info,
	out float4 translation_lerp,
	out float4 rotation_lerp,
	out float4 scal_lerp
)
{
	uint globel_anim_offset = cur_animation_info.x;
	uint globel_anim_length = cur_animation_info.y;
	uint anim_joint_num = cur_animation_info.w;
	float frame_per_sec = (float)cur_animation_info.z;
	float cur_lerp_id = cur_anim_time * frame_per_sec;
	uint lerp_st = max(0, floor(cur_lerp_id));
	uint lerp_ed = min(ceil(cur_lerp_id), globel_anim_length);

	float lerp_length = max(float(lerp_ed) - float(lerp_st), 0.00001f);
	float lerp_value = max(cur_lerp_id - float(lerp_st), 0.0f);
	float lerp_delta = saturate(lerp_value / lerp_length);

	uint st_sample_id = 3 * (globel_anim_offset + lerp_st * anim_joint_num + cur_joint_id);
	float4 st_position_vec = gAnimationDataBuffer[st_sample_id];
	float4 st_rotation_vec = gAnimationDataBuffer[st_sample_id + 1];
	float4 st_scale_vec = gAnimationDataBuffer[st_sample_id + 2];

	uint ed_sample_id = 3 * (globel_anim_offset + lerp_ed * anim_joint_num + cur_joint_id);
	float4 ed_position_vec = gAnimationDataBuffer[st_sample_id];
	float4 ed_rotation_vec = gAnimationDataBuffer[st_sample_id + 1];
	float4 ed_scale_vec = gAnimationDataBuffer[st_sample_id + 2];

	translation_lerp = lerp(st_position_vec, ed_position_vec, lerp_delta);
	scal_lerp = lerp(st_scale_vec, ed_scale_vec, lerp_delta);
	rotation_lerp = quat_lerp(st_rotation_vec, ed_rotation_vec, lerp_delta);
}

uint Aligned2Pow(uint size_in, uint size_aligned_in)
{
	return (size_in + (size_aligned_in - 1)) & ~(size_aligned_in - 1);
}

[numthreads(64, 1, 1)]
void CSMain(
	uint3 Gid : SV_GroupID, 
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID
)
{
	uint globel_instance_offset = Gid.x;
	float4 cur_simulation_param = simulationParameterBuffer[globel_instance_offset];
	uint cur_joint_index = cur_simulation_param.w + GTid.x;
	uint cur_anim_id = (uint)(cur_simulation_param.y);
	float4 cur_animation_info = gAnimationInfoBuffer[cur_anim_id];
	uint anim_joint_num = cur_animation_info.w;
	if (cur_joint_index >= anim_joint_num)
	{
		return;
	}
	float cur_anim_time = cur_simulation_param.x;
	uint joint_out_offset =  cur_simulation_param.z;
	//simulation the animation data(simple sample an animation)
	float4 translation_lerp, scal_lerp, rotation_lerp;
	SampleAnimationByIdAndTime(cur_joint_index,cur_anim_time, cur_animation_info,translation_lerp, rotation_lerp, scal_lerp);
	//compose the animation transform to matrix
	float4x4 combine_matrix = compose(translation_lerp.xyz, rotation_lerp, scal_lerp.xyz);
	uint globel_alignment_joint_offset = cur_simulation_param.z + cur_joint_index;
	LocalPoseDataOutBuffer[globel_alignment_joint_offset] = combine_matrix;
}