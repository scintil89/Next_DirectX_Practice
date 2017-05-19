#include <windows.h>
#include <time.h>
#include <vector>
#include <fstream>

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

	XMFLOAT3	normal;

	XMFLOAT2	tex;
};

struct	 ConstantBuffer
{
	XMMATRIX wvp;

	XMMATRIX world;
	XMFLOAT4 lightDir;
	XMFLOAT4 lightColor;
};

XMFLOAT4 lightDirection
{
	XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
};

XMFLOAT4 lightColor
{
	XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
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
//////////////////////////////////////////////////////////////////////////
XMMATRIX                g_World;
XMMATRIX                g_World2;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
//////////////////////////////////////////////////////////////////////////
ID3D11ShaderResourceView*           	g_pTextureRV = NULL;			// Texture
ID3D11SamplerState*                 		g_pSamplerLinear = NULL;   // SampleState

//////////////////////////////////////////////////////////////////////////


//int vertexCount = 100;	//가로 / 세로의 버택스 수
//int numVertices = 10000;	//전체 버택스 수
int   indexSize = 0;

ID3D11Buffer*	g_pHeightMapVertexBuffer;
ID3D11Buffer*	g_pHeightMapIndexBuffer;

int   vertexCount = 257;     // 제공된 Raw 파일의 가로/세로 정점 개수
int    numVertices = 66049;  // 257 * 257
std::vector<int>    _heightMap;  // 높이값만을 담는 벡터


void   LoadHeightMap()
{
	std::vector<BYTE>   in(numVertices);
	std::ifstream loadFile;
	loadFile.open(L"Texture/heightMap.raw", std::ios_base::binary);

	if (loadFile)
	{
		loadFile.read((char*)&in[0], (std::streamsize)in.size());

		loadFile.close();
	}

	_heightMap.resize(numVertices);
	for (int i = 0; i < in.size(); ++i)
	{
		_heightMap[i] = in[i];
	}
}


void CreateHeightMapVB()
{
	MyVertex   *heightmapVertex = new MyVertex[numVertices];

	for (int z = 0; z < vertexCount; ++z)
	{
		for (int x = 0; x < vertexCount; ++x)
		{
			int   idx = x + (z * (vertexCount));
			heightmapVertex[idx].pos = XMFLOAT3(x, _heightMap[idx], z);
			heightmapVertex[idx].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			heightmapVertex[idx].tex = XMFLOAT2(x / (float)(vertexCount - 1), z / (float)(vertexCount - 1));
		}
	}


	D3D11_BUFFER_DESC		bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(MyVertex) * numVertices;			
	bd.Usage = D3D11_USAGE_DEFAULT;		
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	
	bd.CPUAccessFlags = 0;				

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = heightmapVertex;
	g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pHeightMapVertexBuffer);
}

void CreateHeightMapIB()
{
	int     triangleCount = (vertexCount - 1) * (vertexCount - 1) * 2;    // 삼각형 개수
	indexSize = triangleCount * 3;
	UINT *indices = new UINT[indexSize];

	int baseIndex = 0;
	int _numVertsPerRow = vertexCount;
	for (int z = 0; z < _numVertsPerRow - 1; z++)
	{
		for (int x = 0; x < _numVertsPerRow - 1; x++)
		{
			indices[baseIndex] = z   * _numVertsPerRow + x;				// 0	
			indices[baseIndex + 2] = z   * _numVertsPerRow + x + 1;		//  3
			indices[baseIndex + 1] = (z + 1) * _numVertsPerRow + x;		//  1

			indices[baseIndex + 3] = (z + 1) * _numVertsPerRow + x;		// 3
			indices[baseIndex + 5] = z   * _numVertsPerRow + x + 1;		//  4
			indices[baseIndex + 4] = (z + 1) * _numVertsPerRow + x + 1;	// 1

			baseIndex += 6;
		}
	}

	D3D11_BUFFER_DESC		ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.ByteWidth = sizeof(UINT) * indexSize;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = indices;
	g_pd3dDevice->CreateBuffer(&ibd, &initData, &g_pHeightMapIndexBuffer);
}


void CalcMatrixForHeightmap(float deltaTime)
{
	XMMATRIX mat = XMMatrixRotationY(0.0f);
	g_World = mat;

	XMMATRIX wvp = g_World * g_View * g_Projection;

	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);
	cb.world = XMMatrixTranspose(g_World);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}



