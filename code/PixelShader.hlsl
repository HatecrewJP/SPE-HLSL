cbuffer CBuffer : register(b0){
	float RotationAngle : packoffset(c0.x);
	float Width  : packoffset(c0.y);
	float Height : packoffset(c0.z);
	float ColorR : packoffset(c1.x);
	float ColorG : packoffset(c1.y);
	float ColorB : packoffset(c2.x);
	float ColorA : packoffset(c2.y);
};

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
	[branch]if(input.Color.x == 1.0f){
		output.color = float4(ColorR,ColorG,ColorB,ColorA);
	}
	[flatten]if(input.Color.x == 0.0f){
		output.color = float4(ColorR,ColorG,ColorB,ColorA);
	}
	return output;
}