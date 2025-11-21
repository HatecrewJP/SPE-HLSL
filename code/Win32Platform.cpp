

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

#define internal static
#define global_variable static
typedef int bool32;

#include "Win32Platform.h"

global_variable bool32 GlobalRunning = false;
global_variable UINT GlobalPixelShaderInArrayCount = 0;
global_variable bool32 GlobalAnimationIsActive = false;

global_variable ID3D11Device 			*GlobalDevice 			= NULL;
global_variable IDXGISwapChain1 		*GlobalSwapChain 		= NULL;
global_variable ID3D11DeviceContext 	*GlobalDeviceContext 	= NULL;
global_variable ID3D11RenderTargetView 	*GlobalRenderTargetView = NULL;
global_variable ID3D11Texture2D			*GlobalFrameBuffer 		= NULL;

global_variable ID3D11PixelShader 	*GlobalActivePixelShader;
global_variable ID3D11VertexShader 	*GlobalActiveVertexShader;
global_variable ID3D11Buffer 		*GlobalActiveVertexBuffer;
global_variable ID3D11Buffer 		*GlobalActiveIndexBuffer;
global_variable UINT GlobalActiveIndexCount;
global_variable UINT GlobalStrides[1];
global_variable UINT GlobalOffsets[1];

global_variable ID3D11Buffer* 			GlobalVertexBufferArray[32] = {};
global_variable ID3D11Buffer* 			GlobalIndexBufferArray[64] 	= {};

global_variable ID3D11VertexShader 		*GlobalVertexShader 		= NULL;
global_variable ID3D11HullShader 		*GlobalHullShader 			= NULL;
global_variable ID3D11DomainShader 		*GlobalDomainShader 		= NULL;
global_variable ID3D11GeometryShader 	*GlobalGeometryShader 		= NULL;
global_variable ID3D11PixelShader* 		GlobalPixelShaderArray[MAX_PIXEL_SHADER_COUNT];

global_variable IndexedGeometryObject GlobalSquare;
global_variable IndexedGeometryObject GlobalHexagon;

global_variable ID3D11ComputeShader *GlobalComputeShader = nullptr;
global_variable ID3D11UnorderedAccessView *GlobalUAV = nullptr;

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