HRESULT   LoadTexture()
{
	HRESULT   hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice,
															L"Texture/heightMap.jpg",
															NULL, 				
															NULL, 		
															&g_pTextureRV, 
															NULL				
	);

	D3D11_SAMPLER_DESC 	sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear); // SamplerState 생성

	if (FAILED(hr))
	{
		return hr;
	}
}


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
	sd.OutputWindow = g_hWnd;							//출력할 윈도우 핸들
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
		&g_pSwapChain,				//생성된 SwapChain
		&g_pd3dDevice,				//생성된 device
		&g_featureLevel,			//사용된 featireLevel
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
		__uuidof(ID3D11Texture2D),	//버퍼 형식
		(LPVOID*)&pBackBuffer		//받아온 버퍼
		);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
		NULL, //자원 형식
		&g_pRenderTargetView
		);

	pBackBuffer->Release();	//get 해왔으면 반드시 release

	if (FAILED(hr))
	{
		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	CreateDepthStencilTexture();
	//////////////////////////////////////////////////////////////////////////

	g_pImmediateContext->OMSetRenderTargets(
		1,						//렌더 대상 개수. 장면 분할시 1 초과
		&g_pRenderTargetView,	//렌더 타겟
		g_pDepthStencilView		//깊이/스텐실 버퍼
		);

	D3D11_VIEWPORT		vp;
	vp.Width = 800;			//뷰포트 너비
	vp.Height = 600;		//뷰포트 높이
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;		//그리기 시작 원점 x
	vp.TopLeftY = 0;		//그리기 시작 원점 y

	g_pImmediateContext->RSSetViewports(1, &vp); //레스터라이즈

	return S_OK;
}

void InitMatrix()
{
	// World 행렬 초기화
	g_World = XMMatrixIdentity();

	// View 행렬 구성
	XMVECTOR 	pos = XMVectorSet(-80.0f, 506.0f, 127.0f, 1.0f);
	XMVECTOR 	target = XMVectorSet(0.5f, -0.8f, 0.1f, 0.0f);
	XMVECTOR 	up = XMVectorSet(1.0f, 1.0f, 0.15f, 0.0f);
	g_View = XMMatrixLookAtLH(pos, target, up);

	// Projection 행렬
	g_Projection = XMMatrixPerspectiveFovLH(0.5f * XM_PIDIV2,  	// pi
		800.0f / (FLOAT)600.0f,  // aspect ratio
		0.01f, 1000.0f);  	// near plane, far plane
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

	if (g_pSamplerLinear)
	{
		g_pSamplerLinear->Release();
	}

	if (g_pTextureRV)
	{
		g_pTextureRV->Release();
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
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
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
		L"MyShader.fx", 0, 0,		//shader 파일 설정
		"PS", "ps_5_0",				//컴파일 설정
		0, 0, 0,					//쉐이더 옵션
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
	//꼭지점 만들기
	//////////////////////////////////////////////////////////////////////////


	MyVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f)	, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),	XMFLOAT3(-0.33f, 0.33f, -0.33f)	, XMFLOAT2(1.0f, 1.0f)	},
		{ XMFLOAT3(1.0f, 1.0f, -1.0f)	, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),	XMFLOAT3(0.33f, 0.33f, -0.33f)	, XMFLOAT2(0.0f, 1.0f)	},
		{ XMFLOAT3(1.0f, 1.0f, 1.0f)	, XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f),	XMFLOAT3(0.33f, 0.33f, 0.33f)	, XMFLOAT2(0.0f, 0.0f)	},
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f)	, XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),	XMFLOAT3(-0.33f, 0.33f, 0.33f)	, XMFLOAT2(1.0f, 0.0f)	},
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f)	, XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f),	XMFLOAT3(-0.33f, -0.33f, -0.33f), XMFLOAT2(0.0f, 0.0f)	},
		{ XMFLOAT3(1.0f, -1.0f, -1.0f)	, XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),	XMFLOAT3(0.33f, -0.33f, -0.33f)	, XMFLOAT2(1.0f, 0.0f)	},
		{ XMFLOAT3(1.0f, -1.0f, 1.0f)	, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	XMFLOAT3(0.33f, -0.33f, 0.33f)	, XMFLOAT2(1.0f, 1.0f)	},
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f)	, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),	XMFLOAT3(-0.33f, -0.33f, 0.33f)	, XMFLOAT2(0.0f, 1.0f)	},
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

	HRESULT hr = g_pd3dDevice->CreateBuffer(&bd,	//생성할 버퍼의 정보를 담은 구조체
		&initData,			//버퍼 초기화시 필요한 데이터
		&g_pVertexBuffer	//생성된 버퍼
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
	// 		0, 1, 2, //삼각형0
	// 		0, 2, 3	//삼각형 1
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
	ibd.Usage = D3D11_USAGE_IMMUTABLE; //cpu접근 불가 생성후 변경 불가. gpu만 접근 가능
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
	// 박스를 회전시키기 위한 연산.    위치, 크기를 변경하고자 한다면 SRT를 기억할 것.

	XMMATRIX mat = XMMatrixRotationY(deltaTime);
	mat *= XMMatrixRotationX(-deltaTime);
	g_World = mat;

	XMMATRIX wvp = g_World * g_View * g_Projection;

	ConstantBuffer       cb;
	cb.wvp = XMMatrixTranspose(wvp);

	cb.world = XMMatrixTranspose(g_World);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;



	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}

