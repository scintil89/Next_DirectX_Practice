#include <windows.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////
//DirectX 11 header
#include <dxgi.h>
#include <d3d11.h>
#include <d3dCompiler.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <xnamath.h>
//////////////////////////////////////////////////////////////////////////

#define szWindowClass	TEXT("First")
#define szTitle			TEXT("First App")

struct MyVertex
{
	XMFLOAT3	pos;
	XMFLOAT4	color;
};

struct	 ConstantBuffer
{
	XMMATRIX wvp;
};

HWND					g_hWnd;
IDXGISwapChain*			g_pSwapChain;
ID3D11Device*			g_pd3dDevice;
ID3D11DeviceContext*	g_pImmediateContext; //d3dc
ID3D11RenderTargetView*	g_pRenderTargetView;
D3D_FEATURE_LEVEL		g_featureLevel;

ID3D11VertexShader*		g_pVertexShader;
ID3D11InputLayout*		g_pVertexLayout;
ID3D11PixelShader*		g_pPixelShader;
ID3D11Buffer*			g_pVertexBuffer;
ID3D11Buffer*			g_pIndexBuffer;
//////////////////////////////////////////////////////////////////////////
ID3D11Buffer*			g_pConstantBuffer;
//////////////////////////////////////////////////////////////////////////
ID3D11Texture2D*		g_pDepthStencil = NULL;
ID3D11DepthStencilView*	g_pDepthStencilView = NULL;
//////////////////////////////////////////////////////////////////////////
ID3D11RasterizerState*	g_pSolidRS;
ID3D11RasterizerState*	g_pWireRS;

XMMATRIX                g_World;
XMMATRIX                g_World2;
XMMATRIX                g_View;
XMMATRIX                g_Projection;

void CreateDepthStencilTexture();

LRESULT CALLBACK WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
	);

HRESULT initDevice()
{
	HRESULT		hr = S_OK;

	//Flag ����
	UINT	createDeviceFlags = 0;

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC	sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;	//����� ����

	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//����� ����
	sd.BufferDesc.RefreshRate.Numerator = 60;			//����
	sd.BufferDesc.RefreshRate.Denominator = 1;			//�и�
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//����� ������
	sd.OutputWindow = g_hWnd;							//����� ������ �ڵ�
	sd.SampleDesc.Count = 1;							//multisampling(����ǥ��ȭ)
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		0,							//�⺻ ���÷��� ����� ���
		D3D_DRIVER_TYPE_HARDWARE,	//3D �ϵ���� ����
		0,							//����Ʈ���� ���� ����
		createDeviceFlags,
		featureLevels,
		numFeatureLevels,
		D3D11_SDK_VERSION,			//SDK version
		&sd,						//Swap chain description
		&g_pSwapChain,				//������ SwapChain
		&g_pd3dDevice,				//������ device
		&g_featureLevel,			//���� featireLevel
		&g_pImmediateContext			//DC
		);

	if (FAILED(hr))
	{
		return hr;
	}

	//Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),	//���� ����
		(LPVOID*)&pBackBuffer		//�޾ƿ� ����
		);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		NULL, //�ڿ� ����
		&g_pRenderTargetView
		);

	pBackBuffer->Release();	//get �ؿ����� �ݵ�� release

	if (FAILED(hr))
	{
		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	CreateDepthStencilTexture();
	//////////////////////////////////////////////////////////////////////////

	g_pImmediateContext->OMSetRenderTargets(
		1,						//���� ��� ����. ��� ���ҽ� 1 �ʰ�
		&g_pRenderTargetView,	//���� Ÿ��
		g_pDepthStencilView		//����/���ٽ� ����
		);

	D3D11_VIEWPORT		vp;
	vp.Width = 800;			//����Ʈ �ʺ�
	vp.Height = 600;		//����Ʈ ����
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;		//�׸��� ���� ���� x
	vp.TopLeftY = 0;		//�׸��� ���� ���� y

	g_pImmediateContext->RSSetViewports(1, &vp); //�����Ͷ�����

	return S_OK;
}

void InitMatrix()
{
	// World ��� �ʱ�ȭ
	g_World = XMMatrixIdentity();

	// View ��� ����
	XMVECTOR 	pos = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	XMVECTOR 	target = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR 	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(pos, target, up);

	// Projection ���
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2,  	// pi
		800.0f / (FLOAT)600.0f,  // aspect ratio
		0.01f, 100.0f);  	// near plane, far plane
}

