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

#define szWindowClass	TEXT("03 ConstantBuffer, DepthBuffer")
#define szTitle			TEXT("03 ConstantBuffer, DepthBuffer")

struct MyVertex
{
	XMFLOAT3	pos;
	XMFLOAT4	color;
};

struct	 ConstantBuffer
{
	XMMATRIX   wvp;
};

HWND					g_hWnd = NULL;
IDXGISwapChain*			g_pSwapChain = NULL;
ID3D11Device*			g_pd3dDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL; //d3dc
ID3D11RenderTargetView*	g_pRenderTargetView = NULL;
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11VertexShader*		g_pVertexShader = NULL;
ID3D11InputLayout*		g_pVertexLayout = NULL;
ID3D11PixelShader*		g_pPixelShader = NULL;
ID3D11Buffer*			g_pVertexBuffer = NULL;
ID3D11Buffer*			g_pIndexBuffer = NULL;


//���� ��ȯ�� ���� ��� ����
XMMATRIX				g_World;
XMMATRIX				g_View;
XMMATRIX				g_Projection;

XMMATRIX				g_World2;


//��� ���� ����
ID3D11Buffer*			g_pConstantBuffer;

LRESULT CALLBACK WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
	);


//���� ���� ����
ID3D11Texture2D*        g_pDepthStencil = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;

void InitMatrix()
{
	//World ��� �ʱ�ȭ
	g_World = XMMatrixIdentity();

	//View ��� ����
	XMVECTOR	pos			= XMVectorSet(0.0f, 0.0f, -8.0f, 1.0f);
	XMVECTOR	target		= XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR	up			= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	g_View = XMMatrixLookAtLH(pos, target, up);

	//Projection ���
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, 800.0f / (float)600.0f, 0.3f, 1000.0f); // pi, aspect ratio. near plane, far plane
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	UINT	indices[] =
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

void CalculateMatrixForBox(float deltaTime)
{
	// �ڽ��� ȸ����Ű�� ���� ����.    ��ġ, ũ�⸦ �����ϰ��� �Ѵٸ� SRT�� ����� ��.
	XMMATRIX mat = XMMatrixRotationY(deltaTime);
	mat *= XMMatrixRotationX(-deltaTime);
	g_World = mat;                     // ���⼭ g_world�� �ڽ��� ���� Matrix��.

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer       cb;
	cb.wvp = XMMatrixTranspose(wvp);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
}

void CalculateMatrixForBox2(float deltaTime)
{
	XMMATRIX   scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);   // scale
	XMMATRIX   rotate = XMMatrixRotationZ(deltaTime);    // rotate
	float   moveValue = 5.0f;// move position
	XMMATRIX   position = XMMatrixTranslation(moveValue, 0.0f, 0.0f);
	g_World2 = scale  *  rotate  *  position;     // S * R * T

	XMMATRIX   rotate2 = XMMatrixRotationY(-deltaTime);    // rotate
	g_World2 *= rotate2;

	XMMATRIX   wvp = g_World2  *  g_View  *  g_Projection;
	ConstantBuffer   cb;
	cb.wvp = XMMatrixTranspose(wvp);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}

void CreateDepthStencilTexture()
{
	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC     descDepth;
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
	D3D11_DEPTH_STENCIL_VIEW_DESC 	descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format; 					// == DXGI_FORMAT_D24_UNORM_S8_UINT
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;	// MSAA�� ����Ѵٸ� D3D11_DSV_DIMENSION_TEXTURE2DMS�� �����
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;

	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);

	g_pDepthStencil->Release();
}

void Render(float deltaTime)
{
	float ClearColor[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	//Clear target, Clear flag(depth, stencil), depth buffer ���� �� ä�� ��, stencil buffer ���� �� �ʱⰪ
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); 
	
	// Set Input Assembler
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set Shader
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	// Draw
	CalculateMatrixForBox(deltaTime);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	CalculateMatrixForBox2(deltaTime);
	g_pImmediateContext->DrawIndexed(36, 0, 0);


	g_pSwapChain->Present(0, 0);
}

HRESULT InitDevice()
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

	CreateDepthStencilTexture();

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

void CleanupDevice()
{
	if (g_pImmediateContext)
	{
		g_pImmediateContext->ClearState();
	}

	if (g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
	}

	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
	}

	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
	}

	if (g_pVertexBuffer)
	{
		g_pVertexBuffer->Release();
	}

	if (g_pVertexLayout)
	{
		g_pVertexLayout->Release();
	}

	if (g_pVertexShader)
	{
		g_pVertexShader->Release();
	}

	if (g_pPixelShader)
	{
		g_pPixelShader->Release();
	}

	if (g_pConstantBuffer)
	{
		g_pConstantBuffer->Release();
	}

	if (g_pDepthStencilView)
	{
		g_pDepthStencilView->Release();
	}
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

	InitDevice();
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
			deltaTime += 0.0002f;
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