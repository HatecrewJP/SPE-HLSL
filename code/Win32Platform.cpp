

#define ASSERT(x) if(!(x)) *(char*)0=0;

#define MAX_PIXEL_SHADER_COUNT 32
#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))

#define KB(x) (x)*1024
#define MB(x) KB(x)*1024
#define GB(x) MB(x)*1024

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <math.h>
#define MAX_PIPELINE_STATES 64

#define internal static
#define global_variable static

#include "Structs.h"
#include "Win32Platform.h"





global_variable bool GlobalRunning = false;
global_variable bool GlobalAnimationIsActive = false;
global_variable bool GlobalTesselationActive = false;
global_variable bool GlobalGeometryShaderActive = true;

static ShaderColor GlobalActiveShaderColor = WHITE;

global_variable GraphicsPipelineState PipelineStateArray[MAX_PIPELINE_STATES];
global_variable unsigned int PipelineStateCount = 0;
global_variable GraphicsPipelineState ActivePipelineStateArray[MAX_PIPELINE_STATES];
global_variable unsigned int ActivePipelineStateCount = 0;

global_variable const int Zero = 0;

global_variable UINT GlobalPixelShaderInArrayCount = 0;
global_variable UINT GlobalVertexBufferCount = 0;

global_variable ID3D11Device 			*GlobalDevice 			= nullptr;
global_variable IDXGISwapChain1 		*GlobalSwapChain 		= nullptr;
global_variable ID3D11DeviceContext 	*GlobalDeviceContext 	= nullptr;
global_variable ID3D11RenderTargetView 	*GlobalRenderTargetView = nullptr;
global_variable ID3D11Texture2D			*GlobalFrameBuffer 		= nullptr;

global_variable UINT GlobalActiveIndexCount;
global_variable UINT GlobalStrides[1];
global_variable UINT GlobalOffsets[1];

global_variable ID3D11Buffer* 			GlobalVertexBufferArray[32] = {};
global_variable ID3D11Buffer* 			GlobalIndexBufferArray[64] 	= {};
global_variable ID3D11InputLayout *VSInputLayoutArray[64] = {};

global_variable ID3D11VertexShader* 	GlobalVertexShaderArray[64];
global_variable ID3D11HullShader* 		GlobalHullShaderArray[64];
global_variable ID3D11DomainShader* 	GlobalDomainShaderArray[64];
global_variable ID3D11GeometryShader* 	GlobalGeometryShaderArray[64];
global_variable ID3D11PixelShader* 		GlobalPixelShaderArray[MAX_PIXEL_SHADER_COUNT];



global_variable IndexedGeometryObject GlobalIndexedGeometryArray[64];
global_variable unsigned int IndexedGeometryCount = 0;

global_variable ID3D11ShaderResourceView *GlobalCSShaderResourceView = nullptr;
global_variable ID3D11ComputeShader *GlobalComputeShader = nullptr;
global_variable ID3D11Texture2D *GlobalCSShaderResource = nullptr;
global_variable ID3D11UnorderedAccessView *GlobalUAV = nullptr;
ID3D11Texture2D *GlobalUAVTexture = nullptr;

global_variable GraphicsPipelineState NILGraphicsPipelineState = {};
//Windows functions

LRESULT Wndproc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam){
	switch(Message){
		case WM_CLOSE:{
			GlobalRunning = false;
		}break;
		case WM_DESTROY:{
			GlobalRunning = false;
		}break;
	}
	return DefWindowProcA(WindowHandle,Message,WParam,LParam);
}