void CleanupDevice()
{
	if (g_pIndexBuffer)
	{
		g_pIndexBuffer->Release();
	}

	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
	}

	if (g_pImmediateContext)
	{
		g_pImmediateContext->ClearState();
	}

	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
	}

	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
	}

	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
	}

	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
	}

	if (g_pImmediateContext)
	{
		g_pImmediateContext->Release();
	}

	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
	}

	if (g_pConstantBuffer)
	{
		g_pConstantBuffer->Release();
	}

}

void CreateShader()
{
	//////////////////////////////////////////////////////////////////////////
	//	vertex shader
	//////////////////////////////////////////////////////////////////////////

	ID3DBlob* pErrorBlob = NULL;
	ID3DBlob* pVSBlob = NULL;
	HRESULT hr = D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,		//shader ���� ����
		"VS", "vs_5_0",				//������ ����
		0, 0, 0,					//���̴� �ɼ�
		&pVSBlob, &pErrorBlob, 0	//return value
		);

	if (FAILED(hr))
	{
		return;
	}

	hr = g_pd3dDevice->CreateVertexShader(
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		0,
		&g_pVertexShader
		);

	if (FAILED(hr))
	{
		return;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = ARRAYSIZE(layout);

	hr = g_pd3dDevice->CreateInputLayout(
		layout,
		numElements,
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		&g_pVertexLayout
		);

	if (FAILED(hr))
		return;

	pVSBlob->Release();

	//////////////////////////////////////////////////////////////////////////
	//	PS
	//////////////////////////////////////////////////////////////////////////

	ID3DBlob* pPSBlob = NULL;
	hr = D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,		//shader ���� ����
		"PS", "ps_5_0",				//������ ����
		0, 0, 0,					//���̴� �ɼ�
		&pPSBlob, &pErrorBlob, 0	//return value
		);


	if (FAILED(hr))
	{
		return;
	}

	hr = g_pd3dDevice->CreatePixelShader(
		pPSBlob->GetBufferPointer(),
		pPSBlob->GetBufferSize(),
		0,
		&g_pPixelShader
		);

	if (FAILED(hr))
	{
		return;
	}

	pPSBlob->Release();
}

void CreateVertexBuffer()
{
	//////////////////////////////////////////////////////////////////////////
	//������ �����
	//////////////////////////////////////////////////////////////////////////

	/*
	MyVertex vertices[] = {
	// 		{ XMFLOAT3(-0.5f, 0.5f, 1.0f)	}, //1
	// 		{ XMFLOAT3(0.5f, -0.5f, 1.0f)	}, //2
	// 		{ XMFLOAT3(-0.5f, -0.5f, 1.0f)	}, //3
	//
	// 		{ XMFLOAT3(0.5f, -0.5f, 1.0f)	}, //2
	// 		{ XMFLOAT3(-0.5f, 0.5f, 1.0f)	}, //1
	// 		{ XMFLOAT3(0.5f, 0.5f, 1.0f)	}, //4

	{ XMFLOAT3(-0.5f, 0.5f, 1.0f), XMFLOAT4(0.8f, 0.0f, 0.2f, 1.0f) },
	{ XMFLOAT3(0.5f, 0.5f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
	{ XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(-0.5f, -0.5f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
	};
	*/

	MyVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
	};


	D3D11_BUFFER_DESC		bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(vertices);			//���� ũ��
	bd.Usage = D3D11_USAGE_DEFAULT;		//���� ��� ���
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//���������ο� ����Ǵ� ���� ����
	bd.CPUAccessFlags = 0;						//CPU ���� flag. �Ϲ������� GPU�� ����ϱ� ������ 0�� ����.

	D3D11_SUBRESOURCE_DATA	initData;

	ZeroMemory(&initData, sizeof(initData));

	initData.pSysMem = vertices;	//�ʱ�ȭ�ϱ� ���� ���� �迭

	HRESULT hr = g_pd3dDevice->CreateBuffer(&bd,	//������ ������ ������ ���� ����ü
		&initData,			//���� �ʱ�ȭ�� �ʿ��� ������
		&g_pVertexBuffer	//������ ����
		);

	if (FAILED(hr))
	{
		return;
	}
}

