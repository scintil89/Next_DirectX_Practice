#include <windows.h>
#include <time.h>

#include <dxgi.h>
#include <d3d11.h>
#include <d3dCompiler.h>
#include <d3dx11.h>
#include <dxerr.h>

#include <d3dx11effect.h>
#include <xnamath.h>

#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dx11.lib" )

HRESULT InitDevice();
HWND                    g_hWnd = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11Device*           g_pd3dDevice = NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

ID3D11InputLayout*      g_pVertexLayout = NULL;
ID3D11Buffer*			g_pVertexBuffer;
ID3D11Buffer*			g_pIndexBuffer;
ID3D11Buffer*			g_pConstantBuffer;

ID3D11VertexShader*     g_pVertexShader = NULL;
ID3D11PixelShader*      g_pPixelShader = NULL;

void CreateDepthStencilTexture();
ID3D11Texture2D*        g_pDepthStencil = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;

struct MyVertex
{
	XMFLOAT3 pos;
	XMFLOAT4 color;

	XMFLOAT2   tex;
};

struct ConstantBuffer
{
	XMMATRIX wvp;	
};

ID3D11ShaderResourceView*           	g_pTextureRV1 = NULL;	  // earth Texture
ID3D11ShaderResourceView*           	g_pTextureRV2 = NULL;	  // cloud Texture
ID3D11SamplerState*                 	g_pSamplerLinear = NULL;   // SampleState

HRESULT   LoadTexture()
{
	HRESULT   hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, 		// 텍스처를 생성한 D3D device			
		L"Texture/earth.bmp", 	// 텍스처 경로			
		NULL, 			// 추가 이미지 정보(밉맵 등..), 보통 NULL			
		NULL, 			// 자원 적재 스레드. 보통 NULL			
		&g_pTextureRV1, 	// 생성된 이미지 뷰(텍스처)
		NULL);			// 자원 적재 스레드가 NULL이면 NULL. 보통 NULL

	if (FAILED(hr))
	{
		return hr;
	}

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, 		// 텍스처를 생성한 D3D device			
		L"Texture/cloud.png", 	// 텍스처 경로			
		NULL, 			// 추가 이미지 정보(밉맵 등..), 보통 NULL			
		NULL, 			// 자원 적재 스레드. 보통 NULL			
		&g_pTextureRV2, 	// 생성된 이미지 뷰(텍스처)
		NULL);			// 자원 적재 스레드가 NULL이면 NULL. 보통 NULL

	if (FAILED(hr))
	{
		return hr;
	}

	D3D11_SAMPLER_DESC 	sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;   // 선형 보간 밉 레벨 필터링.
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;   // U좌표 Address Mode
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;   // V좌표 Address Mode
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;  // W좌표 Address Mode
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 샘플링 데이터 비교 안함
	sampDesc.MinLOD = 0;			// 최소 Mipmap Rang e
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;	// 최대 Mipmap Range

	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear); // SamplerState 생성
	if (FAILED(hr))
		return hr;

	return S_OK;
}


HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;  // 분자
	sd.BufferDesc.RefreshRate.Denominator = 1; // 분모
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 백버퍼 렌더링
	sd.OutputWindow = g_hWnd;	// 현재 윈도우
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		0, 
		D3D_DRIVER_TYPE_HARDWARE,
		0, 
		createDeviceFlags, 
		featureLevels, 
		numFeatureLevels,
		D3D11_SDK_VERSION, 
		&sd, 
		&g_pSwapChain, 
		&g_pd3dDevice, 
		&g_featureLevel, 
		&g_pImmediateContext);

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	CreateDepthStencilTexture();
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);	

	return S_OK;
}

ID3D11RasterizerState *g_pSolidRS;
ID3D11RasterizerState *g_pWireFrameRS;

void CreateRenderState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;	
	rasterizerDesc.FrontCounterClockwise = false;
	
	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pSolidRS);

	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FrontCounterClockwise = false;
	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pWireFrameRS);
}

void CreateDepthStencilTexture()
{
	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = 800;
	descDepth.Height = 600;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);	

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;	
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format; // = DXGI_FORMAT_D24_UNORM_S8_UINT
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Flags = 0;
	descDSV.Texture2D.MipSlice = 0;
	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);	
}

void CreateShader()
{
	ID3DBlob *pErrorBlob = NULL;

	ID3DBlob *pVSBlob = NULL;
	
	HRESULT hr = D3DX11CompileFromFile(L"MyShader.fx", 0, 0, "VS", "vs_5_0", 0,
		0, 0, &pVSBlob, &pErrorBlob, 0);

	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	
	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);	
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
										pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();	
	
	ID3DBlob *pPSBlob = NULL;
	D3DX11CompileFromFile(L"MyShader.fx", 0,
		0, "PS", "ps_5_0", 0,
		0, 0, &pPSBlob, &pErrorBlob, 0);
	g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
}

