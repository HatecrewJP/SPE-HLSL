struct ps_input
{
	float4 vPosition : SV_Position;
 	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

struct ps_output
{
	float4 color : SV_Target0;
};

ps_output PSEntry(const ps_input input)
{
	ps_output output;
	output.color = input.Color;
	return output;
}