///////////////////////////////////////////////////////////////////////////////
// Version resource access class implementation file

#define WINVER 0x0401
#include <afx.h>
#include "Version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVersion class

CVersion::CVersion()
{
    CString str;
    PCHAR Buffer;
    UINT Length;
    DWORD zero;
	_TCHAR ModuleName[MAX_PATH];

	GetModuleFileName(NULL, ModuleName, sizeof ModuleName);
   
    m_AppName = ModuleName;

    m_Size = ::GetFileVersionInfoSize((LPTSTR) (LPCTSTR) m_AppName, &zero);
    ASSERT(m_Size);

    m_pBuffer = (PVOID) new char[m_Size];

    int rc = ::GetFileVersionInfo((LPTSTR) (LPCTSTR) m_AppName, NULL, m_Size, m_pBuffer);

    // Get the VS_FIXEDFILEINFO data
    rc = ::VerQueryValue(m_pBuffer, TEXT("\\"), (PVOID*) &m_VersionInfo, &Length);
    ASSERT(rc);
    ASSERT(Length == sizeof(VS_FIXEDFILEINFO));

    rc = ::VerQueryValue(m_pBuffer, TEXT("\\VarFileInfo\\Translation"), (PVOID*) &Buffer, &Length);
    if (rc) m_LangCharset.Format("%04X%04X", LOWORD(*((LONG*)Buffer)), HIWORD(*((LONG*)Buffer)));

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "CompanyName");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_CompanyName = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "FileDescription");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_FileDescription = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "FileVersion");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_FileVersion = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "InternalName");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_InternalName = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "LegalCopyright");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_LegalCopyright = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "OriginalFilename");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_OriginalFilename = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "ProductName");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_ProductName = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "ProductVersion");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_ProductVersion = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "SpecialBuild");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_SpecialBuild = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "PrivateBuild");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_PrivateBuild = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "LegalTrademarks");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_LegalTrademarks = Buffer;

    str.Format("\\StringFileInfo\\%s\\%s", m_LangCharset, "Comments");
    rc = ::VerQueryValue(m_pBuffer, (LPTSTR) (LPCTSTR) str, (PVOID*) &Buffer, &Length);
    if (rc) m_Comments = Buffer;
}

CVersion::~CVersion()
{
    delete m_pBuffer;
}

DWORD CVersion::GetVersion()
{
    return TRUE;
}

LPTSTR CVersion::Scan4Delim(LPCTSTR ptr)
{
    LPSTR ptr2 = (LPSTR) ptr;

    // Scan until delimiter then return the ptr to it
    for(;;ptr2++)
    {
        if (*ptr2 == 0)     return ptr2;
        if (*ptr2 == ',')   return ptr2;
        if (*ptr2 == '"')   return ptr2;
    }
}

LPCTSTR CVersion::GetCompanyName()
{
    return m_CompanyName;
}

LPCTSTR CVersion::GetFileDescription()
{
    return m_FileDescription;
}

LPCTSTR CVersion::GetFileVersion()
{
    return m_FileVersion;
}

LPCTSTR CVersion::GetInternalName()
{
    return m_InternalName;
}

LPCTSTR CVersion::GetLangCharset()
{
    return m_LangCharset;
}

LPCTSTR CVersion::GetLegalCopyright()
{
    return m_LegalCopyright;
}

LPCTSTR CVersion::GetOriginalFilename()
{
    return m_OriginalFilename;
}

LPCTSTR CVersion::GetProductName()
{
    return m_ProductName;
}

LPCTSTR CVersion::GetProductVersion()
{
    return m_ProductVersion;
}

VS_FIXEDFILEINFO* CVersion::GetVersionInfo()
{
    return m_VersionInfo;
}

LPCTSTR CVersion::GetSpecialBuild()
{
    return m_SpecialBuild;
}

LPCTSTR CVersion::GetPrivateBuild()
{
    return m_PrivateBuild;
}

LPCTSTR CVersion::GetLegalTrademarks()
{
    return m_LegalTrademarks;
}

LPCTSTR CVersion::GetComments()
{
    return m_Comments;
}

