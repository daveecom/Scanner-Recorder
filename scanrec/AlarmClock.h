#pragma once

#include "ccs.h"

#ifdef ll
	#undef ll
#endif

#define ll i64

#define ONE_MILLISECOND	(10000ll)
#define ONE_SECOND		(10000000ll)
#define ONE_MINUTE		(ONE_SECOND	*	60)
#define ONE_HOUR		(ONE_SECOND	*	3600)
#define ONE_DAY			(ONE_HOUR	*	24)
#define ONE_WEEK		(ONE_DAY	*	7)

#define UWM_ALARM		(WM_USER+17)	// Modify this message id if it interferes
										// with other WM_USER messages in your project.
										// wParam = *CAlarmClock that sent this msg.
										// lParam = dwUser value passed to SetAlarm.

class CAlarmClock;
typedef BOOL (__stdcall *LPALARMCB)(CAlarmClock* pThis, DWORD_PTR dwUserData);


// Very important union makes it easy to access the FILETIME as a 64 bit unsigned int.
typedef union	_uft
{
	FILETIME		ft;
	ULONGLONG		ll;
	LARGE_INTEGER	i;
}	UFT;

//--//--//--//--//--//--//--//--//--//--//--//--//--//

typedef enum eRepeatType
{
	Repeat_Off	= 0,	// Turn all repeats off
	Repeat_Interval,	// Repeat every DD:HH:MM:SS:MS
	Repeat_Monthly,		// Day every month
	Repeat_Weekdays		// Certain days of the week
}	REPEAT_TYPE;

typedef struct _repeat_parms
{
	REPEAT_TYPE			Type;			// One of several types.

	// The following 3 data are used by the interval, daily and monthly types (resp.)
	int					dd, hh, mm, ss, ms;	// Repeat interval (dd:hh:mm:ss:ms)
	BOOL				RepeatWD[7];	// Day of week ([0] = sun, [1] = mon, [2] = tue, etc.
	int					RepeatDOM;		// Day of month (0 = off, 1 to n

	// Repeat end conditions.
	// If tRepeatUntil != 0, ignore nRepeatcount and bRepeatForever.
	DWORD_PTR			nRepeatCount;	// 0 to n times.
	BOOL				bRepeatForever;	// Don't ever stop repeating
	UFT					tRepeatUntil;	// Date / Time to stop repeating. (UFT.ft = filetime)
}	REPEAT_PARMS;

//--//--//--//--//--//--//--//--//--//--//--//--//--//

class CAlarmClock
{
public:
						CAlarmClock(void);
						~CAlarmClock(void);

						// Use to calculate the next alarm DT. (can be used internally and externally)
	static FILETIME		WhenIsNextAlarm(const FILETIME DTin, const REPEAT_PARMS RepeatParms);

						// Use whenever the local time is needed.
	static void			GetTime(UFT* pUFT);
	static void			GetTime(FILETIME* pFT);

						// Set the alarm and start the clock. This group uses a callback
						// to alert the caller when the alarm time is reached.
	BOOL				SetAlarm(FILETIME Time, LPALARMCB pCB, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(SYSTEMTIME Time, LPALARMCB pCB, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(int DD, int HH, int MM, int SS, int MS, LPALARMCB pCB, DWORD_PTR dwUser = 0);

						// Set the alarm and start the clock. This group uses a window message
						// to alert the caller when the alarm time is reached.
	BOOL				SetAlarm(FILETIME Time, HWND hwndUser, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(SYSTEMTIME Time, HWND hwndUser, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(int DD, int HH, int MM, int SS, int MS, HWND hwndUser, DWORD_PTR dwUser = 0);

						// Set the alarm and start the clock. This group uses an event
						// to alert the caller when the alarm time is reached.
	BOOL				SetAlarm(FILETIME Time, HANDLE hEvent, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(SYSTEMTIME Time, HANDLE hEvent, DWORD_PTR dwUser = 0);
	BOOL				SetAlarm(int DD, int HH, int MM, int SS, int MS, HANDLE hEvent, DWORD_PTR dwUser = 0);

						// Set the alarm and start the clock. Wait here blocked until the
						// alarm time is reached. Obviously repeat alarms aren't supported here.
	BOOL				SetAlarmAndWait(FILETIME Time);
	BOOL				SetAlarmAndWait(SYSTEMTIME Time);
	BOOL				SetAlarmAndWait(int DD, int HH, int MM, int SS, int MS);

						// Submit the REPEAT_PARMS into the CAlarmClock. It's ok to call this
						// function before or after calling SetAlarm.
	BOOL				SetRepeat(REPEAT_PARMS Parms);
	BOOL				SetRepeat(long nCount, int nDays, int nHours, int nMinutes, int nSeconds, int nMilliseconds);
	BOOL				SetRepeat(long nCount, int DayOfMonth);
	BOOL				SetRepeat(long nCount, BOOL bDay[7]);
	REPEAT_PARMS		GetRepeat(void);
	void				GetRepeat(REPEAT_PARMS* pParms);

						// Used to access the tRepeatUntil field in the REPEAT_PARMS that
						// was submitted to us via SetRepeat.
	void				SetExpirationDate(FILETIME* lpDateTime);
	void				SetExpirationDate(SYSTEMTIME* lpDateTime);
	void				GetExpirationDate(FILETIME* lpDateTime);
	void				GetExpirationDate(SYSTEMTIME* lpDateTime);

	void				ResetRepeats(void);	// Turn off and reinit repeat settings.

	FILETIME			GetAlarmTime(void);
	void				GetAlarmTime(LPSYSTEMTIME lpSt);
	void				GetAlarmTime(LPFILETIME lpFt);
	FILETIME			GetTime(void);

	void				KillAlarm(void);
	void				Abort(void) {KillAlarm();}	// Old name for KillAlarm. Compatibility.
	BOOL				IsArmed(void);

protected:
	UFT					m_AlarmTime;		// Wakeup date/time
	LPALARMCB			m_pCB;				// Caller's alarm callback
	DWORD_PTR			m_dwUser;			// User's data for callback
	HWND				m_hwndUser;			// Callers' hwnd for message alerts
	HANDLE				m_hEventUser;		// User's event handle for triggers
	HANDLE				m_hEventAbort;		// Kills any pending wait periods during abort.
	HANDLE				m_hEventWaitForStart;	// Worker waits for this event if no alarm set.
	HANDLE				m_hEventWaitForKill;	// KillAlarm waits for this after issuing m_bKillAlarm

	CSection			m_csRepeats;		// Protect access to this struct.
	REPEAT_PARMS		m_RepeatParms;		// Where the repeat info is kept.

	CSection			m_csAlarm;			// Must hold b4 changing following vars
	BOOL				m_bKillAlarm;		// Tells worker to terminate alarm
	BOOL				m_bKillAlarmThread;	// Tells worker to close
	BOOL				m_bBusy;			// Thread proc is busy measuring time.
	HANDLE				m_hThread;			// Worker thread handle
	HANDLE				m_hTimer;			// Waitable timer handle.

	static unsigned __stdcall CalendarProc(void * arg);
	static void			AlarmTriggered(CAlarmClock* pThis);
	static BOOL			ScheduleRepeat(CAlarmClock* pThis);
	void				StartAlarmThread(void);
	BOOL				KillAlarmThread(void);
	void				Reset(void);
};
