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

HWND					m_hWnd;
IDXGISwapChain*			m_pSwapChain;
ID3D11Device*			m_pd3dDevice;
ID3D11DeviceContext*	m_pImmediateContex; //d3dc
ID3D11RenderTargetView*	m_pRenderTargetView;
D3D_FEATURE_LEVEL		m_featureLevel;

ID3D11VertexShader*		m_pVertexShader;
ID3D11InputLayout*		m_pVertexLayout;
ID3D11PixelShader*		m_pPixelShader;
ID3D11Buffer*			m_pVertexBuffer;
ID3D11Buffer*			m_pIndexBuffer;

LRESULT CALLBACK WndProc(
	HWND hWnd, 
	UINT message,
	WPARAM wParam,
	LPARAM lParam
);

struct MyVertex
{
	XMFLOAT3	pos;
	XMFLOAT4	color;
};

HRESULT initDevice()
{
	HRESULT		hr = S_OK;

	//Flag 설정
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
	sd.BufferCount = 1;	//백버퍼 갯수

	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//백버퍼 포맷
	sd.BufferDesc.RefreshRate.Numerator = 60;			//분자
	sd.BufferDesc.RefreshRate.Denominator = 1;			//분모
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//백버퍼 랜더링
	sd.OutputWindow = m_hWnd;							//출력할 윈도우 핸들
	sd.SampleDesc.Count = 1;							//multisampling(다중표본화)
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(
		0,							//기본 디스플레이 어댑터 사용
		D3D_DRIVER_TYPE_HARDWARE,	//3D 하드웨어 가속
		0,							//소프트웨어 구동 안함
		createDeviceFlags,
		featureLevels,
		numFeatureLevels,
		D3D11_SDK_VERSION,			//SDK version
		&sd,						//Swap chain description
		&m_pSwapChain,				//생성된 SwapChain
		&m_pd3dDevice,				//생성된 device
		&m_featureLevel,			//사용된 featireLevel
		&m_pImmediateContex			//DC
		);

	if (FAILED(hr))
	{
		return hr;
	}

	//Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = m_pSwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),	//버퍼 형식
		(LPVOID*)&pBackBuffer		//받아온 버퍼
		);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		NULL, //자원 형식
		&m_pRenderTargetView
		);

	pBackBuffer->Release();	//get 해왔으면 반드시 release

	if (FAILED(hr))
	{
		return hr;
	}

	m_pImmediateContex->OMSetRenderTargets(
		1,						//렌더 대상 개수. 장면 분할시 1 초과
		&m_pRenderTargetView,	//렌더 타겟
		NULL					//깊이/스텐실 버퍼
		);

	D3D11_VIEWPORT		vp;
	vp.Width = 800;			//뷰포트 너비
	vp.Height = 600;		//뷰포트 높이
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;		//그리기 시작 원점 x
	vp.TopLeftY = 0;		//그리기 시작 원점 y

	m_pImmediateContex->RSSetViewports(1, &vp); //레스터라이즈

	return S_OK;
}

void Render()
{
	//just clear the backbuffer
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // rgbm alpha >> 배경화면 초기화

	//Clear
	m_pImmediateContex->ClearRenderTargetView(m_pRenderTargetView, clearColor);

	//////////////////////////////////////////////////////////////////////////
	//폴리곤 그리는 부분
	//////////////////////////////////////////////////////////////////////////

	//set input assembler
	m_pImmediateContex->IASetInputLayout(m_pVertexLayout);
	m_pImmediateContex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	m_pImmediateContex->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pImmediateContex->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	//set Shader and Draw
	m_pImmediateContex->VSSetShader(m_pVertexShader, NULL, 0);
	m_pImmediateContex->PSSetShader(m_pPixelShader, NULL, 0);
	//g_pImmediateContex->Draw(6, 0);		//index를 쓰지 않는 그리기 함수
	m_pImmediateContex->DrawIndexed(6, 0, 0);

	//Render (백버퍼를 프론트버퍼로 그린다.
	m_pSwapChain->Present(0, 0); //다 그렸으니 즉시 교체
}

void CleanupDevice()
{
	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
	}

	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
	}

	if (m_pImmediateContex)
	{
		m_pImmediateContex->ClearState();
	}

	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
	}

	if (m_pd3dDevice)
	{
		m_pd3dDevice->Release();
	}

	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
	}

	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
	}

	if (m_pImmediateContex)
	{
		m_pImmediateContex->Release();
	}

	if (m_pd3dDevice)
	{
		m_pd3dDevice->Release();
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
		L"MyShader.fx", 0, 0,		//shader 파일 설정
		"VS", "vs_5_0",				//컴파일 설정
		0, 0, 0,					//쉐이더 옵션
		&pVSBlob, &pErrorBlob, 0	//return value
		);

	if (FAILED(hr))
	{
		return;
	}

	hr = m_pd3dDevice->CreateVertexShader(
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		0,
		&m_pVertexShader
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

	hr = m_pd3dDevice->CreateInputLayout(
		layout,
		numElements,
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		&m_pVertexLayout
		);

	if (FAILED(hr))
		return;

	pVSBlob->Release();

	//////////////////////////////////////////////////////////////////////////
	//	PS
	//////////////////////////////////////////////////////////////////////////

	ID3DBlob* pPSBlob = NULL;
	hr = D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,		//shader 파일 설정
		"PS", "ps_5_0",				//컴파일 설정
		0, 0, 0,					//쉐이더 옵션
		&pPSBlob, &pErrorBlob, 0	//return value
		);


	if (FAILED(hr))
	{
		return;
	}

	hr = m_pd3dDevice->CreatePixelShader(
		pPSBlob->GetBufferPointer(),
		pPSBlob->GetBufferSize(),
		0,
		&m_pPixelShader
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
	//꼭지점 만들기
	//////////////////////////////////////////////////////////////////////////

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

	D3D11_BUFFER_DESC		bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(vertices);			//버퍼 크기
	bd.Usage = D3D11_USAGE_DEFAULT;		//버퍼 사용 방식
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//파이프라인에 연결되는 버퍼 형태
	bd.CPUAccessFlags = 0;						//CPU 접근 flag. 일반적으로 GPU를 사용하기 때문에 0을 쓴다.

	D3D11_SUBRESOURCE_DATA	initData;

	ZeroMemory(&initData, sizeof(initData));

	initData.pSysMem = vertices;	//초기화하기 위한 버퍼 배열

	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd,	//생성할 버퍼의 정보를 담은 구조체
		&initData,			//버퍼 초기화시 필요한 데이터
		&m_pVertexBuffer	//생성된 버퍼
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
		0, 1, 2, //삼각형0
		0, 2, 3	//삼각형 1
	};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.ByteWidth = sizeof(indices); //sizeof(uint)*6
	ibd.Usage = D3D11_USAGE_IMMUTABLE; //cpu접근 불가 생성후 변경 불가. gpu만 접근 가능
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA		initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = indices;
	m_pd3dDevice->CreateBuffer(&ibd, &initData, &m_pIndexBuffer);
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
	LPSTR lpszCmdParam, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm		= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
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

	if( !hWnd )
		return 0;

	ShowWindow(hWnd, nCmdShow);

	m_hWnd = hWnd;
	initDevice();
	CreateShader();
	CreateVertexBuffer();
	CreateIndexBuffer();

	MSG			msg;

	while( true )
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
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

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}