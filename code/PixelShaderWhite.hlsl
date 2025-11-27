
struct ps_input
{
	float4 vPosition : SV_Position;
	float3 Normal : NORMAL;
};

struct ps_output
{
	float4 color : SV_Target0;
};
float3 LightSource = {-1.0f,3.0f,-2.0f};

ps_output PSEntry(const ps_input input)
{
	ps_output output;
	
	float3 LightingMultiplier = dot(input.Normal.xyz,LightSource);
	
	output.color = float4(1,1,1,1);
	return output;
}