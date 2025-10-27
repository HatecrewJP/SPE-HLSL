
#define ASSERT(x) if(!(x)) *(char*)0=0;

#define TRIANGLE 0
#define MAX_PIXEL_SHADER_COUNT 32


#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <windows.h>


#define global_variable static
typedef int bool32;



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
	SwapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
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
	
	if(!D3DCompileFromFile(Filename,NULL,NULL,Entrypoint,Target,D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,0,&BlobCode,&BlobError)==S_OK) return NULL;
	
	ASSERT(BlobCode);
	return BlobCode;
}

static ID3D11Buffer * Win32CreateVertexBuffer(ID3D11Device *Device,void* VertexBufferData, UINT VertexBufferSize, UINT VertexSize){
	D3D11_BUFFER_DESC VertexBufferDesc;
	VertexBufferDesc.ByteWidth = VertexBufferSize;
	VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = VertexSize;
	
	D3D11_SUBRESOURCE_DATA SubresourceData;
	SubresourceData.pSysMem = VertexBufferData;
	SubresourceData.SysMemPitch = 0;
	SubresourceData.SysMemSlicePitch = 0;
	
	ID3D11Buffer *VertexBuffer;
	ASSERT(Device->CreateBuffer(&VertexBufferDesc,&SubresourceData,&VertexBuffer)==S_OK);
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
	HWND Window = CreateWindowExA(NULL,WindowClass.lpszClassName,"Direct3D_",WS_OVERLAPPEDWINDOW|WS_VISIBLE, 0, 0, Width, Height,NULL,NULL,hInst,NULL);
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
		
        UINT VertexCount = 0;
        const UINT VertexSize = 3*sizeof(float);
        ID3D11Device *Device;
		ID3D11DeviceContext *DeviceContext = NULL;
		ID3D11Buffer *VertexBuffer = NULL;
        ID3D11VertexShader *VertexShader = NULL;
		
		
		ID3D11PixelShader* PixelShaderArray[MAX_PIXEL_SHADER_COUNT];
		
        IDXGISwapChain1 *SwapChain = NULL;
		ID3D11RenderTargetView *RenderTargetView = NULL;
        UINT Strides[1] = {VertexSize};
        UINT Offsets[1] = {0};
        
        D3D_FEATURE_LEVEL CurrentFeatureLevel;
		
		if(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,D3D11_CREATE_DEVICE_DEBUG ,FeatureLevels,FeatureLevelCount,D3D11_SDK_VERSION,
                             &Device,&CurrentFeatureLevel,&DeviceContext)==S_OK){
			OutputDebugStringA("Device Created");
			
			
			//retrieve IDXGI Interfaces
			IDXGIDevice1 *IdxgiDevice = NULL;
			IDXGIAdapter *IdxgiAdapter = NULL;
			IDXGIFactory2 *IdxgiFactory = NULL;
			
			Win32GetIDXGIInterfacesFromD3DDevice(Device, &IdxgiDevice, &IdxgiAdapter, &IdxgiFactory);
			SwapChain = Win32GetSwapChain(Device,Window,IdxgiFactory);
			
			//Vertex Shader Compilation
			ID3DBlob *BlobVSCode = Win32CompileShaderFromFile(L"VertexShader.hlsl","VSEntry","vs_5_0");
			
			void *CompiledVSShaderCode = BlobVSCode->GetBufferPointer();
			size_t VSShaderSize = BlobVSCode->GetBufferSize();
			ASSERT(CompiledVSShaderCode);
			OutputDebugStringA("VertexShader compiled");
			
			//Vertex Shader 
				VertexShader = Win32CreateVertexShader(Device,CompiledVSShaderCode,VSShaderSize);
				ASSERT(VertexShader);
				OutputDebugStringA("Vertex Shader created");
				ID3D11InputLayout *VSInputLayout = Win32CreateVertexInputLayout(Device,DeviceContext,CompiledVSShaderCode,VSShaderSize);
				DeviceContext->IASetInputLayout(VSInputLayout);
				
				float oVertexBufferData[] {
					0.25f,0.25,0.0f,
					0.0f,0.25f,0.0f,
					0.0f,0.0f,0.0f,
					0.25f,0.25,0.0f
				};
				VertexCount = sizeof(oVertexBufferData) / sizeof(oVertexBufferData[0]) / 3;
				UINT VertexBufferSize = VertexCount * VertexSize;
				VertexBuffer = Win32CreateVertexBuffer(Device, oVertexBufferData, VertexBufferSize, VertexSize);
				
				
				
				
				Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderWhite.hlsl","PSEntry","ps_5_0"));
				Win32AddPixelShaderToArray(PixelShaderArray,Win32CreatePixelShader(Device,L"PixelShaderRed.hlsl","PSEntry","ps_5_0"));
				
					//Resource
					ID3D11Resource *RenderTargetResource = NULL;
					if(SwapChain->GetBuffer(0,__uuidof(ID3D11Resource),(void**)&RenderTargetResource)==S_OK){
						ASSERT(RenderTargetResource);
						//RenderTargetView
						
						
						if(Device->CreateRenderTargetView(RenderTargetResource,NULL,&RenderTargetView)==S_OK){
							ASSERT(RenderTargetView);
							
							OutputDebugStringA("RenderTargetView created");
							
						}
						else{
							OutputDebugStringA("RenderTargetView failed");
						}
					}
					else{
						OutputDebugStringA("RenderTargetResource failed");
					}
					
				
				
			
		}
		else{
			
		}
		
		GlobalRunning =  true;
		ID3D11PixelShader *ActivePixelShader = PixelShaderArray[0];
		ID3D11VertexShader *ActiveVertexShader = VertexShader;
		
		while(GlobalRunning){
			
			MSG Message;
			while(PeekMessage(&Message,0,0,0,PM_REMOVE)){
				
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
			
			
            DeviceContext->IASetVertexBuffers(0,1,&VertexBuffer,Strides,Offsets);
            DeviceContext->VSSetShader(ActiveVertexShader,NULL,0);
			DeviceContext->PSSetShader(ActivePixelShader,NULL,0);
			DeviceContext->OMSetRenderTargets(1,&RenderTargetView,NULL);
			
#if TRIANGLE
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#else
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
#endif
           
            //ViewPort
			
            D3D11_VIEWPORT ViewPort;
            ViewPort.TopLeftX = 0.0f;
            ViewPort.TopLeftY = 0.0f;
            ViewPort.Width = (float)Width;
            ViewPort.Height = (float)Height;
            ViewPort.MinDepth = 0.0f;
            ViewPort.MaxDepth = 1.0f;
            DeviceContext->RSSetViewports(1,&ViewPort);
            DeviceContext->Draw(VertexCount, 0);
            SwapChain->Present(1, 0);
            
            
            
            
            
            
            
        }
	}
    else{
        Win32ProcessError(GetLastError());
    }
} 