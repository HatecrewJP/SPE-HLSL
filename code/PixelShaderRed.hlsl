
struct ps_input
{
	float4 vPosition : POSITION;
};

struct ps_output
{
	float4 color : COLOR;
};

ps_output PSEntry(const ps_input input): COLOR
{
	ps_output output;
	
	output.color = float4(1,0,0,1);
	return output;
}