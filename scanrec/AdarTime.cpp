//////////////////////////////////////////////////////////////////////////////
// Time adjustment dialog handler
//////////////////////////////////////////////////////////////////////////////

#include "AdarPch.h"


// Time setting dialog proc.
//
// Except in response to the WM_INITDIALOG message, the dialog box procedure
// should return nonzero if it processes the message, and zero if it does not.
// In response to a WM_INITDIALOG message, the dialog box procedure should return zero
// if it calls the SetFocus function to set the focus to one of the controls in the
// dialog box. Otherwise, it should return nonzero, in which case the system sets
// the focus to the first control in the dialog box that can be given the focus. 
BOOL CALLBACK TimeDlgProc(HWND hDlg, UINT Msg, WPARAM wParam,LPARAM lParam)
{
    WORD wNotifyCode;       // notification code 
    WORD wID;               // item, control, or accelerator identifier 
    HWND hCtl;				// handle of control 
	LPNMHDR pnmh;			// Pointer to notify info structure
	SYSTEMTIME* pTime;		// Caller's time value
//	WORD wYear;
//	WORD wMonth;
//	WORD wDayOfWeek;
//	WORD wDay;
//	WORD wHour;
//	WORD wMinute;
//	WORD wSecond;
//	WORD wMilliseconds;
	_TCHAR Buffer[128];		// used by conversions such as itoa

	switch (Msg)
	{
		case WM_INITDIALOG:
			pTime = (SYSTEMTIME*) lParam;	// Access caller's time

			// Refresh the dialog to show the initial values passed by the caller
			SendDlgItemMessage(hDlg, IDC_EDIT_TIME_HH, WM_SETTEXT, 0, (LPARAM) itoa(pTime->wHour, Buffer, 10));
			SendDlgItemMessage(hDlg, IDC_EDIT_TIME_MM, WM_SETTEXT, 0, (LPARAM) itoa(pTime->wMinute, Buffer, 10));
			SendDlgItemMessage(hDlg, IDC_EDIT_TIME_SS, WM_SETTEXT, 0, (LPARAM) itoa(pTime->wSecond, Buffer, 10));

			return TRUE;
			break;

		case WM_COMMAND:
            wNotifyCode = HIWORD(wParam);	// notification code 
            wID = LOWORD(wParam);			// item, control, or accelerator identifier 
            hCtl = (HWND) lParam;			// handle of control 

			switch (wID)
			{
				case IDOK:
					EndDialog(hDlg, IDOK);
					return TRUE;
					break;

				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
					break;

			}
			break;

		case WM_NOTIFY:
			wID = (int) wParam;			// Get unit id
			pnmh = (LPNMHDR) lParam;	// Get notify info

			if (pnmh->code == UDN_DELTAPOS)
			{
				NM_UPDOWN* pnmud = (NM_UPDOWN FAR *) lParam;	// Get up/dwn info
				int pos = pnmud->iPos;
				int delta = pnmud->iDelta;
				trace("UpDown Notify, Pos = '%d', Delta = '%d'.\n", pos, delta);

			}

			break;



		case UDN_DELTAPOS:	// Up-down button changed
			break;

	}	// switch (Msg)
	
	return FALSE;	// Return without processing the current message.
}

// Given a resource id, use the template to get a time value from the user.
// Returns TRUE if OK pressed.
BOOL AskForTime(HWND hParent, SYSTEMTIME* pTime)
{
	INT_PTR result;
	SYSTEMTIME time = *pTime;

	// Do Modal and pass the user's SYSTEMTIME to the dlg
	result = DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_DIALOG_SETTIME), hParent, TimeDlgProc, (LPARAM) &time);

	if (result == IDOK)
	{	// Return value to caller.
		*pTime = time;
		return TRUE;
	}
	else
		return FALSE;
}
