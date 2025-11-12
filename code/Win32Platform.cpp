

#define ASSERT(x) if(!(x)) *(char*)0=0;

#define MAX_PIXEL_SHADER_COUNT 32
#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))



#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <windows.h>


#define global_variable static
typedef int bool32;

#include "Win32Platform.h"

global_variable bool32 GlobalRunning = false;
global_variable UINT PixelShaderInArrayCount = 0;

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

static void Win32ProcessError(DWORD Error){
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

static int Win32GetIDXGIInterfacesFromD3DDevice(ID3D11Device *Device, IDXGIDevice1 **IdxgiDevice,IDXGIAdapter **IdxgiAdapter,IDXGIFactory2 **IdxgiFactory ){
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

static IDXGISwapChain1* Win32GetSwapChain(ID3D11Device *Device, HWND Window,IDXGIFactory2 *IdxgiFactory){
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

static ID3DBlob *Win32CompileShaderFromFile(LPCWSTR Filename, LPCSTR Entrypoint, LPCSTR Target){
	ID3DBlob *BlobCode;
	ID3DBlob *BlobError;
	
	HRESULT res;
	if(!((res=D3DCompileFromFile(Filename, NULL, NULL, Entrypoint, Target, D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_DEBUG, 0, &BlobCode, &BlobError)) == S_OK)){
		if(BlobError){
			LPCSTR Buffer = (LPCSTR)BlobError->GetBufferPointer();
			OutputDebugStringA(Buffer);
		}
		return NULL;
	}
	
	ASSERT(BlobCode);
	return BlobCode;
}

static ID3D11Buffer * Win32CreateVertexBuffer(ID3D11Device *Device,void* VertexBufferData, UINT VertexBufferSize){
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

static ID3D11InputLayout* Win32CreateVertexInputLayout(ID3D11Device *Device, ID3D11DeviceContext *DeviceContext,void *CompiledVSShaderCode, size_t ShaderSize){
	D3D11_INPUT_ELEMENT_DESC VSInputElementDesc;
	VSInputElementDesc.SemanticName = "POSITION";
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

static ID3D11VertexShader* Win32CreateVertexShader(ID3D11Device *Device, void *CompiledShaderCode, size_t ShaderSize){
	ID3D11VertexShader *VertexShader = NULL;
	ASSERT(Device->CreateVertexShader(CompiledShaderCode,ShaderSize,NULL,&VertexShader)==S_OK);
	return VertexShader;
	
}
static ID3D11PixelShader* Win32CreatePixelShader(ID3D11Device *Device, LPCWSTR Filename, LPCSTR Entrypoint, LPCSTR Target){
	//Pixel Shader
	ID3DBlob *BlobPSCode = Win32CompileShaderFromFile(Filename,Entrypoint,Target);
	void *CompiledPSShaderCode = BlobPSCode->GetBufferPointer();
	size_t PSShaderSize = BlobPSCode->GetBufferSize();
	ASSERT(CompiledPSShaderCode);
	ID3D11PixelShader *PixelShader = NULL;
	ASSERT(Device->CreatePixelShader(CompiledPSShaderCode,PSShaderSize,NULL,&PixelShader)==S_OK);
	return PixelShader;
	
}
static int Win32AddPixelShaderToArray(ID3D11PixelShader** PixelShaderArray, ID3D11PixelShader* PixelShader){
	if(PixelShaderArray){
		if(PixelShaderInArrayCount < MAX_PIXEL_SHADER_COUNT){
			PixelShaderArray[PixelShaderInArrayCount] = PixelShader;
			PixelShaderInArrayCount++;
			return PixelShaderInArrayCount-1;
		}
	}
	return -1;
	
}


int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow){
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
	
	int Width = 1920;
	int Height = 1080;
	
	
	
	HRESULT res = 0;
	HWND Window = CreateWindowExA(0,WindowClass.lpszClassName,"Direct3D_",WS_OVERLAPPEDWINDOW|WS_VISIBLE, 0, 0, Width, Height,nullptr,nullptr,hInst,nullptr);
	if(Window){
		OutputDebugStringA("Window created\n");
		const D3D_FEATURE_LEVEL FeatureLevels[]={
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1};
		UINT FeatureLevelCount = sizeof(FeatureLevels)/sizeof(FeatureLevels[0]);
		
        ID3D11Device *Device;
		ID3D11DeviceContext *DeviceContext = NULL;
        ID3D11VertexShader *VertexShader = NULL;
		
		ID3D11Buffer* VertexBufferArray[32] = {};
		ID3D11Buffer* IndexBufferArray[64] = {};
		
		ID3D11PixelShader* PixelShaderArray[MAX_PIXEL_SHADER_COUNT];
		ID3D11HullShader *HullShader = NULL;
		ID3D11DomainShader *DomainShader = NULL;
		ID3D11GeometryShader *GeometryShader = NULL;
		
		
        IDXGISwapChain1 *SwapChain = NULL;
		ID3D11RenderTargetView *RenderTargetView = NULL;
        
        
        D3D_FEATURE_LEVEL CurrentFeatureLevel;
		
		if(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,D3D11_CREATE_DEVICE_DEBUG ,FeatureLevels,FeatureLevelCount,D3D11_SDK_VERSION,
                             &Device,&CurrentFeatureLevel,&DeviceContext)==S_OK){
			OutputDebugStringA("Device Created\n");
			
			
			//retrieve IDXGI Interfaces
			IDXGIDevice1 *IdxgiDevice = NULL;
			IDXGIAdapter *IdxgiAdapter = NULL;
			IDXGIFactory2 *IdxgiFactory = NULL;
			
			Win32GetIDXGIInterfacesFromD3DDevice(Device, &IdxgiDevice, &IdxgiAdapter, &IdxgiFactory);
			SwapChain = Win32GetSwapChain(Device,Window,IdxgiFactory);
			
			//Vertex Shader Compilation
			ID3DBlob *BlobVSCode = Win32CompileShaderFromFile(L"VertexShader.hlsl","VSEntry","vs_5_0");
			ASSERT(BlobVSCode);
			void *CompiledVSShaderCode = BlobVSCode->GetBufferPointer();
			size_t VSShaderSize = BlobVSCode->GetBufferSize();
			ASSERT(CompiledVSShaderCode);
			OutputDebugStringA("VertexShader compiled\n");
			
			//Vertex Shader 
			VertexShader = Win32CreateVertexShader(Device,CompiledVSShaderCode,VSShaderSize);
			ASSERT(VertexShader);
			OutputDebugStringA("Vertex Shader created\n");
			ID3D11InputLayout *VSInputLayout = Win32CreateVertexInputLayout(Device,DeviceContext,CompiledVSShaderCode,VSShaderSize);
			DeviceContext->IASetInputLayout(VSInputLayout);
			
			
			
			
			
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
			IndexedGeometryObject Square;
			Square.VertexData = SquareVertices;
			Square.VertexSize = sizeof(float) * 3;
			Square.VertexCount = 4;
			Square.VertexDataSize = Square.VertexCount * Square.VertexSize;
			
			Square.IndexData = SquareIndices;
			Square.IndexSize = sizeof(UINT);
			Square.IndexCount = 6;
			Square.IndexDataSize = Square.IndexSize * Square.IndexCount;
			
			

			VertexBufferArray[0] = Win32CreateVertexBuffer(Device, Square.VertexData, Square.VertexDataSize);
			D3D11_BUFFER_DESC VBDesc;
			VertexBufferArray[0]->GetDesc(&VBDesc);
			{
			D3D11_BUFFER_DESC IndexBufferDesc;
			IndexBufferDesc.ByteWidth = Square.IndexDataSize;
			IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			IndexBufferDesc.CPUAccessFlags = 0;
			IndexBufferDesc.MiscFlags = 0;
			IndexBufferDesc.StructureByteStride = 0;
			
			D3D11_SUBRESOURCE_DATA IndexSubresourceData;
			IndexSubresourceData.pSysMem = Square.IndexData;
			IndexSubresourceData.SysMemPitch = 0;
			IndexSubresourceData.SysMemSlicePitch = 0;
			
			res = Device->CreateBuffer(&IndexBufferDesc,&IndexSubresourceData,&IndexBufferArray[0]);
			ASSERT(res == S_OK && IndexBufferArray[0]);
			}
			
			
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
			
			IndexedGeometryObject Hexagon;
			Hexagon.VertexData = HexagonVertices;
			Hexagon.VertexSize = sizeof(float) * 3;
			Hexagon.VertexCount = 6;
			Hexagon.VertexDataSize = Hexagon.VertexCount * Hexagon.VertexSize;
			Hexagon.IndexData = HexagonIndices;
			Hexagon.IndexSize = sizeof(UINT);
			Hexagon.IndexCount = 12;
			Hexagon.IndexDataSize = Hexagon.IndexSize * Hexagon.IndexCount;
			
			VertexBufferArray[1] = Win32CreateVertexBuffer(Device, Hexagon.VertexData, Hexagon.VertexDataSize);
			
			{
			D3D11_BUFFER_DESC IndexBufferDesc;
			IndexBufferDesc.ByteWidth = Hexagon.IndexDataSize;
			IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			IndexBufferDesc.CPUAccessFlags = 0;
			IndexBufferDesc.MiscFlags = 0;
			IndexBufferDesc.StructureByteStride = 0;
			
			D3D11_SUBRESOURCE_DATA IndexSubresourceData;
			IndexSubresourceData.pSysMem = Hexagon.IndexData;
			IndexSubresourceData.SysMemPitch = 0;
			IndexSubresourceData.SysMemSlicePitch = 0;
			
			res = Device->CreateBuffer(&IndexBufferDesc,&IndexSubresourceData,&IndexBufferArray[1]);
			ASSERT(res == S_OK && IndexBufferArray[1]);
			}
			
			Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderWhite.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderRed.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderGreen.hlsl","PSEntry","ps_5_0"));
			Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderBlue.hlsl","PSEntry","ps_5_0"));
			
			//Resource
			ID3D11Resource *RenderTargetResource = NULL;
			if(SwapChain->GetBuffer(0,__uuidof(ID3D11Resource),(void**)&RenderTargetResource)==S_OK){
				ASSERT(RenderTargetResource);
				//RenderTargetView
				
				
				if(Device->CreateRenderTargetView(RenderTargetResource,NULL,&RenderTargetView)==S_OK){
					ASSERT(RenderTargetView);
					
					OutputDebugStringA("RenderTargetView created\n");
					
				}
				else{
					OutputDebugStringA("RenderTargetView failed\n");
				}
			}
			else{
				OutputDebugStringA("RenderTargetResource failed\n");
			}
			
			ID3DBlob *HSBlob = Win32CompileShaderFromFile(L"HullShader.hlsl","HSEntry","hs_5_0");
			ASSERT(HSBlob);
			void *CompiledHS = HSBlob->GetBufferPointer();
			SIZE_T CompiledHSSize = HSBlob->GetBufferSize();
			res = Device->CreateHullShader(CompiledHS,CompiledHSSize,nullptr,&HullShader);
			ASSERT(res==S_OK);
			
			ID3DBlob *DSBlob = Win32CompileShaderFromFile(L"DomainShader.hlsl","DSEntry","ds_5_0");
			ASSERT(DSBlob);
			void *CompiledDS = DSBlob->GetBufferPointer();
			SIZE_T CompiledDSSize = DSBlob->GetBufferSize();
			res = Device->CreateDomainShader(CompiledDS,CompiledDSSize,nullptr,&DomainShader);
			ASSERT(res==S_OK);
			
			ID3DBlob *GSBlob = Win32CompileShaderFromFile(L"GeometryShader.hlsl","GSEntry","gs_5_0");
			ASSERT(GSBlob);
			void *CompiledGS = GSBlob->GetBufferPointer();
			SIZE_T CompiledGSSize = GSBlob->GetBufferSize();
			res = Device->CreateGeometryShader(CompiledGS,CompiledGSSize,nullptr,&GeometryShader);
			ASSERT(res==S_OK);
			
			
			
			ID3D11PixelShader *ActivePixelShader = PixelShaderArray[0];
			ID3D11VertexShader *ActiveVertexShader = VertexShader;
			ID3D11Buffer *ActiveVertexBuffer = VertexBufferArray[0];
			ID3D11Buffer *ActiveIndexBuffer = IndexBufferArray[0];
			UINT ActiveIndexCount = Square.IndexCount;
			UINT Strides[1] = {Square.VertexSize};
			UINT Offsets[1] = {};
			
			bool32 AnimationIsActive = false;
			
			GlobalRunning =  true;
			while(GlobalRunning){
				
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					
					switch(Message.message){
						case WM_QUIT:{
							GlobalRunning = false;
						}break;
						
						case WM_KEYDOWN:{
							unsigned int VKCode = (unsigned int) Message.wParam;
							if(VKCode == '1'){
								if(PixelShaderInArrayCount >= 1 ) ActivePixelShader = PixelShaderArray[0];
								
							}
							else if(VKCode == '2'){
								if(PixelShaderInArrayCount >= 2 ) ActivePixelShader = PixelShaderArray[1];
							}
							else if(VKCode == '3'){
								if(PixelShaderInArrayCount >= 3 ) ActivePixelShader = PixelShaderArray[2];
							}
							else if(VKCode == '4'){
								if(PixelShaderInArrayCount >= 4 ) ActivePixelShader = PixelShaderArray[3];
							}
							else if(VKCode == '5'){
								if(PixelShaderInArrayCount >= 5 ) ActivePixelShader = PixelShaderArray[4];
							}
							else if(VKCode == '6'){
								if(PixelShaderInArrayCount >= 6 ) ActivePixelShader = PixelShaderArray[5];
							}
							else if(VKCode == VK_SPACE){
								AnimationIsActive ^= true;
							}
							else if(VKCode == 'R'){
								ASSERT(HullShader)
								HullShader->Release();
								HSBlob = Win32CompileShaderFromFile(L"HullShader.hlsl","HSEntry","hs_5_0");
								void* x = HSBlob->GetBufferPointer();
								ASSERT(HSBlob);
								CompiledHS = HSBlob->GetBufferPointer();
								CompiledHSSize = HSBlob->GetBufferSize();
								res = Device->CreateHullShader(CompiledHS,CompiledHSSize,nullptr,&HullShader);
								ASSERT(res==S_OK);
								
								ASSERT(DomainShader)
								DomainShader->Release();
								DSBlob = Win32CompileShaderFromFile(L"DomainShader.hlsl","DSEntry","ds_5_0");
								ASSERT(DSBlob);
								CompiledDS = DSBlob->GetBufferPointer();
								CompiledDSSize = DSBlob->GetBufferSize();
								res = Device->CreateDomainShader(CompiledDS,CompiledDSSize,nullptr,&DomainShader);
								ASSERT(res==S_OK);
							}
							
							else if(VKCode == 'H'){
								ActiveVertexBuffer = VertexBufferArray[1];
								ActiveIndexBuffer = IndexBufferArray[1];
								ActiveIndexCount = Hexagon.IndexCount;
								Strides[0] = Hexagon.VertexSize;
								ActivePixelShader = PixelShaderArray[2];
							}
							else if(VKCode == 'S'){
								ActiveVertexBuffer = VertexBufferArray[0];
								ActiveIndexBuffer = IndexBufferArray[0];
								ActiveIndexCount = Square.IndexCount;
								Strides[0] = Square.VertexSize;
								ActivePixelShader = PixelShaderArray[0];
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
				
				if(!GlobalRunning) break;
				
				RECT ClientRect;
				ASSERT(GetClientRect(Window,&ClientRect));
				Width = ClientRect.right - ClientRect.left;
				Height = ClientRect.bottom - ClientRect.top;
				
				
				static unsigned int AnimationCount = 1;
				
				static int AnimationIndex = 0;
				if(AnimationIsActive){
					AnimationCount = (AnimationCount+1)%(144*16);
					if(AnimationCount == 0){
						AnimationIndex = (AnimationIndex+1)%PixelShaderInArrayCount;
						ActivePixelShader = PixelShaderArray[AnimationIndex];
					}
				}
				
				
				//ViewPort
				D3D11_VIEWPORT ViewPort;
				ViewPort.TopLeftX = 0.0f;
				ViewPort.TopLeftY = 0.0f;
				ViewPort.Width = (float)Width;
				ViewPort.Height = (float)Height;
				ViewPort.MinDepth = 0.0f;
				ViewPort.MaxDepth = 1.0f;
				const float RGBA[4] = {0,0,0,1};
				
				DeviceContext->ClearRenderTargetView(RenderTargetView, RGBA);
				DeviceContext->RSSetViewports(1,&ViewPort);
				DeviceContext->IASetVertexBuffers(0,1,&ActiveVertexBuffer,Strides,Offsets);
				DeviceContext->IASetIndexBuffer(ActiveIndexBuffer,DXGI_FORMAT_R32_UINT,0);
				DeviceContext->VSSetShader(ActiveVertexShader,NULL,0);
				DeviceContext->PSSetShader(ActivePixelShader,NULL,0);
				DeviceContext->OMSetRenderTargets(1,&RenderTargetView,NULL);
#if 0		
				DeviceContext->HSSetShader(HullShader,nullptr,0);
				DeviceContext->DSSetShader(DomainShader,nullptr,0);
				DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
				DeviceContext->DrawIndexed(ActiveIndexCount,0,0);
#else
				DeviceContext->GSSetShader(GeometryShader,nullptr,0);
				DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				DeviceContext->DrawIndexed(ActiveIndexCount,0,0);
#endif	
		
				
				
				
				SwapChain->Present(0, 0);
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