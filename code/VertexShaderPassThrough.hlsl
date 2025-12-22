struct vs_input
{
	float3 vPosition : SV_Position;
	float4 Color : COLOR;
};

struct vs_output
{
	float4 vPosition :SV_Position;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

vs_output VSEntry(const vs_input input)
{
	vs_output Output;
	
	
	Output.vPosition.xyz = input.vPosition;
	Output.vPosition.w = 1;
	Output.Color = input.Color;
	Output.Normal = float3(0,0,0);
	return Output;
}

