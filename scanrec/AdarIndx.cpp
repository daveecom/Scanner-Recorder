/*********************************************************************
*                                                                    *
* AdarIndx.C                                                         *
*                                                                    *
* All-Day Audio Recorder Index   Functions                           *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*                                                                    *
*********************************************************************/

#include "AdarPch.h"

HWND hindex = NULL;     // Global handle of index dialog

// Using a CFile, add a ReadString capability to it

class CFileEx : public CFile
{
    public:
    int ReadString(LPSTR Str, int Length);
};

int CFileEx::ReadString(LPSTR Str, int Length)
{
    char byte;
    for (int i = 0; i < Length; i++)
    {
        int rc = Read(&byte, 1);
        if (!rc)    // Eof ?
        {
            break;
        }
        else
        {   // Not eof
            Str[i] = byte;      // Save next char
            if (byte == 0x0A)   // LF ?
            {   // Yes, terminate operation
                break;
            }
        }
    }
    if (i == Length)
    {
        Str[Length-1] = 0;       // place terminator
        return Length;
    }
    else
    if (i == 0)
    {   // Handle empty line (eof)
        Str[i] = 0;
        return 0;
    }
    else
    {
        Str[++i] = 0;       // place terminator
        return i;
    }
}


// Display the index list as a modeless dialog box. Called by "Options|Display Index" command.
void DisplayIndex(HANDLE hInst, HWND hParent)
{
    // Create the index list dialog. It will self destruct
    if (!hindex) hindex = CreateDialog((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_DIALOG_INDEX), NULL, DisplayIndexProc);
}




BOOL CALLBACK DisplayIndexProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static char text[256];
    int index;
    static int idListBox;
    static HWND hwndListBox; 
    LRESULT rc;
	char Text[128];
	int length;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Align dialog in upper left corner
            SetWindowPos(hwndDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE);

            // Prevent unwanted system menu commands
            HMENU hSysMenu = GetSystemMenu(hwndDlg, FALSE);
            _ASSERT(hSysMenu);
            EnableMenuItem(hSysMenu, SC_MAXIMIZE, MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
            EnableMenuItem(hSysMenu, SC_SIZE,     MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);

            CheckDlgButton(hwndDlg, IDC_CHECK_AUTOSCROLL, LogScrollOff ? 1 : 0);

            // Initialize the index list box display
            InitIndexLB(hwndDlg, IDC_LIST1);

            return TRUE;
        }

        case WM_SYSCOMMAND:
        {
            switch (wParam)
            {
                case SC_CLOSE:
                {
                    DestroyWindow(hwndDlg);
                    return TRUE;
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            LogScrollOff = IsDlgButtonChecked(hwndDlg, IDC_CHECK_AUTOSCROLL);   // Save it
            hindex = NULL;
            break;
        }


        case ADD_RECORD:
        {
            strncpy(Text, (LPCTSTR) lParam, sizeof Text); // make a copy of the new item
            length = (int) strlen(Text);
            if (length > 3)
            {
                Text[length - 2] = '\0';  // remove CRLF
                LRESULT i = SendDlgItemMessage(hindex, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) &Text);
                if (i != LB_ERR)
                {
                    if (!LogScrollOff)
                    {
                        rc = SendDlgItemMessage(hindex, IDC_LIST1, LB_SETCURSEL, i, 0);  // Select the new item
                        _ASSERT(rc != LB_ERR);
                    }
                }
            }
            return TRUE;
        }

        case CLEAR_LOG_DISPLAY:
        {
            // Reset the listbox
            rc = SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_RESETCONTENT, 0, 0);
            break;
        }

        case WM_COMMAND:
        {   // handle messages from the listbox control
            int id = LOWORD(wParam);        // id of control
            int notify = HIWORD(wParam);    // notification code
            HWND handle = (HWND) lParam;    // handle of control

//          trace("WM_COMMAND: notify=%d, hwnd=%d\n", notify ,handle);
//          trace("WM_COMMAND: wParam=%08X, lParam=%08X, id=%d, notify=%d, hwnd=%08X\n", wParam, lParam, id, notify, handle);
            
            switch (id)
            {
                case IDC_CHECK_AUTOSCROLL:
                {
                    LogScrollOff = IsDlgButtonChecked(hwndDlg, IDC_CHECK_AUTOSCROLL);
                    TRACE("LogScrollOff = %d\n", LogScrollOff);
                    break;
                }
                
                case IDC_LIST1:
                {   // from the index listbox
                    switch (notify)
                    {
                        case LBN_SELCHANGE:
                        {
                            idListBox = (int) LOWORD(wParam);
                            hwndListBox = (HWND) lParam; 
                            trace("LBN_SELCHANGE:\n");

                            // get index of selected item
                            index = (int) SendDlgItemMessage(hwndDlg, idListBox, LB_GETCURSEL, 0, 0);
                            rc = SendDlgItemMessage(hwndDlg, idListBox, LB_GETTEXT, index, (LPARAM) &text);  // get selected text
                            trace("Sel Chng:'%s'\n", &text);
                            rc = SendDlgItemMessage(hwndDlg, IDC_STATIC_TEXT1, WM_SETTEXT, 0, (LPARAM) &text);  // get selected text
                            return TRUE;
                        }
                    }
                    break;
                }

                case IDC_REFRESH:
                {
                    // Initialize the index list box display
                    if (InitIndexLB(hwndDlg, IDC_LIST1) != 0)
                    {
                        DestroyWindow(hwndDlg);
                        return TRUE;
                    }
                }
            }   // switch
            
            break;
        }
    
        default:
        return FALSE;   // Message not recognized
    }
    return FALSE;
}


