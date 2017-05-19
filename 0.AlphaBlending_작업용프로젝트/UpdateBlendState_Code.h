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