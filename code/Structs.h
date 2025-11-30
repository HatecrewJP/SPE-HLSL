#pragma once

struct ShaderCode{
	void *Code;
	size_t Size;
};
struct GraphicsPipelineState{
	//IA
	ID3D11Buffer* *VertexBufferArray;
	UINT VertexBufferCount;
	UINT *StrideArray;
	UINT *OffsetArray;
	ID3D11Buffer *IndexBuffer;
	DXGI_FORMAT IndexBufferFormat;
	UINT IndexCount;
	ID3D11InputLayout *InputLayout;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
	//VS
	ID3D11VertexShader *VertexShader;
	ID3D11Buffer* *VertexShaderConstantBufferArray;
	UINT VertexShaderConstantBufferCount;
	//HS
	ID3D11HullShader *HullShader;
	//DS
	ID3D11DomainShader *DomainShader;
	//GS
	ID3D11GeometryShader *GeometryShader;
	//RS
	ID3D11RasterizerState *RasterizerState;
	//PS
	ID3D11PixelShader* *PixelShader;
	ID3D11Buffer* *PixelShaderConstantBufferArray;
	UINT PixelShaderConstantBufferCount;
	//OMS
	ID3D11RenderTargetView* *RenderTargetViewArray;
	UINT RenderTargetViewCount;
	char *Description;
};

struct ComputeShaderState{
	ID3D11ShaderResourceView* *ShaderResourceViewArray;
	UINT ShaderResourceViewCount;
	
	ID3D11UnorderedAccessView* *UnorderedAccessViewArray;
	UINT UnorderedAccessViewCount;

	ID3D11ComputeShader *ComputeShader;
};


struct IndexedGeometryObject{
	void *VertexData;
	UINT VertexSize;
	UINT VertexCount;
	UINT VertexDataSize;
	
	UINT *IndexData;
	UINT IndexSize;
	UINT IndexCount;
	UINT IndexDataSize;
};

enum ShaderColor{
	RED,
	GREEN,
	WHITE,
	BLUE,
	
	SHADER_COLOR_COUNT
};