void CreateVertexBuffer()
{
	MyVertex vertices[] =
	{
		{ XMFLOAT3(-3.0f,  3.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0, 0) },
		{ XMFLOAT3( 3.0f,  3.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1, 0) },
		{ XMFLOAT3( 3.0f, -3.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1, 1) },
		{ XMFLOAT3(-3.0f, -3.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0, 1) },		
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(vertices);
	bd.Usage = D3D11_USAGE_DEFAULT;	
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);	
}

void CreateIndexBuffer()
{
	UINT indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.ByteWidth = sizeof(indices);//sizeof(UINT) * 36;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	ZeroMemory(&iinitData, sizeof(iinitData));
	iinitData.pSysMem = indices;
	g_pd3dDevice->CreateBuffer(&ibd, &iinitData, &g_pIndexBuffer);
}

void CreateConstantBuffer()
{
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	g_pd3dDevice->CreateBuffer(&cbd, NULL, &g_pConstantBuffer);
}

XMMATRIX                g_World;

XMMATRIX                g_World2;

XMMATRIX                g_View;
XMMATRIX                g_Projection;
XMMATRIX                g_wvpMatrix;

void InitMatrix()
{
	// Initialize the world matrix
	g_World = XMMatrixIdentity();
	g_World2 = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(pos, target, up);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2,  // pi
											800.0f / (FLOAT)600.0f,  // aspect ratio
											0.01f, 1000.0f);  // near plane, far plane
}

float GetDeltaTime()
{
	static float deltaTime = 0.0f;
	static DWORD lastTime = 0;
	if (lastTime == 0)	// init
	{
		lastTime = GetTickCount();
	}

	DWORD currentTime = GetTickCount();
	deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;

	return deltaTime;
}

void CalculateMatrixForBox()
{
	// set position
	g_World = XMMatrixTranslation(0, -2, 0);

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp =  XMMatrixTranspose(wvp);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);		// set conatant buffer.
}

void CalculateMatrixForBoxSmall(bool isLeft)
{
	// set position / scaling
	if (isLeft)
	{
		g_World = XMMatrixTranslation(-5.0f, 8.0f, 0.0f);
	}
	else
		g_World = XMMatrixTranslation(5.0f, 8.0f, 0.0f);

	g_World *= XMMatrixScaling(0.5f, 0.5f, 1.0f);

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);		// set conatant buffer.
}

void CalculateMatrixForBox2(float deltaTime)
{
	// scale
	float scaleValue = sinf(deltaTime) + 1;
	XMMATRIX scale = XMMatrixScaling(scaleValue, scaleValue, scaleValue);
	XMMATRIX rotate = XMMatrixRotationZ(deltaTime);
	float moveValue = cosf(deltaTime) * 10.0f;
	XMMATRIX position = XMMatrixTranslation(moveValue, 0.0f, 10.0f);
	g_World2 = scale * rotate * position;	

	XMMATRIX wvp = g_World2 * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);		// set conatant buffer.
}


ID3D11DepthStencilState* g_pDepthStencilState = NULL;
ID3D11DepthStencilState* g_pDepthStencilStateNoZEnable = NULL;

void CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilDesc.DepthEnable = true; //Depth test 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; //depth 쓰기 기능	
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; //z값이 작으면 통과. 즉 그린다

	g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pDepthStencilState);
	
	depthStencilDesc.DepthEnable = false; //depth test 비활성화
	g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pDepthStencilStateNoZEnable);
}

ID3D11BlendState *g_pTransparentBlendState = NULL;

WCHAR temp[256];
void UpdateBlendState(int i)
{
	if (g_pTransparentBlendState)
		g_pTransparentBlendState->Release();

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));

	blendDesc.RenderTarget[0].BlendEnable = true;

	if (i == 0)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"0 >>>> Dest * 1 + Src * 0");
	}
	else if (i == 1)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"1 >>>> Dest * 0 + Src * 1");
	}
	else if (i == 2)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"2 >>>> Dest * (1-a) + Src * a");
	}
	else if (i == 3)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"3 >>>> Dest * 1 + Src * a");
	}
	else if (i == 4)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;

		wsprintf(temp, L"4 >>>> Src * a - Dest * 1");
	}
	else if (i == 5)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"5 >>>> Dest * SrcRGB + Src * 0");
	}
	else if (i == 6)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"6 >>>> Dest * DestRGB + Src * 0");
	}
	else if (i == 7)
	{
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		wsprintf(temp, L"7 >>>> Dest * 0 + Src * (1-DestRGB)");
	}

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	g_pd3dDevice->CreateBlendState(&blendDesc, &g_pTransparentBlendState);
}

