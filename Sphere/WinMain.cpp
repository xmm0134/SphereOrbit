#include<windows.h>
#include<d3d11.h>
#include<d3dcompiler.h>
#include<DirectXMath.h>
#include<iostream>
#include<vector>
#include"D3DApp.h"
#include"imgui.h"
#include"imgui_impl_dx11.h"
#include"imgui_impl_win32.h"
#include"UI.h"
#include"Timer.h"
#include<memory>

ID3D11Device* Device;
ID3D11DeviceContext* Devcon;
IDXGISwapChain* SwapChain;
ID3D11RenderTargetView* RenderTarget;
ID3D11RasterizerState* RasterState;
ID3D11DepthStencilState* DepthStencilState;
ID3D11DepthStencilView* DepthStencilView;
ID3D11Texture2D* DepthStencilBuffer;
ID3D11PixelShader* Pixelshader;
ID3D11VertexShader* VertexShader;
ID3D11InputLayout* Inputlayout;

ID3D11Buffer* LightingCBuffer;
ID3D11Buffer* ConstBuffer;
ID3D11Buffer* SphereMaterial;
ID3D11Buffer* SphereVertexBuffer;
ID3D11Buffer* SphereIndexBuffer;

ID3D11Buffer* ElectronMaterial;
ID3D11Buffer* ElectronVertexBuffer;
ID3D11Buffer* ElectronIndexBuffer;
FLOAT bg[] = {0.0f, 0.0f, 0.0f, 0.0f};
float Theta = 0.0f;
float Phi = 1.5f;

#ifdef DEBUG
UINT Windowed = true;
UINT Debug = D3D11_CREATE_DEVICE_DEBUG;
UINT ShaderDebug = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

#endif // DEBUG

#ifndef DEBUG
UINT Windowed = false;
UINT Debug = NULL;
UINT ShaderDebug = NULL;

#endif // !DEBUG



XMFLOAT4X4 gViewProj;
std::unique_ptr<Timer> gtime;

#define ERadius 1.0f


using namespace DirectX;

struct VERTEX
{
	DirectX::XMFLOAT4 Position;
	DirectX::XMFLOAT3 Normal;
};

struct tMaterial
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;
	XMFLOAT4 Padding;
};

struct tLight 
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;
	XMFLOAT3 LightDir;
	XMFLOAT3 CameraPos;
	XMFLOAT2 Padding;
};

struct tPointLight
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;
	XMFLOAT3 Position;
	XMFLOAT3 Attenuation;
	float Range;
	float Padding;
};

UINT Offset = 0;
UINT Stride = sizeof(VERTEX);
std::vector<VERTEX> verticies;
std::vector<UINT> Indicies;

std::vector<VERTEX> Electronvertices;
std::vector<UINT> ElectronIndicies;

struct cbuffer
{
	XMFLOAT4X4 ViewProj;
	XMFLOAT4X4 World;
};

inline float Clamp(float value, float minVal, float maxVal)
{
	if (value < minVal) return minVal;
	if (value > maxVal) return maxVal;
	return value;
}

