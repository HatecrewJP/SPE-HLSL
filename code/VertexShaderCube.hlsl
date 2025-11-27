
#define PI 3.14159265359
#define DegreeToRad(x) ((x)*PI/180)

cbuffer CBufferAngle{
	float RotationAngle;
	float Width;
	float Height;
};
struct vs_input
{
	float3 vPosition : SV_Position;
	float3 Normal : NORMAL;
};

struct vs_output
{
	float4 vPosition :SV_Position;
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
	
	
	
	float4x4 OrthographicProjectionMatrix = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,-1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f};
	float4 Scaling = {0.5f,0.5f,0.5f,1};
	
	
	float4 ResultPos =  float4(input.vPosition,1);
	ResultPos *= Scaling;
	ResultPos = RotationYaw(ResultPos,RotationAngle);
	ResultPos = RotationPitch(ResultPos,RotationAngle);
	ResultPos = RotationRoll(ResultPos,0);
	ResultPos.x /= (Width/Height);
	output.vPosition = mul(ResultPos,OrthographicProjectionMatrix);
	
	float4 ResultNormal =  float4(input.Normal,1);
	ResultNormal *= Scaling;
	ResultNormal = RotationYaw(ResultNormal,RotationAngle);
	ResultNormal = RotationPitch(ResultNormal,RotationAngle);
	ResultNormal = RotationRoll(ResultNormal,0);
	ResultNormal.x /= (Width/Height);
	output.Normal = mul(ResultNormal,OrthographicProjectionMatrix);
	
	
	return output;
}

