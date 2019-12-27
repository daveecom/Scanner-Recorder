#define WINVER 0x0401

#include <afx.h>
#include <tchar.h>
#include <crtdbg.h>
#include "armadillo.h"

Armadillo::Armadillo(void)
	:m_hModule(NULL)
{
	m_hModule = LoadLibrary(_T("ARMACCESS.DLL"));
	if (!m_hModule)
	{
#ifdef SHAREWARE
		MessageBox(NULL, _T("Warning: This executable is not protected."),
			_T("Software Safety Issue"), MB_ICONINFORMATION|MB_OK);
#endif
	}
}

Armadillo::~Armadillo(void)
{
	if (m_hModule) FreeLibrary(m_hModule);
}

bool Armadillo::VerifyKey(const char *name, const char *code)
{
	VerifyKeyFn pFn;

	if (m_hModule)
	{
		pFn = (VerifyKeyFn) GetProcAddress(m_hModule, _T("VerifyKey"));
		return pFn (name, code);
	}
	else return FALSE;
}

bool Armadillo::InstallKey(const char *name, const char *code)
{
	InstallKeyFn pFn;

	if (m_hModule)
	{
		pFn = (VerifyKeyFn) GetProcAddress(m_hModule, _T("InstallKey"));
		return pFn (name, code);
	}
	else return FALSE;
}
