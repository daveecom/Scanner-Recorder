#undef	_WIN32_WINDOWS
#define	_WIN32_WINDOWS 0x0410

#include <windows.h>
#include <crtdbg.h>
#include <process.h>
#include <mmsystem.h>
#include <assert.h>
#include ".\alarmclock.h"

CAlarmClock::CAlarmClock(void)
: m_bKillAlarm(FALSE)
, m_bKillAlarmThread(FALSE)
, m_hThread(NULL)
, m_pCB(NULL)
, m_dwUser(0)
, m_bBusy(FALSE)
{
	m_hEventAbort			= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEventWaitForStart	= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEventWaitForKill		= CreateEvent(NULL, FALSE, FALSE, NULL);

	Reset();

}

CAlarmClock::~CAlarmClock(void)
{
	KillAlarm();
	KillAlarmThread();

	CloseHandle(m_hEventAbort);
	CloseHandle(m_hEventWaitForStart);
	CloseHandle(m_hEventWaitForKill);
}

// This is the clock's motor and gears. Keep track of time, handle all alarms,
// process alarm actions and schedules repeat alarms.
unsigned CAlarmClock::CalendarProc(void * arg)
{
	CAlarmClock*	pThis = (CAlarmClock*) arg;
	UFT				uLocal;
	UINT			sleepTimeMs;
	LARGE_INTEGER	li;
	BOOL			rc;
	DWORD			dwResult;
	HANDLE			hWaits[3];		// Aborts time/wait.
	HANDLE			hWaitIdle[2];	// Aborts idle/wait.

	hWaits[0]		= CreateWaitableTimer(NULL, FALSE, NULL);
	hWaits[1]		= pThis->m_hEventAbort;

	hWaitIdle[0]	= pThis->m_hEventWaitForStart;
	hWaitIdle[1]	= pThis->m_hEventAbort;

	while (!pThis->m_bKillAlarmThread)
	{
		if (!pThis->m_bBusy)
		{	// Wait here until we're signalled by SetAlarm
			WaitForMultipleObjects(2, hWaitIdle, FALSE, INFINITE);
			if (pThis->m_bKillAlarmThread) break;	// Exit loop and kill this thread.
		}

		// Loop to check for time expiration
		while(!pThis->m_bKillAlarm)
		{
			// Get local time
			GetTime(&uLocal);

			// Compare local time with alarm time.
			if (uLocal.ll >= pThis->m_AlarmTime.ll)
			{	// Alarm time has been reached. Handle the action.
				pThis->AlarmTriggered(pThis);

				{
					if (!ScheduleRepeat(pThis))
					//if (!ScheduleRepeat_OLD(pThis))
					{
						pThis->m_bKillAlarm = TRUE;
						break;	// Kill the time loop.
					}
					else
					{	// Repeat is scheduled. Go redrive the alarm loop.
						continue;	// Go repeat using the new alarm time.
					}
				}
			}
			else
			{	// Need to prepare the interval timer for a period of time to sleep.

				// See if more than one second until the alarm is reached.
				pThis->GetTime(&uLocal);

				if (pThis->m_AlarmTime.ll <= (uLocal.ll + ONE_SECOND))
				{	// One second or less remaining before alarm.
					sleepTimeMs = 10;	// Select fine interval for precision.
				}
				else
				{	// More than one second remaining.
					sleepTimeMs = 1000;	// Select course interval.
				}
			}

			// We're to sleep here until the event is signalled.
			if (!pThis->m_bKillAlarm)
			{
				li.QuadPart = -((LONGLONG)sleepTimeMs);
				rc = SetWaitableTimer(hWaits[0], &li, 0, NULL, NULL, FALSE);
				_ASSERT(rc);

				// Sleep here until the timer interval ends.
				dwResult = WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);
				_ASSERT(dwResult == WAIT_OBJECT_0 || dwResult == WAIT_OBJECT_0 + 1);
			}
		}	// Main clock loop

		// Alarm time has been reached. This is where we return the object to idle

		pThis->ResetRepeats();
		pThis->m_AlarmTime.ll = 0;
		pThis->m_bBusy = FALSE;
		SetEvent(pThis->m_hEventWaitForKill);
	}

	CloseHandle(hWaits[0]);

	return 0;
}

