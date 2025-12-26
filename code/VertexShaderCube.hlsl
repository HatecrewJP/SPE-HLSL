
#define PI 3.14159265359
#define DegreeToRad(x) ((x)*PI/180)

cbuffer CBuffer{
	float RotationAngle;
	float Width;
	float Height;
	float ColorR;
	float ColorG;
	float ColorB;
	float ColorA;
	
};
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

float4 RotationYaw(float4 Vec4,float Angle){
	float RadAngle = DegreeToRad(Angle);
	float4x4 YawRotationMatrix = {
		cos(RadAngle),-sin(RadAngle),0.0f,0.0f,
		sin(RadAngle),cos(RadAngle),0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f};
		
	return mul(Vec4,YawRotationMatrix);
}
float4 RotationPitch(float4 Vec4,float Angle){
	float RadAngle = DegreeToRad(Angle);
	float4x4 PitchRotationMatrix = {
		cos(RadAngle),0.0f,-sin(RadAngle),0.0f,
		0.0f,1.0f,0.0f,0.0f,
		sin(RadAngle),0.0f,cos(RadAngle),0.0f,
		0.0f,0.0f,0.0f,1.0f};
		
	return mul(Vec4,PitchRotationMatrix);
}
float4 RotationRoll(float4 Vec4,float Angle){
	float RadAngle = DegreeToRad(Angle);
	float4x4 RollRotationMatrix = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,cos(RadAngle),-sin(RadAngle),0.0f,
		0.0f,sin(RadAngle),cos(RadAngle),0.0f,
		0.0f,0.0f,0.0f,1.0f};
		
	return mul(Vec4,RollRotationMatrix);
}


vs_output VSEntry(const vs_input input)
{
	vs_output output;
	
	
	float4 Input =  float4(input.vPosition,1);
	

	float4x4 OrthographicProjectionMatrix = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,-1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f};
	
	float4 Scaling = {0.5f,0.5f,0.5f,1};
	
	
	Input *= Scaling;
	Input = RotationYaw(Input,RotationAngle);
	Input = RotationPitch(Input,RotationAngle);
	Input = RotationRoll(Input,0);
	Input.x /= (Width/Height);
	output.vPosition = mul(Input,OrthographicProjectionMatrix);
	output.Color =  input.Color;
	output.Normal = float3(0,0,0);
	return output;
}