bool InitSphere(float Radius, int StackCount, int SectorCount)
{
	float StackStep = XM_PI / StackCount;
	float SectorStep = XM_2PI / SectorCount;
	float du = 1.0f / SectorCount;
	float dv = 1.0f / StackCount;

	// Top pole vertex
	verticies.emplace_back(XMFLOAT4(0.0f, Radius, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	// Middle vertices
	for (int i = 1; i < StackCount; ++i)
	{
		float stackAngle = i * StackStep;
		float y = Radius * cosf(stackAngle);
		float xy = Radius * sinf(stackAngle);
		float v = (i - 1) * dv;
		for (int j = 0; j < SectorCount; ++j)
		{
			float sectorAngle = j * SectorStep;
			float x = xy * cosf(sectorAngle);
			float z = xy * sinf(sectorAngle);
			float u = j * du;

			XMVECTOR f = XMVectorSet(x, y, z, 0.0f);
			XMVECTOR norm = XMVector3Normalize(f);

			XMFLOAT3 Normal;

			XMStoreFloat3(&Normal, norm);

			verticies.emplace_back(XMFLOAT4(x, y, z, 1.0f), Normal);
		}
	}

	// Bottom pole vertex
	verticies.emplace_back(XMFLOAT4(0.0f, -Radius, 0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));

	// Top cap indices
	for (int j = 0; j < SectorCount; ++j)
	{
		int next = (j + 1) % SectorCount;
		Indicies.emplace_back(0);
		Indicies.emplace_back(1 + j);
		Indicies.emplace_back(1 + next);
	}

	// Middle stacks indices
	for (int i = 0; i < StackCount - 2; ++i)
	{
		for (int j = 0; j < SectorCount; ++j)
		{
			int current = i * SectorCount + j + 1;
			int next = i * SectorCount + (j + 1) % SectorCount + 1;
			int below = (i + 1) * SectorCount + j + 1;
			int belowNext = (i + 1) * SectorCount + (j + 1) % SectorCount + 1;

			Indicies.emplace_back(current);
			Indicies.emplace_back(below);
			Indicies.emplace_back(next);

			Indicies.emplace_back(next);
			Indicies.emplace_back(below);
			Indicies.emplace_back(belowNext);
		}
	}

	// Bottom cap indices
	int southPoleIndex = (int)verticies.size() - 1;
	int baseIndex = 1 + (StackCount - 2) * SectorCount;

	for (int j = 0; j < SectorCount; ++j)
	{
		int next = (j + 1) % SectorCount;
		Indicies.emplace_back(southPoleIndex);
		Indicies.emplace_back(baseIndex + next);
		Indicies.emplace_back(baseIndex + j);
	}

	D3D11_BUFFER_DESC VBD;
	ZeroMemory(&VBD, sizeof(VBD));
	VBD.ByteWidth = verticies.size() * sizeof(VERTEX);
	VBD.Usage = D3D11_USAGE_DEFAULT;
	VBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA Vdata;
	ZeroMemory(&Vdata, sizeof(Vdata));
	Vdata.pSysMem = verticies.data();

	D3D11_BUFFER_DESC IBD;
	ZeroMemory(&IBD, sizeof(IBD));
	IBD.ByteWidth = Indicies.size() * sizeof(UINT);
	IBD.Usage = D3D11_USAGE_DEFAULT;
	IBD.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA Idata;
	ZeroMemory(&Idata, sizeof(Idata));
	Idata.pSysMem = Indicies.data();
	
	HRESULT hr = Device->CreateBuffer(&VBD, &Vdata, &SphereVertexBuffer);
	if (FAILED(hr)) 
	{
		MessageBoxA(nullptr, "Failed to create sphere vertex buffer", "Error", MB_OKCANCEL);
		return false;
	}
	hr = Device->CreateBuffer(&IBD, &Idata, &SphereIndexBuffer);
	if (FAILED(hr))
	{
		MessageBoxA(nullptr, "Failed to create sphere index buffer", "Error", MB_OKCANCEL);
		return false;
	}

	tMaterial SphereMat;
	SphereMat.Ambient = XMFLOAT4(0.0f, 0.0f, 0.3f, 1.0f);
	SphereMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.6f, 1.0f);
	SphereMat.Specular = XMFLOAT4(0.0f, 0.0f, 1.0f, 42.0f);
	SphereMat.Padding = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	
	D3D11_SUBRESOURCE_DATA Matdata;
	ZeroMemory(&Matdata, sizeof(Matdata));
	Matdata.pSysMem = &SphereMat;

	D3D11_BUFFER_DESC MBD;
	ZeroMemory(&MBD, sizeof(MBD));
	MBD.ByteWidth = sizeof(tMaterial);
	MBD.Usage = D3D11_USAGE_DEFAULT;
	MBD.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = Device->CreateBuffer(&MBD, &Matdata, &SphereMaterial);
	if (FAILED(hr))
	{
		MessageBoxA(nullptr, "Failed to create sphere material buffer", "Error", MB_OKCANCEL);
		return false;
	}

	return true;
}

bool InitElectron(float Radius, int StackCount, int SectorCount) 
{
	float StackStep = XM_PI / StackCount;
	float SectorStep = XM_2PI / SectorCount;
	float du = 1.0f / SectorCount;
	float dv = 1.0f / StackCount;

	 Electronvertices.emplace_back(XMFLOAT4(0.0f, Radius, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	
	for (int i = 1; i < StackCount; ++i)
	{
		float stackAngle = i * StackStep;
		float y = Radius * cosf(stackAngle);
		float xy = Radius * sinf(stackAngle);
		float v = (i - 1) * dv;
		for (int j = 0; j < SectorCount; ++j)
		{
			float sectorAngle = j * SectorStep;
			float x = xy * cosf(sectorAngle);
			float z = xy * sinf(sectorAngle);
			float u = j * du;

			XMVECTOR f = XMVectorSet(x, y, z, 0.0f);
			XMVECTOR norm = XMVector3Normalize(f);

			XMFLOAT3 Normal;

			XMStoreFloat3(&Normal, norm);

			Electronvertices.emplace_back(XMFLOAT4(x, y, z, 1.0f), Normal);
		}
	}

	
	Electronvertices.emplace_back(XMFLOAT4(0.0f, -Radius, 0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));

	
	for (int j = 0; j < SectorCount; ++j)
	{
		int next = (j + 1) % SectorCount;
		ElectronIndicies.emplace_back(0);
		ElectronIndicies.emplace_back(1 + j);
		ElectronIndicies.emplace_back(1 + next);
	}

	
	for (int i = 0; i < StackCount - 2; ++i)
	{
		for (int j = 0; j < SectorCount; ++j)
		{
			int current = i * SectorCount + j + 1;
			int next = i * SectorCount + (j + 1) % SectorCount + 1;
			int below = (i + 1) * SectorCount + j + 1;
			int belowNext = (i + 1) * SectorCount + (j + 1) % SectorCount + 1;

			ElectronIndicies.emplace_back(current);
			ElectronIndicies.emplace_back(below);
			ElectronIndicies.emplace_back(next);

			ElectronIndicies.emplace_back(next);
			ElectronIndicies.emplace_back(below);
			ElectronIndicies.emplace_back(belowNext);
		}
	}

	
	int southPoleIndex = (int)verticies.size() - 1;
	int baseIndex = 1 + (StackCount - 2) * SectorCount;

	for (int j = 0; j < SectorCount; ++j)
	{
		int next = (j + 1) % SectorCount;
		ElectronIndicies.emplace_back(southPoleIndex);
		ElectronIndicies.emplace_back(baseIndex + next);
		ElectronIndicies.emplace_back(baseIndex + j);
	}

	D3D11_BUFFER_DESC VBD;
	ZeroMemory(&VBD, sizeof(D3D11_BUFFER_DESC));
	VBD.ByteWidth = Electronvertices.size() * sizeof(VERTEX);
	VBD.Usage = D3D11_USAGE_DEFAULT;
	VBD.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA Vdata;
	ZeroMemory(&Vdata, sizeof(D3D11_SUBRESOURCE_DATA));
	Vdata.pSysMem = Electronvertices.data();

	HRESULT hr = Device->CreateBuffer(&VBD, &Vdata, &ElectronVertexBuffer);
	if (FAILED(hr)) 
	{
		MessageBox(nullptr, "Failed to create vertex buffer!", "Error", MB_OKCANCEL);
		return false;
	}

	D3D11_BUFFER_DESC IBD;
	ZeroMemory(&IBD, sizeof(IBD));
	IBD.ByteWidth = ElectronIndicies.size() * sizeof(UINT);
	IBD.Usage = D3D11_USAGE_DEFAULT;
	IBD.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA Idata;
	ZeroMemory(&Idata, sizeof(Idata));
	Idata.pSysMem = ElectronIndicies.data();

	hr = Device->CreateBuffer(&IBD, &Idata, &ElectronIndexBuffer);
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create index buffer!", "Error", MB_OKCANCEL);
		return false;
	}

	tMaterial mat = {};
	mat.Ambient = XMFLOAT4(0.0f, 0.4f, 0.0f, 1.0f);
	mat.Diffuse = XMFLOAT4(0.0f, 0.6f, 0.0f, 1.0f);
	mat.Specular = XMFLOAT4(0.0f, 0.8f, 0.0f, 42.0f);

	D3D11_BUFFER_DESC Emat;
	ZeroMemory(&Emat, sizeof(Emat));
	Emat.ByteWidth = sizeof(tMaterial);
	Emat.Usage = D3D11_USAGE_DEFAULT;
	Emat.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA Ematdata;
	ZeroMemory(&Ematdata, sizeof(Ematdata));
	Ematdata.pSysMem = &mat;

	hr = Device->CreateBuffer(&Emat, &Ematdata, &ElectronMaterial);

	if (FAILED(hr)) 
	{
		MessageBox(nullptr, "Failed to create material buffer!", "Error", MB_OKCANCEL);
		return false;
	}


	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create point light buffer!", "Error", MB_OKCANCEL);
		return false;
	}
	
	return true;
}

bool InitShaders() 
{
	D3D11_INPUT_ELEMENT_DESC Desc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	ID3DBlob* VertexShaderByteCode;
	ID3DBlob* PixelShaderByteCode;
	HRESULT hr = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", ShaderDebug, NULL, &VertexShaderByteCode, nullptr);
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to load vertex shader!", "Error", MB_OKCANCEL);
		return false;
	}

	hr = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", ShaderDebug, NULL, &PixelShaderByteCode, nullptr);
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to load pixel shader!", "Error", MB_OKCANCEL);
		return false;
	}
	
	Device->CreateVertexShader(VertexShaderByteCode->GetBufferPointer(), VertexShaderByteCode->GetBufferSize(), NULL, &VertexShader);
	Device->CreatePixelShader(PixelShaderByteCode->GetBufferPointer(), PixelShaderByteCode->GetBufferSize(), NULL, &Pixelshader);

	Device->CreateInputLayout(Desc, 2, VertexShaderByteCode->GetBufferPointer(), VertexShaderByteCode->GetBufferSize(), &Inputlayout);

	Devcon->PSSetShader(Pixelshader, NULL, NULL);
	Devcon->VSSetShader(VertexShader, NULL, NULL);

	return true;
}