// See if there's a need to repeat an alarm. If so, set up the new alarm time
// and return TRUE to tell the time loop to keep running. Else return FALSE
// if no more repeats which allows the time loop to quit.
BOOL CAlarmClock::ScheduleRepeat(CAlarmClock* pThis)
{
	UFT			u1;
	BOOL		bRepeatUntil;
	BOOL		bResult = FALSE;

	CCS	Lock(&pThis->m_csRepeats);

	_ASSERT(pThis);

	// Get local DT in order to check for expiration DT.
	GetTime(&u1);

	// If there is a non-zero m_RepeatParms.tRepeatUntil then we will use expiration date and
	// ignore the other repeat-end-conditions.
	bRepeatUntil = pThis->m_RepeatParms.tRepeatUntil.ll != 0ll;

	if
	(
		bRepeatUntil && (pThis->m_RepeatParms.tRepeatUntil.ll > u1.ll) ||	// 1st cond'n: exp DT
		pThis->m_RepeatParms.bRepeatForever ||								// 2nd
		pThis->m_RepeatParms.nRepeatCount > 0
	)
	{
		pThis->m_AlarmTime.ft = WhenIsNextAlarm(pThis->m_AlarmTime.ft, pThis->m_RepeatParms);
		if(pThis->m_AlarmTime.ll == 0ll)
			bResult = FALSE;
		else
			bResult = TRUE;

		if (!bRepeatUntil && !pThis->m_RepeatParms.bRepeatForever)
		{
			pThis->m_RepeatParms.nRepeatCount--;
		}
	}

	return bResult;
}

// static: Given a DT and a REPEAT_PARMS, calculate the next alarm event DT that
// will happen in the future. If there are no more events eligible then return 0.
FILETIME CAlarmClock::WhenIsNextAlarm(const FILETIME DTin, const REPEAT_PARMS RepeatParms)
{
	UFT				uNextAlarm = {0};
	UFT				u1, uCurrent;
	SYSTEMTIME		st;
	BOOL			bRepeatUntil;
	REPEAT_PARMS	parms = RepeatParms;	// Make copy of R/O object
	int				nDaysFromNow;	// used by WD calc

	u1.ft = DTin;

	// If there is a non-zero m_RepeatParms.tRepeatUntil then we will use expiration date and
	// ignore the other repeat-end-conditions.
	bRepeatUntil = parms.tRepeatUntil.ll != 0ll;

	// See if the caller's DT has expired. If not then there's nothing to do but return their DT back.
	GetTime(&uCurrent);

	if (u1.ll > uCurrent.ll)
	{	// Not expired yet. No need to check REPEAT_PARMS
		uNextAlarm.ft = DTin;
	}
	else
	{	// Caller's DT has Expired. We need to calc a new alarm
		if (parms.bRepeatForever || parms.nRepeatCount > 0 || bRepeatUntil)
		{
			// See if there's an expired repeat condition.
			if (bRepeatUntil)
			{	// Handle repeat until expiration date.
				if (u1.ll >= parms.tRepeatUntil.ll)
				{
					uNextAlarm.ll = 0;	// Expiration time reached
					return uNextAlarm.ft;
				}
			}
			else
			{	// Handle repeat forever or counter.
				if (parms.nRepeatCount == 0 && !parms.bRepeatForever)
				{
					uNextAlarm.ll = 0;	// Expiration reached
					return uNextAlarm.ft;
				}
			}

			switch (parms.Type)
			{
				case Repeat_Off:

					//uNextAlarm.ll = 0;
					uNextAlarm.ft = DTin;
					break;

				case Repeat_Interval:

					for (;u1.ll <= uCurrent.ll;)	// Catch up loop
					{
						u1.ll	+=	parms.dd	* ONE_DAY;
						u1.ll	+=	parms.hh	* ONE_HOUR;
						u1.ll	+=	parms.mm	* ONE_MINUTE;
						u1.ll	+=	parms.ss	* ONE_SECOND;
						u1.ll	+=	parms.ms	* ONE_MILLISECOND;
					}
					uNextAlarm = u1;		// Give new alarm to caller

					break;

				case Repeat_Monthly:

					for (;u1.ll < uCurrent.ll;)	// Catch up loop
					{
						u1.ll += ONE_DAY;				// Start scan "tomorrow".

						// Loop each day and check the DOM from the st.
						// If no matching DOM then keep scanning. It might've been Feb 29th.
						for (;;)
						{
							FileTimeToSystemTime(&u1.ft, &st);	// Convert test time to get DOM
							if (st.wDay == parms.RepeatDOM)
								break;	// Found mtaching DOM

							// No matching DOM. Keep adding a day at a time until we find it.
							u1.ll += ONE_DAY;
						}
					}
					uNextAlarm = u1;

					break;

				case Repeat_Weekdays:

					for (;u1.ll < uCurrent.ll;)
					{
						// Get the current DOW from the callers DT
						FileTimeToSystemTime(&u1.ft, &st);
						int todayWD = ((st.wDayOfWeek + 1) % 7);	// Increment DOW to "tomorrow"

						// Scan from tomorrow forward until we get a WD flag
						for (int i = todayWD, count = 1;; count++, i++)
						{
							i %= 7;	// Wrap around the weekend
							if (parms.RepeatWD[i])	// If we found a weekday to set at
							{
								nDaysFromNow = count;
								break;
							}
						}

						// Convert to ft then add the offset to make the next alarm time.
						SystemTimeToFileTime(&st, &u1.ft);
						u1.ll += (nDaysFromNow * ONE_DAY);

					}
					uNextAlarm = u1;	// Pass result to caller

					break;
			}

		}
	}

	return uNextAlarm.ft;
}

