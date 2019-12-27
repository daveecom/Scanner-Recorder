#pragma once

#include <crtdbg.h>

#ifndef CCS_DEFINED
#define CCS_DEFINED

// Critical section wrapper class
class CSection : public CRITICAL_SECTION
{
public:
	__forceinline CSection()
	{
		InitializeCriticalSection(this);
	};

	__forceinline ~CSection()
	{
		DeleteCriticalSection(this);
	};
};

class CCS
{
public:
	// Constructor to lock the CS
	__forceinline CCS(CSection* lpCS)
		: m_lpCS(NULL)
	{
		_ASSERT(lpCS != NULL);

		m_lpCS = lpCS;	// Save
		EnterCriticalSection(lpCS);
	}

	// Destructor unlocks it.
	__forceinline ~CCS()
	{
		if (m_lpCS != NULL)
			LeaveCriticalSection(m_lpCS);
	}
	
	CSection*	m_lpCS;
};

#endif // CCS_DEFINED