void CalculateMatrixForBox2(float deltaTime)
{
	//float scaleValue = sinf(deltaTime)*0.5f + 1;
	//XMMATRIX scale = XMMatrixScaling(scaleValue, scaleValue, scaleValue);	//scale

	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);						// scale
	XMMATRIX rotate = XMMatrixRotationZ(deltaTime);							//rotate

	float moveValue = 5.0f;

	XMMATRIX position = XMMatrixTranslation(moveValue, 0.0f, 0.0f);
	g_World2 = scale * rotate * position; //SRT. OPENGL은 TRS


	XMMATRIX rotate2 = XMMatrixRotationY(-deltaTime);
	g_World2 *= rotate2;

	XMMATRIX wvp = g_World2 * g_View * g_Projection;
	ConstantBuffer       cb;
	cb.wvp = XMMatrixTranspose(wvp);

	cb.world = XMMatrixTranspose(g_World2);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0); // update data
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);// set constant buffer.
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
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
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;		//fill 옵션 solid wire
	rasterizerDesc.CullMode = D3D11_CULL_BACK;		//back front| 컬링 옵션
	rasterizerDesc.FrontCounterClockwise = false;	//앞/뒷면 로직 선택

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pSolidRS);
}

void CreateRenderState2()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;		//fill 옵션
	rasterizerDesc.CullMode = D3D11_CULL_NONE;		//back | 컬링 옵션
	rasterizerDesc.FrontCounterClockwise = true;	//앞/뒷면 로직 선택

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pWireRS);
}

void Render(float deltaTime)
{
	float ClearColor[4] = { 0.3f, 0.3f, 0.3f, 1.0f };

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	g_pImmediateContext->ClearDepthStencilView(
		g_pDepthStencilView,	//clear target
		D3D11_CLEAR_DEPTH,		//clear flag(depth, stencil)
		1.0f,					//depth buffer 지울 때 채울값
		0);						//stencil buffer 지울 때 초기값	


	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;

	
	//g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	//g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pHeightMapVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pHeightMapIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	//////////////////////////////////////////////////////////////////////////
	// 계산 및 그리기
	//Box1
	//g_pImmediateContext->RSSetState(g_pSolidRS);
	//CalculateMatrixForBox(deltaTime);
	//g_pImmediateContext->DrawIndexed(36, 0, 0);
	//
	////Box2
	//g_pImmediateContext->RSSetState(g_pWireRS);
	//CalculateMatrixForBox2(deltaTime);
	//g_pImmediateContext->DrawIndexed(36, 0, 0);


	g_pImmediateContext->RSSetState(g_pSolidRS);
	CalcMatrixForHeightmap(deltaTime);   // 지형에 관련한 matrix 연산
	g_pImmediateContext->DrawIndexed(indexSize, 0, 0);



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
	LoadTexture();
	LoadHeightMap();
	CreateRenderState();
	CreateRenderState2();

	CreateShader();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateConstantBuffer();
	InitMatrix();

	CreateHeightMapIB();
	CreateHeightMapVB();

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
			// GetDeltaTime이 없으면 0.00005f등의 작은수를 쓸 것.

			Render(deltaTime);
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}

// 메시지 처리 함수
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