// Set alarm using FILETIME and Callback
BOOL CAlarmClock::SetAlarm(FILETIME Time, LPALARMCB pCB, DWORD_PTR dwUser)
{
	CCS	Lock(&m_csAlarm);

	if (IsArmed()) return FALSE;	// Busy

	m_AlarmTime.ft	= Time;	// Save the desired wakeup time.
	m_pCB		= pCB;
	m_dwUser	= dwUser;
	m_bKillAlarm	= FALSE;
	m_bBusy		= TRUE;

	if (m_hThread == NULL) StartAlarmThread();
	SetEvent(m_hEventWaitForStart);

	return TRUE;
}

// Set alarm using SYSTEMTIME and Callback
BOOL CAlarmClock::SetAlarm(SYSTEMTIME Time, LPALARMCB pCB, DWORD_PTR dwUser)
{
	FILETIME	ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(pCB != NULL);

	SystemTimeToFileTime(&Time, &ft);
	return SetAlarm(ft, pCB, dwUser);
}

// Set an alarm at a given number of units from now in the future. Ex: 20 days, 12 hours.
BOOL CAlarmClock::SetAlarm(int DD, int HH, int MM, int SS, int MS, LPALARMCB pCB, DWORD_PTR dwUser)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(pCB != NULL);

	GetTime(&ft);
	ft.ll		+= DD * ONE_DAY;
	ft.ll		+= HH * ONE_HOUR;
	ft.ll		+= MM * ONE_MINUTE;
	ft.ll		+= SS * ONE_SECOND;
	ft.ll		+= MS * ONE_MILLISECOND;

	return SetAlarm(ft.ft, pCB, dwUser);
}

// Set alarm using FILETIME and HWND message
BOOL CAlarmClock::SetAlarm(FILETIME Time, HWND hwndUser, DWORD_PTR dwUser)
{
	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hwndUser != NULL);

	m_hEventUser	= NULL;
	m_hwndUser		= hwndUser;
	m_dwUser		= dwUser;

	return SetAlarm(Time, (LPALARMCB) NULL, dwUser);
}