internal int Win32GetIDXGIInterfacesFromD3DDevice(ID3D11Device *Device, IDXGIDevice1 **IdxgiDevice,IDXGIAdapter **IdxgiAdapter,IDXGIFactory2 **IdxgiFactory ){
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

internal IDXGISwapChain1* Win32GetSwapChain(ID3D11Device *Device, HWND Window,IDXGIFactory2 *IdxgiFactory){
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

internal ID3DBlob* Win32CompileShaderFromFile(LPCWSTR Filename, LPCSTR Entrypoint, LPCSTR Target){
	ID3DBlob *BlobCode;
	ID3DBlob *BlobError;
	
	HRESULT res;
	if(!((res=D3DCompileFromFile(Filename, NULL, NULL, Entrypoint, Target, /*D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | */ D3DCOMPILE_DEBUG, 0, &BlobCode, &BlobError)) == S_OK)){
		if(BlobError){
			LPCSTR Buffer = (LPCSTR)BlobError->GetBufferPointer();
			OutputDebugStringA(Buffer);
		}
		return NULL;
	}
	
	ASSERT(BlobCode);
	return BlobCode;
}

internal ID3D11Buffer* Win32CreateVertexBuffer(ID3D11Device *Device,void* VertexBufferData, UINT VertexBufferSize){
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

internal ID3D11InputLayout* Win32CreateVertexInputLayout(ID3D11Device *Device, ID3D11DeviceContext *DeviceContext,void *CompiledVSShaderCode, size_t ShaderSize){
	D3D11_INPUT_ELEMENT_DESC VSInputElementDesc;
	VSInputElementDesc.SemanticName = "SV_POSITION";
	VSInputElementDesc.SemanticIndex = 0;
	VSInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	VSInputElementDesc.InputSlot = 0;
	VSInputElementDesc.AlignedByteOffset = 0;
	VSInputElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VSInputElementDesc.InstanceDataStepRate = 0;
	
	ID3D11InputLayout *VSInputLayout = NULL;
	ASSERT(Device->CreateInputLayout(&VSInputElementDesc,1,CompiledVSShaderCode,ShaderSize,&VSInputLayout)==S_OK);
	return VSInputLayout;
	
}

internal ID3D11VertexShader* Win32CreateVertexShader(ID3D11Device *Device, void *CompiledShaderCode, size_t ShaderSize){
	ID3D11VertexShader *VertexShader = NULL;
	ASSERT(Device->CreateVertexShader(CompiledShaderCode,ShaderSize,NULL,&VertexShader)==S_OK);
	return VertexShader;
	
}
internal ID3D11PixelShader* Win32CreatePixelShader(ID3D11Device *Device, LPCWSTR Filename, LPCSTR Entrypoint, LPCSTR Target){
	//Pixel Shader
	ID3DBlob *BlobPSCode = Win32CompileShaderFromFile(Filename,Entrypoint,Target);
	void *CompiledPSShaderCode = BlobPSCode->GetBufferPointer();
	size_t PSShaderSize = BlobPSCode->GetBufferSize();
	ASSERT(CompiledPSShaderCode);
	ID3D11PixelShader *PixelShader = NULL;
	ASSERT(Device->CreatePixelShader(CompiledPSShaderCode,PSShaderSize,NULL,&PixelShader)==S_OK);
	return PixelShader;
	
}
internal int Win32AddPixelShaderToArray(ID3D11PixelShader** PixelShaderArray, ID3D11PixelShader* PixelShader){
	if(PixelShaderArray){
		if(GlobalPixelShaderInArrayCount < MAX_PIXEL_SHADER_COUNT){
			PixelShaderArray[GlobalPixelShaderInArrayCount] = PixelShader;
			GlobalPixelShaderInArrayCount++;
			return GlobalPixelShaderInArrayCount-1;
		}
	}
	return -1;
	
}

internal void MessageLoop(ID3D11Device* Device){
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
		
		switch(Message.message){
			case WM_QUIT:{
				GlobalRunning = false;
			}break;
			
			case WM_KEYDOWN:{
				unsigned int VKCode = (unsigned int) Message.wParam;
				if(VKCode == '1'){
					if(GlobalPixelShaderInArrayCount >= 1 ) GlobalActivePixelShader = GlobalPixelShaderArray[0];
					
				}
				else if(VKCode == '2'){
					if(GlobalPixelShaderInArrayCount >= 2 ) GlobalActivePixelShader = GlobalPixelShaderArray[1];
				}
				else if(VKCode == '3'){
					if(GlobalPixelShaderInArrayCount >= 3 ) GlobalActivePixelShader = GlobalPixelShaderArray[2];
				}
				else if(VKCode == '4'){
					if(GlobalPixelShaderInArrayCount >= 4 ) GlobalActivePixelShader = GlobalPixelShaderArray[3];
				}
				else if(VKCode == '5'){
					if(GlobalPixelShaderInArrayCount >= 5 ) GlobalActivePixelShader = GlobalPixelShaderArray[4];
				}
				else if(VKCode == '6'){
					if(GlobalPixelShaderInArrayCount >= 6 ) GlobalActivePixelShader = GlobalPixelShaderArray[5];
				}
				else if(VKCode == VK_SPACE){
					GlobalAnimationIsActive ^= true;
				}
				else if(VKCode == 'R'){
					HRESULT res;
					ID3DBlob *HSBlob;
					ASSERT(GlobalHullShader)
					GlobalHullShader->Release();
					HSBlob = Win32CompileShaderFromFile(L"HullShader.hlsl","HSEntry","hs_5_0");
					void* x = HSBlob->GetBufferPointer();
					ASSERT(HSBlob);
					void *CompiledHS = HSBlob->GetBufferPointer();
					SIZE_T CompiledHSSize = HSBlob->GetBufferSize();
					res = Device->CreateHullShader(CompiledHS,CompiledHSSize,nullptr,&GlobalHullShader);
					ASSERT(res==S_OK);
					
					ID3DBlob *DSBlob;
					ASSERT(GlobalDomainShader)
					GlobalDomainShader->Release();
					DSBlob = Win32CompileShaderFromFile(L"DomainShader.hlsl","DSEntry","ds_5_0");
					ASSERT(DSBlob);
					void *CompiledDS = DSBlob->GetBufferPointer();
					SIZE_T CompiledDSSize = DSBlob->GetBufferSize();
					res = Device->CreateDomainShader(CompiledDS,CompiledDSSize,nullptr,&GlobalDomainShader);
					ASSERT(res==S_OK);
				}
				
				else if(VKCode == 'H'){
					GlobalActiveVertexBuffer = GlobalVertexBufferArray[1];
					GlobalActiveIndexBuffer = GlobalIndexBufferArray[1];
					GlobalActiveIndexCount = GlobalHexagon.IndexCount;
					GlobalStrides[0] = GlobalHexagon.VertexSize;
					GlobalActivePixelShader = GlobalPixelShaderArray[2];
				}
				else if(VKCode == 'S'){
					GlobalActiveVertexBuffer = GlobalVertexBufferArray[0];
					GlobalActiveIndexBuffer = GlobalIndexBufferArray[0];
					GlobalActiveIndexCount = GlobalSquare.IndexCount;
					GlobalStrides[0] = GlobalSquare.VertexSize;
					GlobalActivePixelShader = GlobalPixelShaderArray[0];
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
			IDXGIDevice1 *IdxgiDevice = NULL;
			IDXGIAdapter *IdxgiAdapter = NULL;
			IDXGIFactory2 *IdxgiFactory = NULL;
			
			Win32GetIDXGIInterfacesFromD3DDevice(GlobalDevice, &IdxgiDevice, &IdxgiAdapter, &IdxgiFactory);

			//Swap Chain
			GlobalSwapChain = Win32GetSwapChain(GlobalDevice,Window,IdxgiFactory);
			
			//Vertex Shader Compilation
			ID3DBlob *BlobVSCode = Win32CompileShaderFromFile(L"VertexShader.hlsl","VSEntry","vs_5_0");
			ASSERT(BlobVSCode);
			void *CompiledVSShaderCode = BlobVSCode->GetBufferPointer();
			size_t VSShaderSize = BlobVSCode->GetBufferSize();
			ASSERT(CompiledVSShaderCode);
			OutputDebugStringA("VertexShader compiled\n");
			
			//Vertex Shader 
			GlobalVertexShader = Win32CreateVertexShader(GlobalDevice,CompiledVSShaderCode,VSShaderSize);
			ASSERT(GlobalVertexShader);
			OutputDebugStringA("Vertex Shader created\n");
			ID3D11InputLayout *VSInputLayout = Win32CreateVertexInputLayout(GlobalDevice,GlobalDeviceContext,CompiledVSShaderCode,VSShaderSize);
			GlobalDeviceContext->IASetInputLayout(VSInputLayout);
			
			
			
			//Square Object
			
			float SquareVertices[] {
				//Clockwise Triangles

				//TopLeft
				-1.0f, 1.0f,0.0f, //0
				 1.0f, 1.0f,0.0f, //1
				-1.0f,-1.0f,0.0f, //2
				 1.0f,-1.0f,0.0f, //3
				
			};
			
			UINT SquareIndices[]{
				0,1,2,
				2,1,3,
				
			};
			
			GlobalSquare.VertexData = SquareVertices;
			GlobalSquare.VertexSize = sizeof(float) * 3;
			GlobalSquare.VertexCount = 4;
			GlobalSquare.VertexDataSize = GlobalSquare.VertexCount * GlobalSquare.VertexSize;
			
			GlobalSquare.IndexData = SquareIndices;
			GlobalSquare.IndexSize = sizeof(UINT);
			GlobalSquare.IndexCount = 6;
			GlobalSquare.IndexDataSize = GlobalSquare.IndexSize * GlobalSquare.IndexCount;
			
			GlobalVertexBufferArray[0] = Win32CreateVertexBuffer(GlobalDevice, GlobalSquare.VertexData, GlobalSquare.VertexDataSize);
			D3D11_BUFFER_DESC VBDesc;
			GlobalVertexBufferArray[0]->GetDesc(&VBDesc);
			{
			D3D11_BUFFER_DESC IndexBufferDesc;
			IndexBufferDesc.ByteWidth = GlobalSquare.IndexDataSize;
			IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			IndexBufferDesc.CPUAccessFlags = 0;
			IndexBufferDesc.MiscFlags = 0;
			IndexBufferDesc.StructureByteStride = 0;
			
			D3D11_SUBRESOURCE_DATA IndexSubresourceData;
			IndexSubresourceData.pSysMem = GlobalSquare.IndexData;
			IndexSubresourceData.SysMemPitch = 0;
			IndexSubresourceData.SysMemSlicePitch = 0;
			
			res = GlobalDevice->CreateBuffer(&IndexBufferDesc,&IndexSubresourceData,&GlobalIndexBufferArray[0]);
			ASSERT(res == S_OK && GlobalIndexBufferArray[0]);
			}
			
			
			//Hexagon Object
			float HexagonVertices[]{
				-0.50f, 0.00f, 0.0f, //0
				-0.25f,-0.50f, 0.0f, //1
				 0.25f,-0.50f, 0.0f, //2
				 0.50f, 0.00f, 0.0f, //3
				 0.25f, 0.50f, 0.0f, //4
				-0.25f, 0.50f, 0.0f, //5
			};
			UINT HexagonIndices[]{
				0,5,1,
				5,4,1,
				1,4,2,
				4,3,2,
			};
			
			
			GlobalHexagon.VertexData = HexagonVertices;
			GlobalHexagon.VertexSize = sizeof(float) * 3;
			GlobalHexagon.VertexCount = 6;
			GlobalHexagon.VertexDataSize = GlobalHexagon.VertexCount * GlobalHexagon.VertexSize;
			GlobalHexagon.IndexData = HexagonIndices;
			GlobalHexagon.IndexSize = sizeof(UINT);
			GlobalHexagon.IndexCount = 12;
			GlobalHexagon.IndexDataSize = GlobalHexagon.IndexSize * GlobalHexagon.IndexCount;
			
			GlobalVertexBufferArray[1] = Win32CreateVertexBuffer(GlobalDevice, GlobalHexagon.VertexData, GlobalHexagon.VertexDataSize);
			
			{
			D3D11_BUFFER_DESC IndexBufferDesc;
			IndexBufferDesc.ByteWidth = GlobalHexagon.IndexDataSize;
			IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			IndexBufferDesc.CPUAccessFlags = 0;
			IndexBufferDesc.MiscFlags = 0;
			IndexBufferDesc.StructureByteStride = 0;
			
			D3D11_SUBRESOURCE_DATA IndexSubresourceData;
			IndexSubresourceData.pSysMem = GlobalHexagon.IndexData;
			IndexSubresourceData.SysMemPitch = 0;
			IndexSubresourceData.SysMemSlicePitch = 0;
			
			res = GlobalDevice->CreateBuffer(&IndexBufferDesc,&IndexSubresourceData,&GlobalIndexBufferArray[1]);
			ASSERT(res == S_OK && GlobalIndexBufferArray[1]);
			}
			
			//Adding PixelShaders
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShaderWhite.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShaderRed.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShaderGreen.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(GlobalPixelShaderArray,Win32CreatePixelShader(GlobalDevice,L"PixelShaderBlue.hlsl","PSEntry","ps_5_0"));
			
			//FrameBuffer
			if(GlobalSwapChain->GetBuffer(0,__uuidof(ID3D11Resource),(void**)&GlobalFrameBuffer)==S_OK){
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
			
			//HUllShader
			ID3DBlob *HSBlob = Win32CompileShaderFromFile(L"HullShader.hlsl","HSEntry","hs_5_0");
			ASSERT(HSBlob);
			void *CompiledHS = HSBlob->GetBufferPointer();
			SIZE_T CompiledHSSize = HSBlob->GetBufferSize();
			res = GlobalDevice->CreateHullShader(CompiledHS,CompiledHSSize,nullptr,&GlobalHullShader);
			ASSERT(res==S_OK);
			
			//DomainShader
			ID3DBlob *DSBlob = Win32CompileShaderFromFile(L"DomainShader.hlsl","DSEntry","ds_5_0");
			ASSERT(DSBlob);
			void *CompiledDS = DSBlob->GetBufferPointer();
			SIZE_T CompiledDSSize = DSBlob->GetBufferSize();
			res = GlobalDevice->CreateDomainShader(CompiledDS,CompiledDSSize,nullptr,&GlobalDomainShader);
			ASSERT(res==S_OK);
			
			//GeometryShader
			ID3DBlob *GSBlob = Win32CompileShaderFromFile(L"GeometryShader.hlsl","GSEntry","gs_5_0");
			ASSERT(GSBlob);
			void *CompiledGS = GSBlob->GetBufferPointer();
			SIZE_T CompiledGSSize = GSBlob->GetBufferSize();
			res = GlobalDevice->CreateGeometryShader(CompiledGS,CompiledGSSize,nullptr,&GlobalGeometryShader);
			ASSERT(res==S_OK);
			
			//Rasterizer
			D3D11_RASTERIZER_DESC RasterizerDesc;
			RasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
			RasterizerDesc.CullMode = D3D11_CULL_BACK;
			RasterizerDesc.FrontCounterClockwise = FALSE;
			RasterizerDesc.DepthBias = 0;
			RasterizerDesc.DepthBiasClamp = 1.0f;
			RasterizerDesc.SlopeScaledDepthBias = 0.0f;
			RasterizerDesc.DepthClipEnable = FALSE;
			RasterizerDesc.ScissorEnable = TRUE;
			RasterizerDesc.MultisampleEnable = FALSE;
			RasterizerDesc.AntialiasedLineEnable = FALSE;
			
			ID3D11RasterizerState *RasterizerState = NULL;
			res = GlobalDevice->CreateRasterizerState(&RasterizerDesc,&RasterizerState);
			ASSERT(RasterizerState);
			
			//ComputeShader
			ID3DBlob *CSBlob = Win32CompileShaderFromFile(L"ComputeShader.hlsl","CSEntry","cs_5_0");
			ASSERT(CSBlob);
			void *CompiledCS = CSBlob->GetBufferPointer();
			SIZE_T CompiledCSSize = CSBlob->GetBufferSize();
			res = GlobalDevice->CreateComputeShader(CompiledCS,CompiledCSSize,nullptr,&GlobalComputeShader);
			ASSERT(res==S_OK);
			
			UINT UAVBufferSize = MB(64);
			ID3D11Buffer *UAVBuffer = nullptr;
			D3D11_BUFFER_DESC UAVBufferDesc;
			UAVBufferDesc.ByteWidth = UAVBufferSize;
			UAVBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			UAVBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
			UAVBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
			UAVBufferDesc.MiscFlags = 0;
			UAVBufferDesc.StructureByteStride = 0;
			
			void *UAVData = VirtualAlloc(nullptr,UAVBufferSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
			ASSERT(UAVData);
			D3D11_SUBRESOURCE_DATA UAVSubresourceData;
			UAVSubresourceData.pSysMem = UAVData;
			UAVSubresourceData.SysMemPitch = 0;
			UAVSubresourceData.SysMemSlicePitch = 0;
			ASSERT(GlobalDevice->CreateBuffer(&UAVBufferDesc,&UAVSubresourceData,&UAVBuffer)==S_OK);
			
			
			D3D11_BUFFER_UAV UAVElementDesc;
			UAVElementDesc.FirstElement = 0;
			UAVElementDesc.NumElements = UAVBufferSize / 4;
			UAVElementDesc.Flags = 0;
			
			D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			UAVDesc.Format = DXGI_FORMAT_R16G16_UINT;
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			UAVDesc.Buffer = UAVElementDesc;

			ASSERT(GlobalDevice->CreateUnorderedAccessView(UAVBuffer,&UAVDesc,&GlobalUAV)==S_OK);
			
			
			
			//Initializing active shader
			GlobalActivePixelShader 	= GlobalPixelShaderArray[1];
			GlobalActiveVertexShader 	= GlobalVertexShader;
			GlobalActiveVertexBuffer 	= GlobalVertexBufferArray[0];
			GlobalActiveIndexBuffer 	= GlobalIndexBufferArray[0];
			GlobalActiveIndexCount 		= GlobalSquare.IndexCount;
			GlobalStrides[0] = {GlobalSquare.VertexSize};
			GlobalOffsets[0] = {};
			
			
			GlobalRunning =  true;
			
			//Loop
			while(GlobalRunning){
				
				MessageLoop(GlobalDevice);
				
				if(!GlobalRunning) break;
				
				ASSERT(GetClientRect(Window,&ClientRect));
				UINT NewWidth  = ClientRect.right - ClientRect.left;
				UINT NewHeight = ClientRect.bottom - ClientRect.top;
				
				if(Width != NewWidth || Height!= NewHeight){
					Width = NewWidth;
					Height = NewHeight;
					ResizeSwapChainBuffers(Width,Height);
					
				}
				
				static unsigned int AnimationCount = 1;
				
				static int AnimationIndex = 0;
				if(GlobalAnimationIsActive){
					AnimationCount = (AnimationCount+1)%(144*16);
					if(AnimationCount == 0){
						AnimationIndex = (AnimationIndex+1)%GlobalPixelShaderInArrayCount;
						GlobalActivePixelShader = GlobalPixelShaderArray[AnimationIndex];
					}
				}
				
				
				
				D3D11_VIEWPORT ViewPort;
				ViewPort.TopLeftX = 0.0f;
				ViewPort.TopLeftY = 0.0f;
				ViewPort.Width = (float)Width;
				ViewPort.Height = (float)Height;
				ViewPort.MinDepth = 0.0f;
				ViewPort.MaxDepth = 1.0f;
				const float RGBA[4] = {0,0,0,1};
				
				D3D11_RECT ScissorRect;
				ScissorRect.left  	= (LONG)(ViewPort.TopLeftX + (Width * 0.25f));
				ScissorRect.top   	= (LONG)(ViewPort.TopLeftY + (Height * 0.25f));
				ScissorRect.right 	= (LONG)(ViewPort.Width * 0.75f);
				ScissorRect.bottom  = (LONG)(ViewPort.Height * 0.75f);
				
				
				//
				GlobalDeviceContext->ClearRenderTargetView(GlobalRenderTargetView, RGBA);
				
				//Draw Full Square
				GlobalDeviceContext->RSSetViewports(1,&ViewPort);
				GlobalDeviceContext->RSSetState(NULL);
				GlobalDeviceContext->IASetVertexBuffers(0, 1, &GlobalActiveVertexBuffer, GlobalStrides, GlobalOffsets);
				GlobalDeviceContext->IASetIndexBuffer(GlobalActiveIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
				GlobalDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				GlobalDeviceContext->VSSetShader(GlobalActiveVertexShader, NULL, 0);
				GlobalDeviceContext->PSSetShader(GlobalPixelShaderArray[0], NULL, 0);
				GlobalDeviceContext->GSSetShader(nullptr,nullptr,0);
				GlobalDeviceContext->OMSetRenderTargets(1, &GlobalRenderTargetView, NULL);
				
				GlobalDeviceContext->DrawIndexed(GlobalActiveIndexCount, 0, 0);

				
				//Draw Geometry Lines
				GlobalDeviceContext->RSSetViewports(1,&ViewPort);
				GlobalDeviceContext->RSSetScissorRects(1,&ScissorRect);
				GlobalDeviceContext->RSSetState(RasterizerState);
				
				GlobalDeviceContext->IASetVertexBuffers(0,1,&GlobalActiveVertexBuffer, GlobalStrides, GlobalOffsets);
				GlobalDeviceContext->IASetIndexBuffer(GlobalActiveIndexBuffer,DXGI_FORMAT_R32_UINT,0);
				
				GlobalDeviceContext->VSSetShader(GlobalActiveVertexShader,NULL,0);
				GlobalDeviceContext->PSSetShader(GlobalActivePixelShader,NULL,0);
				GlobalDeviceContext->OMSetRenderTargets(1,&GlobalRenderTargetView,NULL);
#if 0		
				GlobalDeviceContext->HSSetShader(GlobalHullShader,nullptr,0);
				GlobalDeviceContext->DSSetShader(GlobalDomainShader,nullptr,0);
				GlobalDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
				
#else
				GlobalDeviceContext->GSSetShader(GlobalGeometryShader,nullptr,0);
				GlobalDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				
#endif	
				GlobalDeviceContext->DrawIndexed(GlobalActiveIndexCount, 0, 0);

				GlobalDeviceContext->CSSetUnorderedAccessViews(0,1,&GlobalUAV,nullptr);
				GlobalDeviceContext->CSSetShader(GlobalComputeShader,nullptr,0);
				GlobalDeviceContext->Dispatch(1,1,1);
				
				
				GlobalSwapChain->Present(0, 0);
				
			
				
			
				
				
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