void DrawOpaque()
{
	//used texture 1

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV1);

	CalculateMatrixForBox();
	g_pImmediateContext->DrawIndexed(6, 0, 0);

	CalculateMatrixForBoxSmall(true);
	g_pImmediateContext->DrawIndexed(6, 0, 0);

	//used texture 2
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV2);

	CalculateMatrixForBoxSmall(false);

	g_pImmediateContext->DrawIndexed(6, 0, 0);
}

void DrawTransparent()
{
	CalculateMatrixForBox();

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV2);
	g_pImmediateContext->DrawIndexed(6, 0, 0);
}

void Render(float deltaTime)
{
	// Just clear the backbuffer
	float ClearColor[4] = { 0.3f, 0.3f, 0.3f, 1.0f }; // red,green,blue,alpha
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Use Vertex Buffer
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(MyVertex);	
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->RSSetState(g_pSolidRS);

	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	//=============================
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV1);
	CalculateMatrixForBox();
	g_pImmediateContext->DrawIndexed(6, 0, 0);
	//=============================	
	//////////////////////////////////////////////////////////////////////////
	//Z test 활성화 State 설정.
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, 0);
	g_pImmediateContext->OMSetBlendState(0, 0, 0xffffffff);

	//불투명(opaque) 오브젝트 그리기
	DrawOpaque();

	//Z test 비활성화 state 설정
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateNoZEnable, 0);
	g_pImmediateContext->OMSetBlendState(g_pTransparentBlendState, 0, 0xffffffff);

	//투명한 오브젝트 그리기
	DrawTransparent();

	g_pSwapChain->Present(0, 0);
}

void CleanupDevice()
{
	if (g_pSamplerLinear)g_pSamplerLinear->Release();

	if (g_pTextureRV1)g_pTextureRV1->Release();

	if (g_pWireFrameRS)g_pWireFrameRS->Release();
	if (g_pSolidRS)g_pSolidRS->Release();	

	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();

	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();	        
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();

	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();

	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}



#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000 ? 0 : 1))

#define szWindowClass	TEXT("First")
#define szTitle			TEXT("First App")

LRESULT CALLBACK WndProc( HWND hWnd
						 , UINT message
						 , WPARAM wParam
						 , LPARAM lParam );

int APIENTRY WinMain( HINSTANCE hInstance,
					  HINSTANCE hPrevInstance,
					  LPSTR lpszCmdParam,
					  int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize		= sizeof(WNDCLASSEX);
	wcex.style		= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon	= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;

	if( !RegisterClassEx(&wcex) )
		return 0;

	HWND	hWnd = CreateWindowEx( WS_EX_APPWINDOW
		, szWindowClass
		, szTitle
		, WS_OVERLAPPEDWINDOW
		, CW_USEDEFAULT
		, CW_USEDEFAULT
		, 800
		, 600
		, NULL
		, NULL
		, hInstance
		, NULL );
	g_hWnd = hWnd;

	if( !hWnd )
		return 0;

	ShowWindow(hWnd, nCmdShow);

	InitDevice();
	CreateShader();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateConstantBuffer();

	CreateRenderState();	

	LoadTexture();	
	
	InitMatrix();

	CreateDepthStencilState();
	UpdateBlendState(0);

	MSG			msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (WM_QUIT == msg.message)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			static float deltaTime = 0;
			deltaTime += GetDeltaTime();

			
			Render(deltaTime);
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}

// 메시지 처리 함수
LRESULT CALLBACK WndProc( HWND hWnd
						 , UINT message
						 , WPARAM wParam
						 , LPARAM lParam )
{
	HDC	hdc;
	PAINTSTRUCT	ps;
	static int modeIndex = 0;

	switch(message)
	{
	case WM_CREATE:

		break;
	case WM_PAINT:
		{
			hdc = BeginPaint( hWnd, &ps );

			EndPaint( hWnd, &ps );
		}
		break;
	case WM_KEYDOWN:
	{
		switch (wParam) 
		{
		case VK_LEFT:
			{
				--modeIndex;
				if (modeIndex == -1)
					modeIndex = 7;

				UpdateBlendState(modeIndex);
			}
			break;
		case VK_RIGHT:
			{
				++modeIndex;
				if (modeIndex == 8)
					modeIndex = 0;

				UpdateBlendState(modeIndex);
			}
			break;
		}

	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}