internal void Win32ProcessError(DWORD Error){
	LPVOID lpMsgBuf = 0;
	FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  Error,
                  MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
                  (LPTSTR) & lpMsgBuf,
                  0, NULL);
	OutputDebugStringA((LPSTR)lpMsgBuf);
	exit(-1);
}
#define ResetModeAndOffset() TesselationMode = 0; PipelineStateOffset = 4;
internal void MessageLoop(ID3D11Device* Device, float *ConstantBuffer){
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
		static int TesselationMode = 0, PipelineStateOffset = 1;
		switch(Message.message){
			case WM_QUIT:{
				GlobalRunning = false;
			}break;
			case WM_KEYDOWN:{
				unsigned int VKCode = (unsigned int) Message.wParam;
				if(VKCode == '1'){
					//PassThrough
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[0]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				else if(VKCode == '2'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[7]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				else if(VKCode == '3'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[8]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				else if(VKCode == '4'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[9]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				else if(VKCode == '5'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[10]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				else if(VKCode == '6'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[11]);
					ConstantBuffer[0] = 200.0f;
					ResetModeAndOffset();
				}
				
				else if(VKCode == 'T'){
					PipelineStateOffset = (PipelineStateOffset+3)%6;
					
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[(TesselationMode%3)+PipelineStateOffset]);
					
				}
				else if(VKCode == VK_SPACE){
					GlobalAnimationIsActive ^= true;
				}
				else if(VKCode == 'C'){
					ClearActivePipelineState();
					PushPipelineState(&PipelineStateArray[(++TesselationMode%3)+PipelineStateOffset]);
				}
				bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
				if((VKCode == VK_F4) && AltKeyWasDown){
					GlobalRunning = false;
				}
			}break;
			
			default:{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}break;
		}
	}
}


//DXGI functions
internal int Win32GetIDXGIInterfacesFromD3DDevice(
	ID3D11Device *Device, 
	IDXGIDevice1 **IdxgiDevice,
	IDXGIAdapter **IdxgiAdapter,
	IDXGIFactory2 **IdxgiFactory)
{
	*IdxgiDevice = NULL;
	*IdxgiAdapter = NULL;
	*IdxgiFactory = NULL;
	
	if(Device){
		Device->QueryInterface(__uuidof(IDXGIDevice1),(void **)IdxgiDevice);
		if(*IdxgiDevice){
			(*IdxgiDevice)->GetAdapter(IdxgiAdapter);
			if(*IdxgiAdapter){
				(*IdxgiAdapter)->GetParent(__uuidof(IDXGIFactory2),(void **)IdxgiFactory);
				if(*IdxgiFactory){
					return 1;
				}
			}
		}
	}
	return 0;
}




internal IDXGISwapChain1* Win32GetSwapChain(
	ID3D11Device *Device, 
	HWND Window,
	IDXGIFactory2 *IdxgiFactory)
{
	DXGI_SAMPLE_DESC SampleDesc = {1,0};
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc1;
	SwapChainDesc1.Width = 0;
	SwapChainDesc1.Height = 0;
	SwapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc1.Stereo = FALSE;
	SwapChainDesc1.SampleDesc = SampleDesc;
	SwapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc1.BufferCount = 2;
	SwapChainDesc1.Scaling = DXGI_SCALING_NONE;
	SwapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapChainDesc1.Flags = 0;
	
	IDXGISwapChain1 *SwapChain;
	if(IdxgiFactory->CreateSwapChainForHwnd(Device,Window,&SwapChainDesc1,NULL,NULL,&SwapChain)!=S_OK){
		return NULL;
	}
	return SwapChain;
	
}

//Shader compilation functions
internal ShaderCode Win32CompileShaderFromFile(LPCWSTR Filename, LPCSTR Entrypoint, LPCSTR Target){
	ID3DBlob *BlobCode;
	ID3DBlob *BlobError;
	ShaderCode Result = {};
	HRESULT res;
	if(!((res=D3DCompileFromFile(Filename, NULL, NULL, Entrypoint, Target, D3DCOMPILE_DEBUG, 0, &BlobCode, &BlobError)) == S_OK)){
		if(BlobError){
			LPCSTR Buffer = (LPCSTR)BlobError->GetBufferPointer();
			OutputDebugStringA(Buffer);
		}
		return Result;
	}
	
	ASSERT(BlobCode);
	
	Result.Code = BlobCode->GetBufferPointer();
	Result.Size = BlobCode->GetBufferSize();
	return Result;
}

//Buffer functions

internal ID3D11Buffer* Win32CreateVertexBuffer(
	ID3D11Device *Device,
	void* VertexBufferData, 
	UINT VertexBufferSize)
{
	D3D11_BUFFER_DESC VertexBufferDesc;
	VertexBufferDesc.ByteWidth = VertexBufferSize;
	VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;
	
	D3D11_SUBRESOURCE_DATA SubresourceData;
	SubresourceData.pSysMem = VertexBufferData;
	SubresourceData.SysMemPitch = 0;
	SubresourceData.SysMemSlicePitch = 0;
	
	ID3D11Buffer *VertexBuffer;
	ASSERT(Device->CreateBuffer(&VertexBufferDesc,&SubresourceData,&VertexBuffer)==S_OK);

	D3D11_BUFFER_DESC VBDesc;
	VertexBuffer->GetDesc(&VBDesc);

	return VertexBuffer;
}

internal ID3D11InputLayout* Win32CreateVertexInputLayout(
	ID3D11Device *Device, 
	ID3D11DeviceContext *DeviceContext,
	void *CompiledVSShaderCode, 
	size_t ShaderSize)
{
	D3D11_INPUT_ELEMENT_DESC VSInputElementDescArray[2];
	VSInputElementDescArray[0].SemanticName = "SV_POSITION";
	VSInputElementDescArray[0].SemanticIndex = 0;
	VSInputElementDescArray[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	VSInputElementDescArray[0].InputSlot = 0;
	VSInputElementDescArray[0].AlignedByteOffset = 0;
	VSInputElementDescArray[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VSInputElementDescArray[0].InstanceDataStepRate = 0;
	
	VSInputElementDescArray[1].SemanticName = "COLOR";
	VSInputElementDescArray[1].SemanticIndex = 0;
	VSInputElementDescArray[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	VSInputElementDescArray[1].InputSlot = 0;
	VSInputElementDescArray[1].AlignedByteOffset = 3*sizeof(float);
	VSInputElementDescArray[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VSInputElementDescArray[1].InstanceDataStepRate = 0;
	
	ID3D11InputLayout *VSInputLayout = NULL;
	ASSERT(Device->CreateInputLayout(VSInputElementDescArray,2,CompiledVSShaderCode,ShaderSize,&VSInputLayout)==S_OK);
	return VSInputLayout;
	
}


internal void CreateVBForIndexedGeometry(
	float *GeometryData,
	unsigned int VertexDataSize,
	unsigned int VertexElementWidth,
	unsigned int *IndexData,
	unsigned int IndexDataSize,
	unsigned int IndexElementWidth)
{
	IndexedGeometryObject NewObject;
	
	NewObject.VertexData = GeometryData;
	NewObject.VertexSize = VertexElementWidth;
	NewObject.VertexCount = VertexDataSize / VertexElementWidth;
	NewObject.VertexDataSize = NewObject.VertexCount * NewObject.VertexSize;
	
	NewObject.IndexData = IndexData;
	NewObject.IndexSize = IndexElementWidth;
	NewObject.IndexCount = IndexDataSize / IndexElementWidth;
	NewObject.IndexDataSize = NewObject.IndexSize * NewObject.IndexCount;
	
	GlobalIndexedGeometryArray[IndexedGeometryCount++]=NewObject;
	
	GlobalVertexBufferArray[GlobalVertexBufferCount] = Win32CreateVertexBuffer(GlobalDevice, NewObject.VertexData, NewObject.VertexDataSize);
	
	D3D11_BUFFER_DESC VBDesc;
	GlobalVertexBufferArray[GlobalVertexBufferCount]->GetDesc(&VBDesc);
	
	D3D11_BUFFER_DESC IndexBufferDesc;
	IndexBufferDesc.ByteWidth = NewObject.IndexDataSize;
	IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = 0;
	
	D3D11_SUBRESOURCE_DATA IndexSubresourceData;
	IndexSubresourceData.pSysMem = NewObject.IndexData;
	IndexSubresourceData.SysMemPitch = 0;
	IndexSubresourceData.SysMemSlicePitch = 0;
	
	HRESULT res = GlobalDevice->CreateBuffer(
		&IndexBufferDesc,
		&IndexSubresourceData,
		&GlobalIndexBufferArray[GlobalVertexBufferCount]);
		
	ASSERT(res == S_OK && GlobalIndexBufferArray[GlobalVertexBufferCount]);
	GlobalVertexBufferCount++;
}

//Shader Creation functions
internal ID3D11VertexShader* Win32CreateVertexShader(
	ID3D11Device *Device, 
	void *CompiledShaderCode, 
	size_t ShaderSize)
{
	ID3D11VertexShader *VertexShader = NULL;
	ASSERT(Device->CreateVertexShader(CompiledShaderCode,ShaderSize,NULL,&VertexShader)==S_OK);
	return VertexShader;
	
}
internal ID3D11PixelShader* Win32CreatePixelShader(
	ID3D11Device *Device, 
	LPCWSTR Filename, 
	LPCSTR Entrypoint, 
	LPCSTR Target)
{
	//Pixel Shader
	ShaderCode PSCode = Win32CompileShaderFromFile(Filename,Entrypoint,Target);
	if(!PSCode.Code) return nullptr;
	ID3D11PixelShader *PixelShader = nullptr;
	ASSERT(Device->CreatePixelShader(PSCode.Code,PSCode.Size,NULL,&PixelShader)==S_OK);
	return PixelShader;
	
}
//PipelineStates
internal UINT SetPipelineState(
	ID3D11DeviceContext *DeviceContext,
	GraphicsPipelineState *PipelineState,
	D3D11_VIEWPORT *ViewportArray, 
	UINT ViewportCount,
	D3D11_RECT *ScissorRectArray,
	UINT ScissorRectCount)
{
	//IA
	DeviceContext->IASetVertexBuffers(0, 
		PipelineState->VertexBufferCount, PipelineState->VertexBufferArray,
		PipelineState->StrideArray, PipelineState->OffsetArray);
	DeviceContext->IASetIndexBuffer(PipelineState->IndexBuffer, PipelineState->IndexBufferFormat, 0);
	DeviceContext->IASetInputLayout(PipelineState->InputLayout);
	DeviceContext->IASetPrimitiveTopology(PipelineState->PrimitiveTopology);
	//VS
	DeviceContext->VSSetShader(PipelineState->VertexShader, nullptr, 0);
	DeviceContext->VSSetConstantBuffers(0,PipelineState->VertexShaderConstantBufferCount,PipelineState->VertexShaderConstantBufferArray);
	//HS
	DeviceContext->HSSetShader(PipelineState->HullShader,nullptr,0);
	//DS
	DeviceContext->DSSetShader(PipelineState->DomainShader,nullptr,0);
	//GS1
	DeviceContext->GSSetShader(PipelineState->GeometryShader,nullptr,0);
	//RS
	DeviceContext->RSSetScissorRects(ScissorRectCount,ScissorRectArray);
	DeviceContext->RSSetState(PipelineState->RasterizerState);
	DeviceContext->RSSetViewports(ViewportCount,ViewportArray);
	//PS
	DeviceContext->PSSetShader(*(PipelineState->PixelShader), nullptr, 0);
	DeviceContext->PSSetConstantBuffers(0,PipelineState->PixelShaderConstantBufferCount,PipelineState->PixelShaderConstantBufferArray);
	//OMS
	DeviceContext->OMSetRenderTargets(PipelineState->RenderTargetViewCount, PipelineState->RenderTargetViewArray, nullptr);
	return PipelineState->IndexCount;
}

internal GraphicsPipelineState BuildPipelineState(
	ID3D11Buffer* *VertexBufferArray,
	UINT VertexBufferCount,
	UINT *StrideArray,
	UINT *OffsetArray,
	ID3D11Buffer *IndexBuffer,
	DXGI_FORMAT IndexBufferFormat,
	UINT IndexCount,
	ID3D11InputLayout *InputLayout,
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology,
	ID3D11VertexShader *VertexShader,
	ID3D11Buffer* *VertexShaderConstantBufferArray,
	UINT VertexShaderConstantBufferCount,
	ID3D11HullShader *HullShader,
	ID3D11DomainShader *DomainShader,
	ID3D11GeometryShader *GeometryShader,
	ID3D11RasterizerState *RasterizerState,
	ID3D11PixelShader* *PixelShader,
	ID3D11Buffer* *PixelShaderConstantBufferArray,
	UINT PixelShaderConstantBufferCount,
	ID3D11RenderTargetView* *RenderTargetViewArray,
	UINT RenderTargetViewCount,
	char *Description = "Unknown"
	)
{
	GraphicsPipelineState NewPipelineState = {};
	NewPipelineState.VertexBufferArray = VertexBufferArray;
	NewPipelineState.VertexBufferCount = VertexBufferCount;
	NewPipelineState.StrideArray = StrideArray;
	NewPipelineState.OffsetArray = OffsetArray;
	NewPipelineState.IndexBuffer = IndexBuffer;
	NewPipelineState.IndexBufferFormat = IndexBufferFormat;
	NewPipelineState.IndexCount = IndexCount;
	NewPipelineState.InputLayout = InputLayout;
	NewPipelineState.PrimitiveTopology = PrimitiveTopology;
	NewPipelineState.VertexShader = VertexShader;
	NewPipelineState.VertexShaderConstantBufferArray = VertexShaderConstantBufferArray;
	NewPipelineState.VertexShaderConstantBufferCount = VertexShaderConstantBufferCount;
	NewPipelineState.HullShader = HullShader;
	NewPipelineState.DomainShader = DomainShader;
	NewPipelineState.GeometryShader = GeometryShader;
	NewPipelineState.RasterizerState = RasterizerState;
	NewPipelineState.PixelShader = PixelShader;
	NewPipelineState.PixelShaderConstantBufferArray = PixelShaderConstantBufferArray;
	NewPipelineState.PixelShaderConstantBufferCount = PixelShaderConstantBufferCount;
	NewPipelineState.RenderTargetViewArray = RenderTargetViewArray;
	NewPipelineState.RenderTargetViewCount = 1;
	NewPipelineState.Description = Description;
	return NewPipelineState;
}
internal void AddPipelineStateToArray(GraphicsPipelineState PipelineState){
	PipelineStateArray[PipelineStateCount] = PipelineState;
	PipelineStateCount++;
}

internal void SetComputeShaderState(ID3D11DeviceContext *DeviceContext, ComputeShaderState &CSState){
	DeviceContext->CSSetUnorderedAccessViews(0,CSState.UnorderedAccessViewCount,CSState.UnorderedAccessViewArray,nullptr);
	DeviceContext->CSSetShaderResources(0,CSState.ShaderResourceViewCount,CSState.ShaderResourceViewArray);
	DeviceContext->CSSetShader(CSState.ComputeShader,nullptr,0);
}
internal void ClearActivePipelineState(){
	for(unsigned int i = 0; i < ActivePipelineStateCount; i++){
		ActivePipelineStateArray[i] = NILGraphicsPipelineState;
	}
	ActivePipelineStateCount = 0;
}
internal void PushPipelineState(GraphicsPipelineState *State){
	ASSERT(State);
	ActivePipelineStateArray[ActivePipelineStateCount++]=*State;
}



//Miscs
internal int Win32AddPixelShaderToArray(
	ID3D11PixelShader** PixelShaderArray, 
	ID3D11PixelShader* PixelShader)
{
	if(PixelShaderArray){
		if(GlobalPixelShaderInArrayCount < MAX_PIXEL_SHADER_COUNT){
			PixelShaderArray[GlobalPixelShaderInArrayCount] = PixelShader;
			GlobalPixelShaderInArrayCount++;
			return GlobalPixelShaderInArrayCount-1;
		}
	}
	return -1;
	
}


internal void ResizeSwapChainBuffers(UINT NewWidth, UINT NewHeight){
	GlobalDeviceContext->OMSetRenderTargets(0,0,0);
	GlobalFrameBuffer->Release();
	GlobalFrameBuffer = nullptr;
	GlobalRenderTargetView->Release();
	GlobalRenderTargetView = nullptr;
	GlobalSwapChain->ResizeBuffers(0,NewWidth,NewHeight,DXGI_FORMAT_UNKNOWN,0);
	if(GlobalSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&GlobalFrameBuffer)==S_OK){
		ASSERT(GlobalFrameBuffer);
		//RenderTargetView
		if(GlobalDevice->CreateRenderTargetView(GlobalFrameBuffer,NULL,&GlobalRenderTargetView)==S_OK){
			ASSERT(GlobalRenderTargetView);
			OutputDebugStringA("Resize Success\n");
		}
		else{
			OutputDebugStringA("Resize RTV failed\n");
		}
	}
	else{
		OutputDebugStringA("Resize Buffer failed\n");
	}
	
}

internal void UpdateCSTexture(UINT Width, UINT Height){
	if(GlobalCSShaderResource){
		GlobalCSShaderResource->Release();
	}
	
	//Create new Texture2D
	D3D11_TEX2D_SRV CSSRV;
	CSSRV.MostDetailedMip = 0;
	CSSRV.MipLevels = 1;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D =  CSSRV;
	
	UINT TextureSize = Width * Height * 4;
	void *CSShaderResourceData =  VirtualAlloc(nullptr,TextureSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	ASSERT(CSShaderResourceData);
	
	D3D11_SUBRESOURCE_DATA CSSubresourceData;
	CSSubresourceData.pSysMem = CSShaderResourceData;
	CSSubresourceData.SysMemPitch = 4;
	CSSubresourceData.SysMemSlicePitch = 0;
	D3D11_TEXTURE2D_DESC Tex2DDesc;
	GlobalFrameBuffer->GetDesc(&Tex2DDesc);
	Tex2DDesc.BindFlags |= (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
	ASSERT(GlobalDevice->CreateTexture2D(&Tex2DDesc,&CSSubresourceData,&GlobalCSShaderResource)==S_OK);
	
	//Create new UAV
	D3D11_BUFFER_UAV UAVElementDesc;
	UAVElementDesc.FirstElement = 0;
	UAVElementDesc.NumElements = TextureSize / 4;
	UAVElementDesc.Flags = 0;
	
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVDesc.Buffer = UAVElementDesc;

	ASSERT(GlobalDevice->CreateUnorderedAccessView(GlobalCSShaderResource,&UAVDesc,&GlobalUAV)==S_OK);
	VirtualFree(CSShaderResourceData, 0, MEM_RELEASE);
}

internal void CycleShaderColors(ShaderColor *CurrentShaderColor){

	*CurrentShaderColor = ShaderColor((*CurrentShaderColor + 1) % SHADER_COLOR_COUNT);
	
}


int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow){
	//Windows Window
	WNDCLASSEXA WindowClass;
	WindowClass.cbSize = sizeof(WNDCLASSEXA);
	WindowClass.style = CS_HREDRAW|CS_OWNDC|CS_VREDRAW;
	WindowClass.lpfnWndProc = Wndproc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = hInst;
	WindowClass.hIcon = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = NULL;
	WindowClass.lpszMenuName = "Direct3D";
	WindowClass.lpszClassName="Direct3DWindow";
	WindowClass.hIconSm = NULL;
	
	ATOM WindowAtom = RegisterClassExA(&WindowClass);
	ASSERT(WindowAtom);
	
	UINT Width = 1920;
	UINT Height = 1080;
	
	
	
	HRESULT res = 0;
	HWND Window = CreateWindowExA(0,WindowClass.lpszClassName,"Direct3D_",WS_OVERLAPPEDWINDOW|WS_VISIBLE, 0, 0, Width, Height,nullptr,nullptr,hInst,nullptr);
	if(Window){
		OutputDebugStringA("Window created\n");
		const D3D_FEATURE_LEVEL FeatureLevels[]={
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			};
		UINT FeatureLevelCount = sizeof(FeatureLevels)/sizeof(FeatureLevels[0]);
		
		RECT ClientRect;
		ASSERT(GetClientRect(Window,&ClientRect));
		
		Width = ClientRect.right - ClientRect.left;
		Height  = ClientRect.bottom - ClientRect.top;
        

        D3D_FEATURE_LEVEL CurrentFeatureLevel;
		//D3D11 Device
		if(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,D3D11_CREATE_DEVICE_DEBUG ,FeatureLevels,FeatureLevelCount,D3D11_SDK_VERSION,
                             &GlobalDevice,&CurrentFeatureLevel,&GlobalDeviceContext)==S_OK){
			OutputDebugStringA("Device Created\n");
			
			
			
			//retrieve IDXGI Interfaces
			IDXGIDevice1 *IdxgiDevice = nullptr;
			IDXGIAdapter *IdxgiAdapter = nullptr;
			IDXGIFactory2 *IdxgiFactory = nullptr;
			
			Win32GetIDXGIInterfacesFromD3DDevice(GlobalDevice, &IdxgiDevice, &IdxgiAdapter, &IdxgiFactory);

			//Swap Chain
			GlobalSwapChain = Win32GetSwapChain(GlobalDevice,Window,IdxgiFactory);
			
			float CubeVertices[]{
				/*Pos*/ -0.50f,-0.50f, 0.00f, /*COLOR*/ 0.00f, 0.00f, 0.00f, 1.00f,
				/*Pos*/  0.50f,-0.50f, 0.00f, /*COLOR*/ 0.00f, 0.00f, 1.00f, 1.00f,
				/*Pos*/ -0.50f, 0.50f, 0.00f, /*COLOR*/ 0.00f, 1.00f, 0.00f, 1.00f,
				/*Pos*/  0.50f, 0.50f, 0.00f, /*COLOR*/ 0.00f, 1.00f, 1.00f, 1.00f,
				/*Pos*/ -0.50f,-0.50f, 1.00f, /*COLOR*/ 1.00f, 0.00f, 0.00f, 1.00f,
				/*Pos*/  0.50f,-0.50f, 1.00f, /*COLOR*/ 1.00f, 0.00f, 1.00f, 1.00f,
				/*Pos*/ -0.50f, 0.50f, 1.00f, /*COLOR*/ 1.00f, 1.00f, 0.00f, 1.00f,
				/*Pos*/  0.50f, 0.50f, 1.00f, /*COLOR*/ 1.00f, 1.00f, 1.00f, 1.00f,
			};
			UINT CubeIndices[]{
				//Front
				0,2,3,
				0,3,1,
				//Back
				5,6,4,
				5,7,6,
				//Left
				4,6,2,
				4,2,0,
				//Right
				1,3,7,
				1,7,5,
				//Top
				2,6,7,
				2,7,3,
				//Bottom
				0,4,5,
				0,5,1
			};
			CreateVBForIndexedGeometry(
				CubeVertices,
				sizeof(CubeVertices),
				7 * sizeof(float),
				CubeIndices,
				sizeof(CubeIndices),
				1*sizeof(UINT));
				
				
				
			float TriVertices[]{
				/*Pos*/ -1.00f,0.00f, 0.00f, /*COLOR*/ 1.00f, 0.00f, 0.00f, 1.00f,
				/*Pos*/ -1.00f,1.00f, 0.00f, /*COLOR*/ 0.00f, 1.00f, 0.00f, 1.00f,
				/*Pos*/  0.00f,1.00f, 0.00f, /*COLOR*/ 0.00f, 0.00f, 1.00f, 1.00f,
				
			};
			UINT TriIndices[]{
				//Front
				0,2,3
			};
			CreateVBForIndexedGeometry(
				TriVertices,
				sizeof(TriVertices),
				7 * sizeof(float),
				TriIndices,
				sizeof(TriVertices),
				1*sizeof(UINT));
			
			
			
				
			#define VSPassTrough GlobalVertexShaderArray[0]
			#define VSPassTroughInputLayout VSInputLayoutArray[0]
			ShaderCode VSCode = Win32CompileShaderFromFile(L"VertexShaderPassThrough.hlsl","VSEntry","vs_5_0");
			ASSERT(VSCode.Code);
			VSPassTrough = Win32CreateVertexShader(GlobalDevice,VSCode.Code,VSCode.Size);
				ASSERT(VSPassTrough);
				VSPassTroughInputLayout = Win32CreateVertexInputLayout(
				GlobalDevice,
				GlobalDeviceContext,
				VSCode.Code,
				VSCode.Size);
				
			VSCode = Win32CompileShaderFromFile(L"VertexShaderCube.hlsl","VSEntry","vs_5_0");
			ASSERT(VSCode.Code);
			#define VSCube GlobalVertexShaderArray[1]
			#define VSCubeInputLayout VSInputLayoutArray[1]
			
			VSCube = Win32CreateVertexShader(GlobalDevice,VSCode.Code,VSCode.Size);
				ASSERT(VSCube);
				VSCubeInputLayout = Win32CreateVertexInputLayout(
				GlobalDevice,
				GlobalDeviceContext,
				VSCode.Code,
				VSCode.Size);	
			
			//Adding PixelShaders
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShaderPassThrough.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShader.hlsl","PSEntry","ps_5_0"));
			
			
			
			
			//HUllShader
			ShaderCode HSCode = Win32CompileShaderFromFile(L"HullShader.hlsl","HSEntry","hs_5_0");
			ASSERT(HSCode.Code);
			res = GlobalDevice->CreateHullShader(HSCode.Code,HSCode.Size,nullptr,&GlobalHullShaderArray[0]);
			ASSERT(res==S_OK);
			
			//DomainShader
			ShaderCode DSCode = Win32CompileShaderFromFile(L"DomainShader.hlsl","DSEntry","ds_5_0");
			ASSERT(DSCode.Code);
			res = GlobalDevice->CreateDomainShader(DSCode.Code,DSCode.Size,nullptr,&GlobalDomainShaderArray[0]);
			ASSERT(res==S_OK);
			
			//GeometryShader
			ShaderCode GSCode = Win32CompileShaderFromFile(L"GeometryShaderSubdiv.hlsl","GSEntry","gs_5_0");
			ASSERT(GSCode.Code);
			res = GlobalDevice->CreateGeometryShader(GSCode.Code,GSCode.Size,nullptr,&GlobalGeometryShaderArray[0]);
			ASSERT(res==S_OK);
			//GeometryShader
			GSCode = Win32CompileShaderFromFile(L"GeometryShaderCube.hlsl","GSEntry","gs_5_0");
			ASSERT(GSCode.Code);
			res = GlobalDevice->CreateGeometryShader(GSCode.Code,GSCode.Size,nullptr,&GlobalGeometryShaderArray[1]);
			ASSERT(res==S_OK);
			
			//Rasterizer
			D3D11_RASTERIZER_DESC RasterizerDesc1;
			RasterizerDesc1.FillMode = D3D11_FILL_WIREFRAME;
			RasterizerDesc1.CullMode = D3D11_CULL_NONE;
			RasterizerDesc1.FrontCounterClockwise = FALSE;
			RasterizerDesc1.DepthBias = 0;
			RasterizerDesc1.DepthBiasClamp = 1.0f;
			RasterizerDesc1.SlopeScaledDepthBias = 0.0f;
			RasterizerDesc1.DepthClipEnable = FALSE;
			RasterizerDesc1.ScissorEnable = FALSE;
			RasterizerDesc1.MultisampleEnable = FALSE;
			RasterizerDesc1.AntialiasedLineEnable = FALSE;
			
			ID3D11RasterizerState *RasterizerState1 = nullptr;
			res = GlobalDevice->CreateRasterizerState(&RasterizerDesc1,&RasterizerState1);
			ASSERT(RasterizerState1);

			D3D11_RASTERIZER_DESC RasterizerDesc2;
			RasterizerDesc2.FillMode = D3D11_FILL_WIREFRAME;
			RasterizerDesc2.CullMode = D3D11_CULL_NONE;
			RasterizerDesc2.FrontCounterClockwise = FALSE;
			RasterizerDesc2.DepthBias = 0;
			RasterizerDesc2.DepthBiasClamp = 1.0f;
			RasterizerDesc2.SlopeScaledDepthBias = 0.0f;
			RasterizerDesc2.DepthClipEnable = FALSE;
			RasterizerDesc2.ScissorEnable = FALSE;
			RasterizerDesc2.MultisampleEnable = FALSE;
			RasterizerDesc2.AntialiasedLineEnable = FALSE;
			
			ID3D11RasterizerState *RasterizerState2 = nullptr;
			res = GlobalDevice->CreateRasterizerState(&RasterizerDesc2,&RasterizerState2);
			ASSERT(RasterizerState2);
			
			
			//FrameBuffer
			if(GlobalSwapChain->GetBuffer(0,__uuidof(ID3D11Resource),(void**)&GlobalFrameBuffer)==S_OK){
				ASSERT(GlobalFrameBuffer);
				//RenderTargetView
				if(GlobalDevice->CreateRenderTargetView(GlobalFrameBuffer,NULL,&GlobalRenderTargetView)==S_OK){
					ASSERT(GlobalRenderTargetView);
					OutputDebugStringA("RTV Success\n");
				}
				else{
					OutputDebugStringA(" RTV failed\n");
				}
				}
			else{
				OutputDebugStringA("SwapChain no Buffer\n");
			}
			
			float ConstantBufferData[64] = {};
			ConstantBufferData[4] = 1.0f;
			ConstantBufferData[5] = 1.0f;
			ConstantBufferData[8] = 1.0f;
			ConstantBufferData[9] = 1.0f;
			//Buffer for Angle
			D3D11_BUFFER_DESC AngleConstantBufferDesc;
			AngleConstantBufferDesc.ByteWidth = sizeof(ConstantBufferData);
			AngleConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			AngleConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			AngleConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			AngleConstantBufferDesc.MiscFlags = 0;
			AngleConstantBufferDesc.StructureByteStride = 0;
			
			D3D11_SUBRESOURCE_DATA ConstantBufferSubresourceData;
			ConstantBufferSubresourceData.pSysMem = ConstantBufferData;
			ConstantBufferSubresourceData.SysMemPitch = 0;
			ConstantBufferSubresourceData.SysMemSlicePitch = 0;
			ID3D11Buffer *ConstantBuffer = nullptr;
			GlobalDevice->CreateBuffer(&AngleConstantBufferDesc,&ConstantBufferSubresourceData,&ConstantBuffer);
			ASSERT(ConstantBuffer);
			
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSPassTrough,
					nullptr,0,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					GlobalPixelShaderArray,
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"0:PassThrough"));
			
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSCube,
					&ConstantBuffer,1,
					nullptr,
					nullptr,
					GlobalGeometryShaderArray[1],
					RasterizerState2,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"1:Cube"));
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSCube,
					&ConstantBuffer,1,
					nullptr,
					nullptr,
					GlobalGeometryShaderArray[0],
					RasterizerState2,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"2:CubeGeometryShaderEnabled"));
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
					VSCube,
					&ConstantBuffer,1,
					GlobalHullShaderArray[0],
					GlobalDomainShaderArray[0],
					nullptr,
					RasterizerState2,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"3:CubeGeometryTesselationEnabled"));
					
					
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[1],1,
					(UINT*)&GlobalIndexedGeometryArray[1].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[1],DXGI_FORMAT_R32_UINT,ArrayCount(TriIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSCube,
					&ConstantBuffer,1,
					nullptr,
					nullptr,
					GlobalGeometryShaderArray[1],
					RasterizerState1,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"4:Tri"));
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[1],1,
					(UINT*)&GlobalIndexedGeometryArray[1].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[1],DXGI_FORMAT_R32_UINT,ArrayCount(TriIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSCube,
					&ConstantBuffer,1,
					nullptr,
					nullptr,
					GlobalGeometryShaderArray[0],
					RasterizerState1,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"5:TriGeometryShaderEnabled"));
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[1],1,
					(UINT*)&GlobalIndexedGeometryArray[1].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[1],DXGI_FORMAT_R32_UINT,ArrayCount(TriIndices),
					VSCubeInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
					VSCube,
					&ConstantBuffer,1,
					GlobalHullShaderArray[0],
					GlobalDomainShaderArray[0],
					nullptr,
					RasterizerState1,
					&GlobalPixelShaderArray[1],
					&ConstantBuffer,1,
					&GlobalRenderTargetView, 1,
					"6:TriGeometryTesselationEnabled"));
			
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSPassTrough,
					nullptr,0,
					nullptr,
					nullptr,
					nullptr,
					RasterizerState2,
					&GlobalPixelShaderArray[0],
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"7:RasterizerSet"));
						
			
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
					VSCube,
					&ConstantBuffer,1,
					nullptr,
					nullptr,
					nullptr,
					RasterizerState2,
					&GlobalPixelShaderArray[0],
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"8:VSActive"));
				
					
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
					VSCube,
					&ConstantBuffer,1,
					GlobalHullShaderArray[0],
					GlobalDomainShaderArray[0],
					nullptr,
					RasterizerState2,
					&GlobalPixelShaderArray[0],
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"9:Tesselation"));
			
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
					VSCube,
					&ConstantBuffer,1,
					GlobalHullShaderArray[0],
					GlobalDomainShaderArray[0],
					GlobalGeometryShaderArray[0],
					RasterizerState2,
					&GlobalPixelShaderArray[0],
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"10:GeometryShader"));
					
			AddPipelineStateToArray(BuildPipelineState(
					&GlobalVertexBufferArray[0],1,
					(UINT*)&GlobalIndexedGeometryArray[0].VertexSize,
					(UINT*)&Zero,
					GlobalIndexBufferArray[0],DXGI_FORMAT_R32_UINT,ArrayCount(CubeIndices),
					VSPassTroughInputLayout,
					D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
					VSCube,
					&ConstantBuffer,1,
					GlobalHullShaderArray[0],
					GlobalDomainShaderArray[0],
					GlobalGeometryShaderArray[0],
					RasterizerState2,
					&GlobalPixelShaderArray[1],
					nullptr,0,
					&GlobalRenderTargetView, 1,
					"11:PixelShader"));
			
			
			
			
			//ComputeShader
			ShaderCode CSCode = Win32CompileShaderFromFile(L"ComputeShader.hlsl","CSEntry","cs_5_0");
			ASSERT(CSCode.Code);
			res = GlobalDevice->CreateComputeShader(CSCode.Code,CSCode.Size,nullptr,&GlobalComputeShader);
			ASSERT(res==S_OK);
			UpdateCSTexture(Width,Height);
			
			ComputeShaderState CSState = {};
			CSState.ShaderResourceViewArray = &GlobalCSShaderResourceView;
			CSState.ShaderResourceViewCount = 1;
			CSState.UnorderedAccessViewArray = &GlobalUAV;
			CSState.UnorderedAccessViewCount = 1;
			CSState.ComputeShader = GlobalComputeShader;
			
			
			PushPipelineState(&PipelineStateArray[1]);
			

			static unsigned int AnimationCount = 1;
			GlobalRunning = true;
			//Loop
			while(GlobalRunning){
				MessageLoop(GlobalDevice,ConstantBufferData);
				if(!GlobalRunning) break;
				
				ASSERT(GetClientRect(Window,&ClientRect));
				UINT NewWidth  = ClientRect.right - ClientRect.left;
				UINT NewHeight = ClientRect.bottom - ClientRect.top;
				
				if(Width != NewWidth || Height!= NewHeight){
					Width = NewWidth;
					Height = NewHeight;
					ResizeSwapChainBuffers(Width,Height);
					UpdateCSTexture(Width,Height);
				}
				
				D3D11_VIEWPORT ViewPort;
				ViewPort.TopLeftX = 0.0f;
				ViewPort.TopLeftY = 0.0f;
				ViewPort.Width = (float)Width;
				ViewPort.Height = (float)Height;
				ViewPort.MinDepth = 0.0f;
				ViewPort.MaxDepth = 1.0f;
				
				
				D3D11_RECT ScissorRect;		

				ScissorRect.left  	= (LONG)(ViewPort.TopLeftX + (Width * 0.25f));
				ScissorRect.top   	= (LONG)(ViewPort.TopLeftY + (Height * 0.25f));
				ScissorRect.right 	= (LONG)(ViewPort.Width * 0.75f);
				ScissorRect.bottom  = (LONG)(ViewPort.Height * 0.75f);
				
				const float RGBA[4] = {0,0,0,1};
				GlobalDeviceContext->ClearRenderTargetView(GlobalRenderTargetView, RGBA);
				
				ConstantBufferData[1]=(float)Width;
				ConstantBufferData[2]=(float)Height;
				
				D3D11_MAPPED_SUBRESOURCE AngleSubresource = {};
					GlobalDeviceContext->Map(ConstantBuffer,0,D3D11_MAP_WRITE_DISCARD,NULL,&AngleSubresource);
					memcpy(AngleSubresource.pData,ConstantBufferData,sizeof(ConstantBufferData));
					
					GlobalDeviceContext->Unmap(ConstantBuffer,0);
				
				
				for(unsigned int i = 0; i< ActivePipelineStateCount;i++){
					UINT DrawIndexCount = SetPipelineState(GlobalDeviceContext,&ActivePipelineStateArray[i],&ViewPort,1,&ScissorRect,1);
					ASSERT(DrawIndexCount);
					GlobalDeviceContext->DrawIndexed(DrawIndexCount,0,0);
				}

				GlobalDeviceContext->CopyResource(GlobalCSShaderResource, GlobalFrameBuffer);
				SetComputeShaderState(GlobalDeviceContext,CSState);
				GlobalDeviceContext->Dispatch(Width,Height,1);
				GlobalDeviceContext->CopyResource(GlobalFrameBuffer,GlobalCSShaderResource);
				
				
				GlobalSwapChain->Present(1, 0);
				
				if (GlobalAnimationIsActive) {
					AnimationCount = (AnimationCount + 1) % (60);
					if (AnimationCount == 0) {
						switch(GlobalActiveShaderColor){
							case WHITE:{
								ConstantBufferData[4] = 1.0f;
								ConstantBufferData[5] = 0.0f;
								ConstantBufferData[8] = 0.0f;
								ConstantBufferData[9] = 1.0f;
								
							}break;
							case RED:{
								ConstantBufferData[4] = 0.0f;
								ConstantBufferData[5] = 1.0f;
								ConstantBufferData[8] = 0.0f;
								ConstantBufferData[9] = 1.0f;
								
							}break;
							case GREEN:{
								ConstantBufferData[4] = 0.0f;
								ConstantBufferData[5] = 0.0f;
								ConstantBufferData[8] = 1.0f;
								ConstantBufferData[9] = 1.0f;
							}break;
							case BLUE:{
								ConstantBufferData[4] = 1.0f;
								ConstantBufferData[5] = 1.0f;
								ConstantBufferData[8] = 1.0f;
								ConstantBufferData[9] = 1.0f;
							}break;
						}
						CycleShaderColors(&GlobalActiveShaderColor);
					}
					ConstantBufferData[0] = (float)fmod((ConstantBufferData[0]+ 0.5f),360.0f);
				}
			}
		}
		else{
			Win32ProcessError(GetLastError());
		}
	}
	else{
		Win32ProcessError(GetLastError());	
	}
		
		
} 