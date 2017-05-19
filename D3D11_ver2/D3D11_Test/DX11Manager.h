#pragma once


#include "DX11namespace.h"



class CDX11Manager
{
public:
	CDX11Manager();
	~CDX11Manager();

//////////////////////////////////////////////////////////////////////////

	HRESULT		initDevice();
	void		Render();
	void		CleanupDevice();
	void		CreateShader();
	void		CreateVertexBuffer();
	void		CreateIndexBuffer();

protected:
private:
};

