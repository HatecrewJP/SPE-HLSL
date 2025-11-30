struct GSOutput{
	float4 Pos : SV_Position;
	float3 Normal: NORMAL;
};

struct GSInput{
	float4	Pos: SV_Position;
	float3 Normal :NORMAL;
};


[maxvertexcount(9)]
void GSEntry(triangle GSInput InputTri[3] : SV_Position, inout TriangleStream<GSOutput> OutStream){


	float3 PlaneVec1 = InputTri[1].Normal-InputTri[0].Normal;
	float3 PlaneVec2 = InputTri[2].Normal-InputTri[1].Normal;
	float3 FaceNormal = normalize(cross(PlaneVec1,PlaneVec2));
	
	GSOutput output;
	output.Normal = FaceNormal;
	output.Pos = InputTri[0].Pos;
	OutStream.Append(output);
	output.Pos = InputTri[1].Pos;
	OutStream.Append(output);
	output.Pos = InputTri[2].Pos;
	OutStream.Append(output);
	
	
	

	
}