// Set an alarm at a given number of units from now in the future. Ex: 20 days, 12 hours.
BOOL CAlarmClock::SetAlarm(int DD, int HH, int MM, int SS, int MS, HWND hwndUser, DWORD_PTR dwUser)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hwndUser != NULL);

	GetTime(&ft);
	ft.ll		+= DD * ONE_DAY;
	ft.ll		+= HH * ONE_HOUR;
	ft.ll		+= MM * ONE_MINUTE;
	ft.ll		+= SS * ONE_SECOND;
	ft.ll		+= MS * ONE_MILLISECOND;

	return SetAlarm(ft.ft, hwndUser, dwUser);
}

BOOL CAlarmClock::SetAlarm(SYSTEMTIME Time, HWND hwndUser, DWORD_PTR dwUser)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hwndUser != NULL);

	SystemTimeToFileTime(&Time, &ft.ft);

	return SetAlarm(ft.ft, hwndUser, dwUser);
}

BOOL CAlarmClock::SetAlarm(FILETIME Time, HANDLE hEvent, DWORD_PTR dwUser)
{
	CCS	Lock(&m_csAlarm);

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hEvent != NULL);

	m_hEventUser	= hEvent;
	m_dwUser		= dwUser;
	m_AlarmTime.ft	= Time;
	m_bKillAlarm	= FALSE;
	m_bBusy			= TRUE;
	
	if (m_hThread == NULL) StartAlarmThread();
	SetEvent(m_hEventWaitForStart);

	return TRUE;
}

BOOL CAlarmClock::SetAlarm(SYSTEMTIME Time, HANDLE hEvent, DWORD_PTR dwUser)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hEvent != NULL);

	SystemTimeToFileTime(&Time, &ft.ft);

	return SetAlarm(ft.ft, hEvent, dwUser);
}

// Set an alarm at a given number of units from now in the future. Ex: 20 days, 12 hours.
BOOL CAlarmClock::SetAlarm(int DD, int HH, int MM, int SS, int MS, HANDLE hEvent, DWORD_PTR dwUser)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy
	_ASSERT(hEvent != NULL);

	GetTime(&ft);
	ft.ll		+= DD * ONE_DAY;
	ft.ll		+= HH * ONE_HOUR;
	ft.ll		+= MM * ONE_MINUTE;
	ft.ll		+= SS * ONE_SECOND;
	ft.ll		+= MS * ONE_MILLISECOND;

	return SetAlarm(ft.ft, hEvent, dwUser);
}

BOOL CAlarmClock::SetAlarmAndWait(FILETIME Time)
{
	HANDLE	hEvent;
	BOOL	rc;
	HANDLE	hWaits[2];

	if (IsArmed()) return FALSE;	// Busy

	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_ASSERT(hEvent != NULL);

	rc = SetAlarm(Time, hEvent);
	_ASSERT(rc);

	hWaits[0]	= hEvent;
	hWaits[1]	= m_hEventAbort;

	ResetEvent(m_hEventAbort);

	// Wait here until the alarm is triggered or aborted.
	WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);

	CloseHandle(hEvent);

	return !m_bKillAlarm;	// TRUE = time expired, FALSE = aborted.
}

BOOL CAlarmClock::SetAlarmAndWait(SYSTEMTIME Time)
{
	UFT		ft;

	if (IsArmed()) return FALSE;	// Busy

	SystemTimeToFileTime(&Time, &ft.ft);

	return SetAlarmAndWait(ft.ft);
}

BOOL CAlarmClock::SetAlarmAndWait(int DD, int HH, int MM, int SS, int MS)
{
	UFT			ft;

	if (IsArmed()) return FALSE;	// Busy

	GetTime(&ft);
	ft.ll		+= DD * ONE_DAY;
	ft.ll		+= HH * ONE_HOUR;
	ft.ll		+= MM * ONE_MINUTE;
	ft.ll		+= SS * ONE_SECOND;
	ft.ll		+= MS * ONE_MILLISECOND;

	return SetAlarmAndWait(ft.ft);
}

BOOL CAlarmClock::KillAlarmThread(void)
{
	CCS	Lock(&m_csAlarm);

	if (m_hThread == NULL) return FALSE;

	m_bKillAlarmThread = TRUE;
	SetEvent(m_hEventAbort);

	if (m_hThread != NULL)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	return TRUE;
}