bool InitUI(ID3D11Device* Device, ID3D11DeviceContext* Devcon, HWND hwnd)
{
	ImGui::CreateContext();
	ImGuiIO& IO = ImGui::GetIO();
	ImGui_ImplDX11_Init(Device, Devcon);
	ImGui_ImplWin32_Init(hwnd);
	ImGui::StyleColorsDark();
	return true;
}

bool InitD3D(HWND hwnd) 
{
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = 1;
	
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Windowed = Windowed;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 4; 
	desc.BufferDesc.Width = 1920;
	desc.BufferDesc.Height = 1080;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.OutputWindow = hwnd;
	
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, Debug, NULL, NULL, D3D11_SDK_VERSION, &desc , &SwapChain, &Device, NULL, &Devcon);

	if (FAILED(hr)) 
	{
		MessageBox(nullptr, "Failed to init d3d11", "Error", MB_OKCANCEL);
		return false;
	}

	ID3D11Texture2D* BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to get backbuffer!", "Error", MB_OKCANCEL);
		return false;
	}

	hr = Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTarget);

	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create render target!", "Error", MB_OKCANCEL);
		return false;
	}

	D3D11_TEXTURE2D_DESC DepthStencilDesc;
	ZeroMemory(&DepthStencilDesc, sizeof(DepthStencilDesc));
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilDesc.Height = 1080;
	DepthStencilDesc.Width = 1920;
	DepthStencilDesc.SampleDesc.Count = 4;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;

	hr = Device->CreateTexture2D(&DepthStencilDesc, nullptr, &DepthStencilBuffer);

	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create DepthStencilBuffer!", "Error", MB_OKCANCEL);
		return false;
	}
	
	hr = Device->CreateDepthStencilView(DepthStencilBuffer, NULL, &DepthStencilView);

	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create DepthStencilView!", "Error", MB_OKCANCEL);
		return false;
	}

	Devcon->OMSetRenderTargets(1, &RenderTarget, DepthStencilView);

	D3D11_DEPTH_STENCIL_DESC DSD;
	ZeroMemory(&DSD, sizeof(DSD));
	DSD.DepthEnable = true;
	DSD.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	hr = Device->CreateDepthStencilState(&DSD, &DepthStencilState);
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create DepthStencilState!", "Error", MB_OKCANCEL);
		return false;
	}
	Devcon->OMSetDepthStencilState(DepthStencilState, 1);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = 1920;
	vp.Height = 1080;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	Devcon->RSSetViewports(1, &vp);

	D3D11_RASTERIZER_DESC RSD;
	ZeroMemory(&RSD, sizeof(RSD));
	RSD.CullMode = D3D11_CULL_BACK;
	RSD.FillMode = D3D11_FILL_SOLID;
	RSD.AntialiasedLineEnable = TRUE;
	RSD.MultisampleEnable = TRUE;

	hr = Device->CreateRasterizerState(&RSD, &RasterState);
	if (FAILED(hr))
	{
		MessageBox(nullptr, "Failed to create RasterState!", "Error", MB_OKCANCEL);
		return false;
	}

	Devcon->RSSetState(RasterState);

	InitUI(Device, Devcon, hwnd);

	return true;
}