void CreateIndexBuffer()
{
	// 	UINT	indices[] =
	// 	{
	// 		0, 1, 2, //�ﰢ��0
	// 		0, 2, 3	//�ﰢ�� 1
	// 	};

	UINT indices[] =
	{
		3, 1, 0,
		2, 1, 3,
		0, 5, 4,
		1, 5, 0,
		3, 4, 7,
		0, 4, 3,
		1, 6, 5,
		2, 6, 1,
		2, 7, 6,
		3, 7, 2,
		6, 4, 5,
		7, 4, 6,
	};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.ByteWidth = sizeof(indices); //sizeof(uint)*6
	ibd.Usage = D3D11_USAGE_IMMUTABLE; //cpu���� �Ұ� ������ ���� �Ұ�. gpu�� ���� ����
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA		initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = indices;
	g_pd3dDevice->CreateBuffer(&ibd, &initData, &g_pIndexBuffer);
}

void CreateConstantBuffer()
{
	D3D11_BUFFER_DESC 	cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;

	g_pd3dDevice->CreateBuffer(&cbd, NULL, &g_pConstantBuffer);
}

void CalculateMatrixForBox(float deltaTime)
{
	// �ڽ��� ȸ����Ű�� ���� ����.    ��ġ, ũ�⸦ �����ϰ��� �Ѵٸ� SRT�� ����� ��.

	XMMATRIX mat = XMMatrixRotationY(deltaTime);
	mat *= XMMatrixRotationX(-deltaTime);
	g_World = mat;

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer       cb;
	cb.wvp = XMMatrixTranspose(wvp);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
}

void CalculateMatrixForBox2(float deltaTime)
{
	float scaleValue = sinf(deltaTime)*0.5f + 1;
	XMMATRIX scale = XMMatrixScaling(scaleValue, scaleValue, scaleValue);	//scale
	XMMATRIX rotate = XMMatrixRotationZ(deltaTime);							//rotate
	float moveValue = cosf(deltaTime)*5.0f;									//move position
	XMMATRIX position = XMMatrixTranslation(moveValue, 0.0f, 0.0f);
	g_World2 = scale * rotate * position; //SRT. OPENGL�� TRS

	XMMATRIX wvp = g_World2 * g_View * g_Projection;
	ConstantBuffer       cb;
	cb.wvp = XMMatrixTranspose(wvp);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
}

void CreateDepthStencilTexture()
{
	//create depth stencil texture
	D3D11_TEXTURE2D_DESC	descDepth;
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

	//create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
			// == DXGI_FORMAT_D24_UNORM_S8_UNIT
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;
	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);

}

void CreateRenderState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;		//fill �ɼ�
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;		//back | �ø� �ɼ�
	rasterizerDesc.FrontCounterClockwise = false;	//��/�޸� ���� ����

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pSolidRS);
}

void CreateRenderState2()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;		//fill �ɼ�
	rasterizerDesc.CullMode = D3D11_CULL_NONE;		//back | �ø� �ɼ�
	rasterizerDesc.FrontCounterClockwise = true;	//��/�޸� ���� ����

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pWireRS);
}

void Render(float deltaTime)
{
	float ClearColor[4] = { 1.0f, 0.3f, 0.3f, 1.0f };

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	g_pImmediateContext->ClearDepthStencilView(
		g_pDepthStencilView,	//clear target
		D3D11_CLEAR_DEPTH,		//clear flag(depth, stencil)
		1.0f,					//depth buffer ���� �� ä�ﰪ
		0);						//stencil buffer ���� �� �ʱⰪ	


	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	// ��� �� �׸���

	//////////////////////////////////////////////////////////////////////////
	g_pImmediateContext->RSSetState(g_pSolidRS);
	CalculateMatrixForBox(deltaTime);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	//////////////////////////////////////////////////////////////////////////
	g_pImmediateContext->RSSetState(g_pWireRS);
	CalculateMatrixForBox2(deltaTime);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	g_pSwapChain->Present(0, 0);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wcex))
		return 0;

	HWND	hWnd = CreateWindowEx(WS_EX_APPWINDOW
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
		, NULL);

	if (!hWnd)
		return 0;

	ShowWindow(hWnd, nCmdShow);

	g_hWnd = hWnd;

	initDevice();

	CreateRenderState();
	CreateRenderState2();

	CreateShader();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateConstantBuffer();
	InitMatrix();

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
			deltaTime += 0.00005f;
			// GetDeltaTime�� ������ 0.00005f���� �������� �� ��.

			Render(deltaTime);
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}

// �޽��� ó�� �Լ�
LRESULT CALLBACK WndProc(HWND hWnd
	, UINT message
	, WPARAM wParam
	, LPARAM lParam)
{
	HDC	hdc;
	PAINTSTRUCT	ps;

	switch (message)
	{
	case WM_CREATE:
		break;

	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
	}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}