// Initialize the list box
int InitIndexLB(HWND hwnd, int id)
{
    long rc;
    char Text[128];
    int length;     // length of index file
    CFileEx IndexFile;
	OPENFILENAME ofn;
    static char szFile[MAX_PATH];
    static char szFileTitle[MAX_PATH];
    
    //int i;

    strcpy( szFile, "");            // Clear result areas
    strcpy( szFileTitle, "");


//  memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize 		= sizeof(OPENFILENAME);
    ofn.hwndOwner 			= hwndd;
    ofn.hInstance 			= hinst;
    ofn.lpstrFilter 		= "Log Files (*.LOG)\0*.LOG\0";
    ofn.lpstrCustomFilter 	= NULL;
    ofn.nMaxCustFilter 		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFile;
    ofn.nMaxFile			= sizeof szFile;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= sizeof szFileTitle;
    ofn.lpstrInitialDir		= NULL;         // check this if problem
    ofn.lpstrTitle			= "Open Log File";
    ofn.Flags				= OFN_HIDEREADONLY;
    ofn.nFileOffset			= 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = "LOG";
    ofn.lCustData			= 0;
    ofn.lpfnHook			= NULL;
    ofn.lpTemplateName		= NULL;

    //i = GetOpenFileName(&ofn);                              // Invoke the DLL routine
    //if (!i)  return 1;  // OK button not pressed

    // Copy the file name into the caller's buffer
    //strncpy(Text, szFile, sizeof Text);


    rc = IndexFile.Open(IndexFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone);
    if (rc)
    {
        // Reset the listbox
        rc = (long) SendDlgItemMessage(hwnd, id, LB_RESETCONTENT, 0, 0);

        if ((length = (int) IndexFile.GetLength()) == 0)
        {   // file length zero
            MessageBox(hwnd, "Log file is empty.", NULL, MB_OK);
            return 1;   // error
        }
        else
        {
            // Discard header lines
            IndexFile.ReadString(Text, sizeof Text);
            IndexFile.ReadString(Text, sizeof Text);

            // display index records
            for (;;)
            {
                int rc = IndexFile.ReadString(Text, sizeof Text);
                if (!rc) break;
                Text[rc-2] = 0; // Chop off CRLF at the end of string
            
                rc = (int) SendDlgItemMessage(hwnd, id, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) &Text);
                if (rc == LB_ERRSPACE || rc == LB_ERR)
                {
                    MessageBox(hwnd, "LB_ADDSTRING return code error. Use debugger to view error code: rc.",
                        "Listbox Error", MB_OK);
                    DebugBreak();
                }
            }
        }
        return 0;   // Normal return
    }
    else return 1;  // else error
}