void InitGlobals() 
{
	XMMATRIX ViewMatrix = DirectX::XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -4.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	XMMATRIX ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(XM_PIDIV4, 1920.0f / 1080.0f, 0.1f, 1000.f);
	XMMATRIX ViewProj = ViewMatrix * ProjectionMatrix;
	XMStoreFloat4x4(&gViewProj, ViewProj);

	cbuffer SceneGlobals = {};
	XMStoreFloat4x4(&SceneGlobals.ViewProj, ViewProj);
	XMStoreFloat4x4(&SceneGlobals.World, XMMatrixIdentity());

	D3D11_BUFFER_DESC GB;
	ZeroMemory(&GB, sizeof(GB));
	GB.ByteWidth = sizeof(cbuffer);
	GB.Usage = D3D11_USAGE_DYNAMIC;
	GB.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	GB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA Gdata;
	ZeroMemory(&Gdata, sizeof(Gdata));
	Gdata.pSysMem = &SceneGlobals;

	HRESULT hr  = Device->CreateBuffer(&GB, &Gdata, &ConstBuffer);

	tLight Lighting = {};
	Lighting.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	Lighting.Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	Lighting.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Lighting.LightDir = XMFLOAT3(0.6, -1.0f, 0.0f);
	Lighting.CameraPos = XMFLOAT3(0.0f, 0.0f, -4.0f);

	D3D11_BUFFER_DESC LD;
	ZeroMemory(&LD, sizeof(LD));
	LD.ByteWidth = sizeof(tLight);
	LD.Usage = D3D11_USAGE_DYNAMIC;
	LD.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	LD.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA Ldata;
	ZeroMemory(&Ldata, sizeof(Ldata));
	Ldata.pSysMem = &Lighting;

	if (FAILED(hr)) 
	{
		MessageBox(nullptr, "Could not create constant buffer!", "Error", MB_OKCANCEL);
		return;
	}

	hr = Device->CreateBuffer(&LD, &Ldata, &LightingCBuffer);

	if (FAILED(hr)) 
	{
		MessageBox(nullptr, "Could not create lighting constant buffer!", "Error", MB_OKCANCEL);
		return;
	}

	Devcon->VSSetConstantBuffers(0, 1, &ConstBuffer);
	Devcon->PSSetConstantBuffers(0, 1, &LightingCBuffer);

}

