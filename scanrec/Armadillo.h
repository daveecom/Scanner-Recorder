#pragma once

typedef bool (__stdcall *CheckCodeFn)(const char *name, const char *code); 
typedef bool (__stdcall *VerifyKeyFn)(const char *name, const char *code); 
typedef bool (__stdcall *InstallKeyFn)(const char *name, const char *code); 
typedef bool (__stdcall *InstallKeyLaterFn)(const char *name, const char *code); 
typedef bool (__stdcall *UninstallKeyFn)(void); 
typedef bool (__stdcall *SetDefaultKeyFn)(void); 
typedef bool (__stdcall *UpdateEnvironmentFn)(void); 
typedef bool (__stdcall *IncrementCounterFn)(void); 
typedef int  (__stdcall *CopiesRunningFn)(void); 
typedef bool (__stdcall *ChangeHardwareLockFn)(void); 
typedef DWORD (__stdcall *GetShellProcessIDFn)(void); 
typedef bool (__stdcall *FixClockFn)(const char *fixclockkey); 
typedef DWORD (__stdcall *RawFingerprintInfoFn)(DWORD item); 
typedef bool (__stdcall *SetUserStringFn)(int which, const char *string); 
typedef DWORD (__stdcall *GetUserStringFn)(int which, char *buffer, DWORD bufferlength); 
typedef bool (__stdcall *WriteHardwareChangeLogFn)(const char *filename); 
typedef bool (__stdcall *ConnectedToServerFn)(void); 
typedef bool (__stdcall *CallBuyNowURLFn)(HWND parent); 
typedef bool (__stdcall *CallCustomerServiceURLFn)(HWND parent); 
typedef void (__stdcall *ShowReminderMessageFn)(HWND parent); 
typedef void (__stdcall *ShowReminderMessage2Fn)(HWND parent); 
typedef bool (__stdcall *ExpireCurrentKeyFn)(void); 
typedef bool (__stdcall *ShowEnterKeyDialogFn)(HWND parent); 

// Wrapper class for the Armadillo library

class	Armadillo
{
public:
	Armadillo(void);
	~Armadillo(void);

	bool		CheckCode(const char *name, const char *code); 
	bool		VerifyKey(const char *name, const char *code); 
	bool		InstallKey(const char *name, const char *code); 
	bool		InstallKeyLater(const char *name, const char *code); 
	bool		UninstallKey(void); 
	bool		SetDefaultKey(void); 
	bool		UpdateEnvironment(void); 
	bool		IncrementCounter(void); 
	int			CopiesRunning(void); 
	bool		ChangeHardwareLock(void); 
	DWORD		GetShellProcessID(void); 
	bool		FixClock(const char *fixclockkey); 
	DWORD		RawFingerprintInfo(DWORD item); 
	bool		SetUserString(int which, const char *string); 
	DWORD		GetUserString(int which, char *buffer, DWORD bufferlength); 
	bool		WriteHardwareChangeLog(const char *filename); 
	bool		ConnectedToServer(void); 
	bool		CallBuyNowURL(HWND parent); 
	bool		CallCustomerServiceURL(HWND parent); 
	void		ShowReminderMessage(HWND parent); 
	void		ShowReminderMessage2(HWND parent); 
	bool		ExpireCurrentKey(void); 
	bool		ShowEnterKeyDialog(HWND parent); 

	HMODULE		m_hModule;		// Module handle for the libaray
};