BOOL CAlarmClock::IsArmed(void)
{
	return (m_bBusy);
}

// Reset this object to its initial state.
void CAlarmClock::Reset(void)
{
	KillAlarmThread();

	m_hEventUser	= NULL;
	m_hwndUser		= NULL;
	m_dwUser		= 0;
	m_pCB			= NULL;

	m_AlarmTime.ll	= 0;

	ResetRepeats();
}

// Used to terminate an alarm in progress.
void CAlarmClock::KillAlarm(void)
{
	CCS Lock(&m_csAlarm);

	ResetEvent(m_hEventWaitForKill);

	if (m_bBusy)
	{
		m_bKillAlarm = TRUE;
		SetEvent(m_hEventAbort);
		WaitForSingleObject(m_hEventWaitForKill, INFINITE);
		m_bKillAlarm = FALSE;
	}
}

// Return all repeat controls to their initial states.
void CAlarmClock::ResetRepeats(void)
{
	CCS	Lock(&m_csRepeats);

	ZeroMemory(&m_RepeatParms, sizeof(REPEAT_PARMS));
}

// All expired alarms will eventually call this function to reflect the
// status back to the user.
void CAlarmClock::AlarmTriggered(CAlarmClock* pThis)
{
	_ASSERT(pThis);

	if (pThis->m_hEventUser != NULL)
	{
		SetEvent(pThis->m_hEventUser);
	}
	else
	if (pThis->m_hwndUser != NULL)
	{
		::PostMessage(pThis->m_hwndUser, UWM_ALARM, (WPARAM) pThis, (LPARAM) pThis->m_dwUser);
	}
	else
	if (pThis->m_pCB != NULL)
	{
		pThis->m_pCB(pThis, pThis->m_dwUser);
	}
}

// Validate and apply repeat parameters.
BOOL CAlarmClock::SetRepeat(REPEAT_PARMS Parms)
{
	int		i, count;
	BOOL	bInvalid	= FALSE;

	CCS	Lock(&m_csAlarm);

	if (Parms.bRepeatForever == FALSE && Parms.nRepeatCount == 0 && Parms.tRepeatUntil.ll == 0)
		bInvalid = TRUE;


	switch (Parms.Type)
	{
		case Repeat_Off:
			break;

		case Repeat_Interval:
			if (Parms.dd > 365 || Parms.hh > 24 || Parms.mm > 60 || Parms.ss > 60 || /* Parms.ms > 999 || */
				(Parms.dd + Parms.hh + Parms.mm + Parms.ss + Parms.ms == 0) )
				bInvalid = TRUE;
			break;

		case Repeat_Monthly:
			if (!(Parms.RepeatDOM >= 1 && Parms.RepeatDOM <= 31))
				bInvalid = TRUE;
			break;

		case Repeat_Weekdays:
			// Make sure the array is valid boolean TRUE or FALSE only
			for (i = 0, count = 0; i < 7; i++)
			{
				if (!(Parms.RepeatWD[i] == TRUE || Parms.RepeatWD[i] == FALSE))
				{
					_ASSERT(0);	// Invalid data in array, not TRUE nor FALSE
					bInvalid	= TRUE;
				}
				if (Parms.RepeatWD[i])
					count++;
			}

			if (count == 0) bInvalid = TRUE;	// No weekdays were selected.

			break;
	}

	// If everything above checks out then save the parms.
	if (!bInvalid)
	{
		CCS	Lock(&m_csRepeats);

		m_RepeatParms = Parms;
		return TRUE;
	}
	else
	{
		//_ASSERT(0);		// Something in the RepeatParms is set wrong.
		return FALSE;	// Invalid parameter(s) detected.
	}
}

FILETIME CAlarmClock::GetAlarmTime(void)
{
	return m_AlarmTime.ft;
}

void CAlarmClock::GetAlarmTime(LPSYSTEMTIME lpSt)
{
	FileTimeToSystemTime(&m_AlarmTime.ft, lpSt);
}

