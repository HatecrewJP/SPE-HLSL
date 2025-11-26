
#define PI 3.14159265359
#define DegreeToRad(x) ((x)*PI/180)
struct vs_input
{
	float3 vPosition : SV_Position;
};

struct vs_output
{
	float4 vPosition :SV_Position;
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
	float4 Offset = {0.3,0.2,0,0};
	Input *= Scaling;
	Input = RotationYaw(Input,0);
	Input = RotationPitch(Input,45);
	Input = RotationRoll(Input,45);
	//Input += Offset;
	output.vPosition = mul(Input,OrthographicProjectionMatrix);
	
	return output;
}

