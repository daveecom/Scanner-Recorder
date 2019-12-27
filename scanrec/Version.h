//////////////////////////////////////////////////////////////////////////////
// Version resource access class header file
//////////////////////////////////////////////////////////////////////////////
// CVersion class

#ifndef _VERSION_HEADER_DEFINED
#define _VERSION_HEADER_DEFINED

//////////////////////////////////////////////////////////////////////////////
class CVersion : public CObject
{
public:
	VS_FIXEDFILEINFO* GetVersionInfo();
	LPCTSTR GetProductVersion();
	LPCTSTR GetProductName();
	LPCTSTR GetOriginalFilename();
	LPCTSTR GetLegalCopyright();
	LPCTSTR GetLegalTrademarks();
	LPCTSTR GetLangCharset();
	LPCTSTR GetInternalName();
	LPCTSTR GetFileVersion();
	LPCTSTR GetFileDescription();
	LPCTSTR GetCompanyName();
	LPCTSTR GetSpecialBuild();
	LPCTSTR GetPrivateBuild();
	LPCTSTR GetComments();
	DWORD GetVersion();
	~CVersion();
	CVersion();
    LPTSTR Scan4Delim(LPCTSTR ptr);

protected:
	CString m_LangCharset;
	CString m_ProductVersion;
	CString m_ProductName;
	CString m_OriginalFilename;
	CString m_LegalCopyright;
    CString m_LegalTrademarks;
	CString m_InternalName;
	CString m_FileDescription;
	CString m_FileVersion;
	CString m_CompanyName;
	CString m_AppName;
    CString m_SpecialBuild;
    CString m_PrivateBuild;
    CString m_Comments;
	VS_FIXEDFILEINFO* m_VersionInfo;
	PVOID m_pBuffer;
	DWORD m_Size;
};

#endif
