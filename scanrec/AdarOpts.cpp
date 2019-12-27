///////////////////////////////////////////////////////////////////////////////
// Scanner Recorder options
///////////////////////////////////////////////////////////////////////////////

#include "AdarPch.h"

///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK OptionsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	char buffer[10];
	int wNotifyCode, wID;
	HWND hwndCtl;
	BOOL flag;
//	PIPVARS	localPip;


	switch (uMsg)
	{
		case WM_INITDIALOG:
			CheckDlgButton(hDlg, IDC_CHECK_USE_COMPRESSION,
				GetPrivateProfileInt(IniSectionName, "Compression", 1, IniName) ? BST_CHECKED : BST_UNCHECKED);
				SquelchDelayTimeMs = GetPrivateProfileInt(IniSectionName, "SquelchDelayTimeMs", 1000, IniName);
				SetDlgItemInt(hDlg, IDC_OPTIONS_EDIT_SQDLY, SquelchDelayTimeMs, FALSE);

			CheckDlgButton(hDlg, IDC_CHECK_ENABLEPIP, GetPrivateProfileInt(IniSectionName, "pip", 0, IniName) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_CHECK_ENABLEANTICLIP, GetPrivateProfileInt(IniSectionName, "AntiClip", 1, IniName) ? BST_CHECKED : BST_UNCHECKED);

			return FALSE;
			break;

		case WM_COMMAND:
			wNotifyCode = HIWORD(wParam); // notification code 
			wID = LOWORD(wParam);         // item, control, or accelerator identifier 
			hwndCtl = (HWND) lParam;      // handle of control 

			switch (wID)
			{
				case IDOK:
					WritePrivateProfileString(IniSectionName, "Compression", itoa(IsDlgButtonChecked(hDlg, IDC_CHECK_USE_COMPRESSION) == BST_CHECKED, buffer, 10), IniName);
					WritePrivateProfileString(IniSectionName, "Pip", itoa(IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLEPIP) == BST_CHECKED, buffer, 10), IniName);
					WritePrivateProfileString(IniSectionName, "AntiClip", itoa(IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLEANTICLIP) == BST_CHECKED, buffer, 10), IniName);
					EndDialog(hDlg, IDOK);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;

				case IDC_OPTIONS_EDIT_SQDLY:
					switch(wNotifyCode)
					{
						case EN_KILLFOCUS:
							DWORD tmp = GetDlgItemInt(hDlg, IDC_OPTIONS_EDIT_SQDLY, &flag, FALSE);
							if (flag)
							{	// Success reading squelch delay value
								tmp = max(30, tmp);		// Prevent values less than 30 ms
								SquelchDelayTimeMs = tmp;
								WritePrivateProfileString(IniSectionName, "SquelchDelayTimeMs", ltoa(tmp, buffer, 10), IniName);
							}
							SetWindowText(hwndCtl, ltoa(tmp, buffer, 10));

							
							return TRUE;
					}

					return FALSE;

				case IDC_EDIT_FREQ:
				{
					switch (wNotifyCode)
					{
						case EN_KILLFOCUS:
//							localPip.Freq = GetDlgItemInt(IDC_EDIT_FREQ, &errorFlag, FALSE);




							return TRUE;
					}					
				}
			
			
			
			}

			break;
	}

	return FALSE;	// Message not handled by us
}