void Run() 
{
	gtime->Tick();
	Devcon->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	Devcon->ClearRenderTargetView(RenderTarget, bg);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);

	ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

	ImGui::DragFloat("LightX", &UIVars::LightX, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("LightY", &UIVars::LightY, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("LightZ", &UIVars::LightZ, 0.01f, -1.0f, 1.0f);

	ImGui::DragFloat("Light Ambient", &UIVars::LightAmbient, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Light Diffuse", &UIVars::LightDiffuse, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Light Specular", &UIVars::LightSpecular, 0.01f, 0.0f, 1.0f);

	ImGui::DragFloat("Horizontal Speed", &UIVars::ThetaSpeed, 0.1f, 0.001f, 20.0f);

	ImGui::End();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	D3D11_MAPPED_SUBRESOURCE map;

	HRESULT hr = Devcon->Map(LightingCBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &map);

	tLight light = {};
	light.Ambient = XMFLOAT4(UIVars::LightAmbient, UIVars::LightAmbient, UIVars::LightAmbient, 1.0f);
	light.Diffuse = XMFLOAT4(UIVars::LightDiffuse, UIVars::LightDiffuse, UIVars::LightDiffuse, 1.0f);
	light.Specular = XMFLOAT4(UIVars::LightSpecular, UIVars::LightSpecular, UIVars::LightSpecular, 1.0f);
	light.CameraPos = XMFLOAT3(0.0f, 0.0f, -4.0f);
	light.LightDir = XMFLOAT3(UIVars::LightX, UIVars::LightY, UIVars::LightZ);

	if (SUCCEEDED(hr)) 
	{
		memcpy(map.pData, &light, sizeof(tLight));
		Devcon->Unmap(LightingCBuffer, NULL);
	}
	D3D11_MAPPED_SUBRESOURCE smap = {};
	cbuffer gSphereMatrices;
	gSphereMatrices.ViewProj = gViewProj;
	XMStoreFloat4x4(&gSphereMatrices.World, XMMatrixIdentity());

	hr = Devcon->Map(ConstBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &smap);

	if (SUCCEEDED(hr)) 
	{
		memcpy(smap.pData,&gSphereMatrices, sizeof(cbuffer));
		Devcon->Unmap(ConstBuffer, NULL);
	}

	
	Theta += gtime->GetDeltaTime() * UIVars::ThetaSpeed;

	XMFLOAT3 OrbitPos;
	OrbitPos.x = ERadius * sinf(Phi) * cosf(Theta);
	OrbitPos.y = ERadius * cos(Phi);
	OrbitPos.z = ERadius * sinf(Phi) * sinf(Theta);

	Devcon->PSSetConstantBuffers(1, 1, &SphereMaterial);
	Devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Devcon->IASetIndexBuffer(SphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Devcon->IASetVertexBuffers(0, 1, &SphereVertexBuffer, &Stride, &Offset);
	Devcon->IASetInputLayout(Inputlayout);
	Devcon->DrawIndexed(Indicies.size(), 0, 0);

	XMMATRIX WorldMatrix = XMMatrixTranslation(OrbitPos.x, OrbitPos.y, OrbitPos.z);
	cbuffer globals = {};
	XMStoreFloat4x4(&globals.World, WorldMatrix);
	globals.ViewProj = gViewProj;
	D3D11_MAPPED_SUBRESOURCE cmap = {};



	hr = Devcon->Map(ConstBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &cmap);

	if (SUCCEEDED(hr)) 
	{
		memcpy(cmap.pData, &globals, sizeof(cbuffer));
		Devcon->Unmap(ConstBuffer, NULL);
	}

	Devcon->PSSetConstantBuffers(1, 1, &ElectronMaterial);
	Devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Devcon->IASetIndexBuffer(ElectronIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Devcon->IASetVertexBuffers(0, 1, &ElectronVertexBuffer, &Stride, &Offset);
	Devcon->DrawIndexed(Indicies.size(), 0, 0);

	SwapChain->Present(1, NULL);
}


BOOL WINAPI WinMain(HINSTANCE hinst, HINSTANCE prevhinst, LPSTR lpcmdl, int showcmd) 
{
	D3DApp Application(hinst, "Sphere", 1920, 1080, true);
	HWND hwnd = Application.GetHWND();
	InitD3D(hwnd);
	InitShaders();
	InitGlobals();
	InitSphere(0.6f, 80, 80);
	InitElectron(0.05f, 80, 80);
	gtime = std::make_unique<Timer>();
	gtime->Reset();

	while (true) 
	{
		Application.PollWindowEvents();
		Application.Run();

		Run();

		Sleep(1);
	}

}