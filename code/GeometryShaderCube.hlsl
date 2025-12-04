struct GSOutput{
	float4 Pos : SV_Position;
	float4 Color : COLOR;
	float3 Normal: NORMAL;
};

struct GSInput{
	float4 Pos: SV_Position;
	float4 Color : COLOR;

};


[maxvertexcount(9)]
void GSEntry(triangle GSInput InputTri[3] : SV_Position, inout TriangleStream<GSOutput> OutStream){

	float4 PlaneVec1 = InputTri[1].Pos-InputTri[0].Pos;
	float4 PlaneVec2 = InputTri[2].Pos-InputTri[1].Pos;
	float3 FaceNormal = normalize(cross(PlaneVec1.xyz,PlaneVec2.xyz));
	
	GSOutput output;
	output.Color.xyz = (InputTri[0].Color.xyz + InputTri[1].Color.xyz + InputTri[2].Color.xyz)/3.0f;
	output.Color.w = 1.0f;
	output.Normal = FaceNormal;
	output.Pos = InputTri[0].Pos;
	OutStream.Append(output);
	output.Pos = InputTri[1].Pos;
	OutStream.Append(output);
	output.Pos = InputTri[2].Pos;
	OutStream.Append(output);
}