void CAlarmClock::GetAlarmTime(LPFILETIME lpFt)
{
	*lpFt = m_AlarmTime.ft;
}

BOOL CAlarmClock::SetRepeat(long nCount, int nDays, int nHours, int nMinutes, int nSeconds, int nMilliseconds)
{
	REPEAT_PARMS		parms;

	ZeroMemory(&parms, sizeof parms);

	parms.Type			= Repeat_Interval;
	parms.nRepeatCount	= nCount;
	parms.dd			= nDays;
	parms.hh			= nHours;
	parms.mm			= nMinutes;
	parms.ss			= nSeconds;
	parms.ms			= nMilliseconds;

	parms.bRepeatForever = (nCount == 0) ? TRUE:FALSE;

	return SetRepeat(parms);
}

BOOL CAlarmClock::SetRepeat(long nCount, int DayOfMonth)
{
	REPEAT_PARMS		parms;

	ZeroMemory(&parms, sizeof parms);

	parms.Type			= Repeat_Monthly;
	parms.nRepeatCount	= nCount;
	parms.RepeatDOM		= DayOfMonth;

	parms.bRepeatForever = (nCount == 0) ? TRUE:FALSE;

	return SetRepeat(parms);
}

BOOL CAlarmClock::SetRepeat(long nCount, BOOL bDay[7])
{
	REPEAT_PARMS		parms;

	ZeroMemory(&parms, sizeof parms);

	parms.Type			= Repeat_Weekdays;
	parms.nRepeatCount	= nCount;

	for (int i = 0; i < 7; i++) parms.RepeatWD[i] = bDay[i];

	parms.bRepeatForever = (nCount == 0) ? TRUE:FALSE;

	return SetRepeat(parms);
}

REPEAT_PARMS CAlarmClock::GetRepeat(void)
{
	return m_RepeatParms;
}

void CAlarmClock::GetRepeat(REPEAT_PARMS* pParms)
{
	*pParms = m_RepeatParms;
}

void CAlarmClock::SetExpirationDate(FILETIME* lpDateTime)
{
	CCS	Lock(&m_csRepeats);

	m_RepeatParms.tRepeatUntil.ft	= *lpDateTime;
	m_RepeatParms.bRepeatForever	= FALSE;
	m_RepeatParms.nRepeatCount		= 0;
}

void CAlarmClock::SetExpirationDate(SYSTEMTIME* lpDateTime)
{
	CCS	Lock(&m_csRepeats);

	SystemTimeToFileTime(lpDateTime, &m_RepeatParms.tRepeatUntil.ft);
	m_RepeatParms.bRepeatForever	= FALSE;
	m_RepeatParms.nRepeatCount		= 0;
}

void CAlarmClock::GetExpirationDate(FILETIME* lpDateTime)
{
	*lpDateTime = m_RepeatParms.tRepeatUntil.ft;
}

void CAlarmClock::GetExpirationDate(SYSTEMTIME* lpDateTime)
{
	FileTimeToSystemTime(&m_RepeatParms.tRepeatUntil.ft, lpDateTime);
}

// Statics to get local time in different formats.
void CAlarmClock::GetTime(UFT* pUFT)
{
	SYSTEMTIME	tm;
	UFT			u1;

	::GetLocalTime(&tm);
	SystemTimeToFileTime(&tm, &u1.ft);

	*pUFT = u1;
}

void CAlarmClock::GetTime(FILETIME* pFT)
{
	UFT			u1;

	GetTime(&u1);
	*pFT = u1.ft;
}


FILETIME CAlarmClock::GetTime(void)
{
	UFT	uCurrent;

	GetTime(&uCurrent);

	return uCurrent.ft;
}

void CAlarmClock::StartAlarmThread(void)
{
	unsigned int threadid;

	ResetEvent(m_hEventAbort);
	ResetEvent(m_hEventWaitForStart);

	m_bKillAlarm		= FALSE;
	m_bKillAlarmThread	= FALSE;

	m_hThread = (HANDLE) _beginthreadex(0, 0, CalendarProc, this, 0, &threadid);
}

