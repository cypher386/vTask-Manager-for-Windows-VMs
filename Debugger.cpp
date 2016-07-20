#define STRICT
#include "Debugger.h"


//Author: Kannan Balasubramanian
//Last Updated date: June 10, 2008

//Pending: Timer based refresh for all
//Pending: Prevent Double Loading of Process Info ListViews
//Pending: Add all the module size displayed in the Module list ( THAPI & PSAPI ) and put a static in the bottom
//Pending: Handle TCP/IP Protocol

VOID DumpColumnWidth ( HWND hwnd, INT nCol )
{
  INT wd;
  char szBuf[1024];
  *szBuf = NULL;
  for ( INT i = 0; i < nCol; i++ )
  {
    wd = ListView_GetColumnWidth ( hwnd, i );
    wsprintf ( szBuf + lstrlen ( szBuf ), "%d = %d\r\n", i, wd );
  }
  CheckAndDisplay ( szBuf, lstrlen(szBuf ) );
}

//General Functions

BOOL StartCommunicationServers ()
{
  SECURITY_ATTRIBUTES sa;
  DWORD dwThreadId;

  ZeroMemory ( &sa, sizeof(sa) );
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  ghSwitchEditEvent = CreateEvent ( &sa, TRUE, TRUE, "DebuggerSymaphore" );

  ghPipe = CreateNamedPipe ( gszServerName, PIPE_ACCESS_INBOUND | FILE_FLAG_WRITE_THROUGH, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE  | PIPE_NOWAIT,
                             PIPE_UNLIMITED_INSTANCES, 1024, 1024, 1000, &sa );
  dwThreadId = 0;
  ghPipeThread = CreateThread ( &sa, NULL, PipeThreadFunc, NULL, 0, &dwThreadId );

  WSAData wsad;
  WSAStartup ( MAKEWORD(1,1), &wsad );
  dwThreadId = 0;
  ghWinSockTCPIPThread = CreateThread ( &sa, NULL, TCPIPWinSockThreadFunc, NULL, 0, &dwThreadId );
  ghWinSockTCPIPEvent = WSACreateEvent ();

  dwThreadId = 0;
  ghWinSockUDPThread = CreateThread ( &sa, NULL, UDPWinSockThreadFunc, NULL, 0, &dwThreadId );
  ghWinSockUDPEvent = WSACreateEvent ();

  return TRUE;
}

BOOL StopCommunicationServers ()
{
  if ( WaitForSingleObject ( ghPipeThread, 2000 ) == WAIT_TIMEOUT  )
    TerminateThread ( ghPipeThread, 0 );
  if ( WaitForSingleObject ( ghWinSockTCPIPThread, 2000 ) == WAIT_TIMEOUT  )
    TerminateThread ( ghWinSockTCPIPThread, 0 );
  if ( WaitForSingleObject ( ghWinSockUDPThread, 2000 ) == WAIT_TIMEOUT  )
    TerminateThread ( ghWinSockUDPThread, 0 );
  CloseHandle ( ghSwitchEditEvent );
  CloseHandle ( ghPipe );
  CloseHandle ( ghPipeThread );
  CloseHandle ( ghWinSockTCPIPThread );
  WSACloseEvent ( ghWinSockTCPIPEvent );
  CloseHandle ( ghWinSockUDPThread );
  WSACloseEvent ( ghWinSockUDPEvent );
  WSACleanup ();
  return TRUE;
}

UINT CALLBACK BrowseHookProc( HWND hwnd , UINT uMsg, WPARAM, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
      return TRUE;

    case WM_NOTIFY:
    {
      OFNOTIFY *DialogNotify = (OFNOTIFY*)lParam ;
      if( DialogNotify->hdr.code == CDN_FILEOK )
      {
        /*char Buffer[MAX_PATH];
        char *pParam;
        GetDlgItemText( hwnd, IDC_PARAM, Buffer, sizeof(Buffer) );
        pParam = lstrcpy ( new char[lstrlen( Buffer )+1], Buffer );
        DialogNotify->lpOFN->lCustData = (LPARAM)pParam;*/

        char Buffer[MAX_PATH];
        char *pParam = new char[MAX_PATH*2];
		ZeroMemory(Buffer, sizeof(pParam));
        GetDlgItemText( hwnd, IDC_PARAM, Buffer, sizeof(Buffer) );
        lstrcpy(pParam, Buffer);
		MessageBox(NULL, pParam, "1", 0);
		ZeroMemory(Buffer, sizeof(Buffer));
		GetDlgItemText( hwnd, IDC_WORKDIR, Buffer, sizeof(Buffer) );
		//lstrcat(pParam, Buffer);
		*(int*)DialogNotify->lpOFN->lCustData = (LPARAM)pParam;
		MessageBox(NULL, "Test", "2", 0);
      }
      return TRUE;
    }
  }
  return FALSE;
}

VOID ReadRegistry ()
{
  HKEY hKey;
  LPSTR pRegKey = "Software\\Debugger";
  DWORD dwBufSize;
  DWORD dwValType;
  char szBuf[MAX_PATH];
  INT nVal;

  if ( RegOpenKey ( HKEY_LOCAL_MACHINE, pRegKey, &hKey ) != ERROR_SUCCESS )
    return;

  dwBufSize = sizeof(szBuf);
  dwValType = REG_SZ;
  if ( RegQueryValueEx ( hKey, "Server", NULL, &dwValType, (LPBYTE)szBuf, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  lstrcpy ( gszServerName, szBuf );

  dwBufSize = 4;
  dwValType = REG_DWORD;
  if ( RegQueryValueEx ( hKey, "CommunicationMethod", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gnClientPreferedMethod = nVal;

  if ( RegQueryValueEx ( hKey, "ServerTCPIPInPort", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbServerTCPIPInputPort = nVal;

  if ( RegQueryValueEx ( hKey, "ServerUDPInPort", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbServerUDPInputPort = nVal;

  if ( RegQueryValueEx ( hKey, "AutoLineFeed", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbAutoLineFeed = nVal;

  if ( RegQueryValueEx ( hKey, "AlwaysOnTop", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbAlwaysOnTop = nVal;

  if ( RegQueryValueEx ( hKey, "MinimizeToShellTray", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbMinimizeToShellTray = nVal;

  if ( RegQueryValueEx ( hKey, "ReadOnlyWindows", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbReadOnlyWindows = nVal;

  if ( RegQueryValueEx ( hKey, "ClientOutputPort", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gnClientOutPort = nVal;

  if ( RegQueryValueEx ( hKey, "ServerInportFromClient", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gnClientServerInPort = nVal;

  if ( RegQueryValueEx ( hKey, "SupportPause", NULL, &dwValType, (LPBYTE)&nVal, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  gbSupportPause = nVal;

  dwBufSize = sizeof(szBuf);
  dwValType = REG_SZ;
  if ( RegQueryValueEx ( hKey, "ServerNameForClient", NULL, &dwValType, (LPBYTE)szBuf, &dwBufSize ) != ERROR_SUCCESS )
    goto RegReadFail;
  lstrcpy ( gszClientServerName, szBuf );

RegReadFail:
  RegCloseKey ( hKey );
}

VOID WriteRegistry ( HWND hwndDlg )
{
  HKEY hKey;
  LPSTR pRegKey = "Software\\Debugger";
  DWORD dwBufSize;
  DWORD dwValType;
  char szBuf[MAX_PATH];
  INT nVal;

  if ( RegCreateKey ( HKEY_LOCAL_MACHINE, pRegKey, &hKey ) != ERROR_SUCCESS )
    return;

  dwBufSize = sizeof(szBuf);
  dwValType = REG_SZ;
  GetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERPIPENAME ), szBuf, dwBufSize );
  if ( RegSetValueEx ( hKey, "Server", NULL, dwValType, (LPBYTE)szBuf, lstrlen ( szBuf ) ) != ERROR_SUCCESS )
    goto RegWriteFail;
  lstrcpy ( gszServerName, szBuf );

  dwBufSize = 4;
  dwValType = REG_DWORD;
  nVal = SendMessage ( GetDlgItem ( hwndDlg, IDC_COMMUNICATIONMETHOD ), CB_GETCURSEL, 0, 0 );
  if ( RegSetValueEx ( hKey, "CommunicationMethod", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gnClientPreferedMethod = nVal;

  GetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERTCPIPPORT ), szBuf, sizeof(szBuf) );
  nVal = atoi ( szBuf );
  if ( RegSetValueEx ( hKey, "ServerTCPIPInPort", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbServerTCPIPInputPort = nVal;

  GetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERUDPPORT ), szBuf, sizeof(szBuf) );
  nVal = atoi ( szBuf );
  if ( RegSetValueEx ( hKey, "ServerUDPInPort", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbServerUDPInputPort = nVal;

  nVal = IsDlgButtonChecked ( hwndDlg, IDC_AUTOLINEFEED );
  if ( RegSetValueEx ( hKey, "AutoLineFeed", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbAutoLineFeed = nVal;

  nVal = IsDlgButtonChecked ( hwndDlg, IDC_ALWAYSONTOP );
  if ( RegSetValueEx ( hKey, "AlwaysOnTop", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbAlwaysOnTop = nVal;

  nVal = IsDlgButtonChecked ( hwndDlg, IDC_MINIMIZETOSHELLTRAY );
  if ( RegSetValueEx ( hKey, "MinimizeToShellTray", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbMinimizeToShellTray = nVal;

  nVal = IsDlgButtonChecked ( hwndDlg, IDC_READONLYWINDOW );
  if ( RegSetValueEx ( hKey, "ReadOnlyWindows", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbReadOnlyWindows = nVal;

  GetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTOUTPORT ), szBuf, sizeof(szBuf ) );
  nVal = atoi ( szBuf );
  if ( RegSetValueEx ( hKey, "ClientOutputPort", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gnClientOutPort = nVal;

  GetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERINPORT ), szBuf, sizeof(szBuf ) );
  nVal = atoi ( szBuf );
  if ( RegSetValueEx ( hKey, "ServerInportFromClient", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gnClientServerInPort = nVal;

  nVal = IsDlgButtonChecked ( hwndDlg, IDC_PAUSERESUME );
  if ( RegSetValueEx ( hKey, "SupportPause", NULL, dwValType, (LPBYTE)&nVal, dwBufSize ) != ERROR_SUCCESS )
    goto RegWriteFail;
  gbSupportPause = nVal;

  dwBufSize = sizeof(szBuf);
  dwValType = REG_SZ;
  GetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERNAME ), szBuf, dwBufSize );
  if ( RegSetValueEx ( hKey, "ServerNameForClient", NULL, dwValType, (LPBYTE)szBuf, lstrlen(szBuf) ) != ERROR_SUCCESS )
    goto RegWriteFail;
  lstrcpy ( gszClientServerName, szBuf );

RegWriteFail:
  RegCloseKey ( hKey );
}

//Helper Functions

BOOL CheckAndDisplay ( LPSTR pStr, DWORD, HWND hwnd )
{
  HWND hEdit;

  hEdit = hwnd ? hwnd : CheckAndActivate ();
  if ( !hEdit || !IsWindow ( hEdit ) )
  {
    SendMessage ( ghFrameWnd, WM_COMMAND,  MAKELPARAM(IDM_NEW, 0), 0 );
    hEdit = CheckAndActivate ();
    if ( !hEdit || !IsWindow ( hEdit ) )
      return 0;
  }
  INT nTextLen = GetWindowTextLength ( hEdit );
  if ( nTextLen < 0 || nTextLen > 0x70000000 )
  {
    INT nLineCount = (INT)SendMessage ( hEdit, EM_GETLINECOUNT, 0, 0 );
    if ( nLineCount > 100 )
      SendMessage ( hEdit, EM_SETSEL, 0, (LPARAM)SendMessage ( hEdit, EM_LINEINDEX, 100, 0 ) );
    else
      SendMessage ( hEdit, EM_SETSEL, 0, 100*1024 );
    SendMessage ( hEdit, WM_CLEAR, 0, 0 );
    nTextLen = GetWindowTextLength ( hEdit );
  }
  SendMessage ( hEdit, EM_SETSEL, nTextLen, nTextLen );
  SendMessage ( hEdit, EM_REPLACESEL, FALSE, (LPARAM)pStr );
  SendMessage ( hEdit, EM_SCROLLCARET, 0, 0 );
  return TRUE;
}

BOOL GetFilenameFromHandle ( HANDLE hFile, HANDLE hProcess, LPSTR szBuf )
{
  TCHAR szFilename[MAX_PATH];
  BOOL bSuccess = FALSE;
  ZeroMemory ( szFilename, MAX_PATH );
  int uMaxLenDest = 0;

  // Get the file size.
  DWORD dwFileSizeHi = 0;
  DWORD dwFileSizeLo = GetFileSize ( hFile, &dwFileSizeHi ); 

  // Create a file mapping object.
  HANDLE hFileMap = CreateFileMapping ( hFile, NULL, PAGE_READONLY, 0, dwFileSizeLo, NULL );

  if (hFileMap)
  {
    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile ( hFileMap, FILE_MAP_READ, 0, 0, 1 );

    if ( pMem )
    {
      if ( pGetMappedFileName ( hProcess, pMem, (LPTSTR)szFilename, MAX_PATH ) )
      {
        // Attempt to translate the path with device name into drive letters.
        bSuccess = TRUE;
        TCHAR szTemp[512];
        *szTemp = NULL;

        if ( GetLogicalDriveStrings ( lstrlen ( szTemp ), szTemp ) )
        {
          TCHAR szName[MAX_PATH];
          TCHAR szDrive[3] = TEXT(" :");
          BOOL bFound = FALSE;
          TCHAR* p = szTemp;

          do
          {
            // Copy the drive letter into the template string, removing the backslash.
            *szDrive = *p;

            // Look up each device name.
            if ( QueryDosDevice ( szDrive, szName, lstrlen ( szName ) ) )
            {
              int uNameLen = lstrlen ( szName );

              // If greater than file name length, it's not a match.
              if ( uNameLen < uMaxLenDest )
              {
                //bFound = strncmpi ( szFilename, szName, uNameLen ) == 0; //kannan
				  bFound = true; //kannan

                if ( bFound )
                {
                  // Reconstruct zFilename using szTemp as scratch,
                  // and replace device path with our DOS path.
                  TCHAR szTempFile[MAX_PATH];
                  wsprintf ( szTempFile, "%s%s", szDrive, &szFilename + uNameLen );
                  lstrcpyn ( szFilename, szTempFile, uMaxLenDest );
                }
              }
            }

            // Go to the next NULL character.
            while ( *p++ );
          } while ( !bFound && *p ); // at the end of the string
        }
      }
      UnmapViewOfFile ( pMem );
    } 
    CloseHandle ( hFileMap );
  }
  if ( szBuf )
    lstrcpy ( szBuf, szFilename );
  return bSuccess;
}

INT CenterWindow ( HWND hWnd )
{
  RECT rc;

  GetWindowRect ( hWnd, &rc );
  SetWindowPos ( hWnd, NULL,
                 ( ( GetSystemMetrics ( SM_CXSCREEN ) - ( rc.right - rc.left ) ) / 2 ),
                 ( ( GetSystemMetrics ( SM_CYSCREEN ) - ( rc.bottom - rc.top ) ) / 2 ),
                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE );
  return 1;
}

unsigned DateToJulianDay ( unsigned m, unsigned d, unsigned y )
{
  unsigned c, ya;
  if( y <= 99 )
    y += 1900;

  if( m > 2 )
    m -= 3;
  else
  {
    m += 9;
    y--;
  }

  c = y / 100;
  ya = y - 100*c;
  return ((146097L*c)>>2) + ((1461*ya)>>2) + (153*m + 2)/5 + d + 1721119L;
}

VOID ConvertFileTime2String ( FILETIME* pft, LPSTR szBuf, BOOL bDuration )
{
  SYSTEMTIME st;
  ZeroMemory ( &st, sizeof(SYSTEMTIME) );
  FileTimeToSystemTime ( pft, &st );
  if ( bDuration )
    wsprintf ( szBuf, "%d  %02hd:%02hd:%02hd", DateToJulianDay ( st.wMonth, st.wDay, st.wYear ) - 2305814, st.wHour, st.wMinute, st.wSecond );
  else
  {
    TIME_ZONE_INFORMATION tzi;
    SYSTEMTIME stl;
    ZeroMemory ( &stl, sizeof(SYSTEMTIME) );
    ZeroMemory ( &tzi, sizeof(TIME_ZONE_INFORMATION) );
    GetTimeZoneInformation ( &tzi );
    if ( SystemTimeToTzSpecificLocalTime ( &tzi, &st, &stl ) )
      wsprintf ( szBuf, "%02hd/%02hd/%04hd %02hd:%02hd:%02hd", stl.wMonth, stl.wDay, stl.wYear, stl.wHour, stl.wMinute, stl.wSecond );
    else
      wsprintf ( szBuf, "%02hd/%02hd/%04hd %02hd:%02hd:%02hd GMT", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond );
  }
}

VOID DoWin2000SocketFix ( SOCKET sock )
{
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

  DWORD dwBytesReturned = 0;
  BOOL bNewBehavior = FALSE;
  WSAIoctl ( sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL );
}

VOID GetPriorityClassFromPriorityString ( LPSTR pStr, DWORD& dwPriority )
{
  if ( !lstrcmp ( pStr, "High" ) )
    dwPriority = HIGH_PRIORITY_CLASS;
  else if ( !lstrcmp ( pStr, "Idle" ) )
    dwPriority = IDLE_PRIORITY_CLASS;
  else if ( !lstrcmp ( pStr, "Norm" ) )
    dwPriority = NORMAL_PRIORITY_CLASS;
  else if ( !lstrcmp ( pStr, "Real" ) )
    dwPriority = REALTIME_PRIORITY_CLASS;
  else
    dwPriority = atoi ( pStr );
}

VOID GetPriorityStringFromPriorityClass ( DWORD dwPriority, LPSTR pStr )
{
  *pStr = NULL;
  switch ( dwPriority )
  {
    case HIGH_PRIORITY_CLASS:
      lstrcpy ( pStr, "High" );
      break;

    case IDLE_PRIORITY_CLASS:
      lstrcpy ( pStr, "Idle" );
      break;

    case NORMAL_PRIORITY_CLASS:
      lstrcpy ( pStr, "Normal" );
      break;

    case REALTIME_PRIORITY_CLASS:
      lstrcpy ( pStr, "Real" );
      break;

    default:
      wsprintf ( pStr, "%u", dwPriority );
      break;
  }
}

VOID GetStringThreadBasePriorityFromDWORD ( DWORD dwBasePriority, LPSTR szBuf )
{
  *szBuf = NULL;
  switch ( dwBasePriority )
  {
    case 0x00:
      lstrcpy ( szBuf, "THREAD_PRIORITY_IDLE" );
      break;

    case 0x01:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST - 5" );
      break;

    case 0x02:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST - 4" );
      break;

    case 0x03:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST - 3" );
      break;

    case 0x04:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST - 2" );
      break;

    case 0x05:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST - 1" );
      break;

    case 0x06:
      lstrcpy ( szBuf, "THREAD_PRIORITY_LOWEST" );
      break;

    case 0x07:
      lstrcpy ( szBuf, "THREAD_PRIORITY_BELOW_NORMAL" );
      break;

    case 0x08:
      lstrcpy ( szBuf, "THREAD_PRIORITY_NORMAL" );
      break;

    case 0x09:
      lstrcpy ( szBuf, "THREAD_PRIORITY_ABOVE_NORMAL" );
      break;

    case 0x0A:
      lstrcpy ( szBuf, "THREAD_PRIORITY_HIGHEST" );
      break;

    case 0x0B:
      lstrcpy ( szBuf, "THREAD_PRIORITY_HIGHEST + 1" );
      break;

    case 0x0C:
      lstrcpy ( szBuf, "THREAD_PRIORITY_HIGHEST + 2" );
      break;

    case 0x0D:
      lstrcpy ( szBuf, "THREAD_PRIORITY_HIGHEST + 3" );
      break;

    case 0x0E:
      lstrcpy ( szBuf, "THREAD_PRIORITY_HIGHEST + 4" );
      break;

    case 0x0F:
      lstrcpy ( szBuf, "THREAD_PRIORITY_TIME_CRITICAL" );
      break;
  }
  wsprintf ( szBuf + lstrlen ( szBuf ), *szBuf ? " (%08X)" : "(%08X)", dwBasePriority );
}

VOID PrintError ()
{
  LPVOID lpMsgBuf;

  FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf, 0, NULL );

  MessageBox( ghFrameWnd, (LPSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

  LocalFree( lpMsgBuf );
}

VOID WSAPrintError ( LPSTR pTitle )
{
  LPVOID lpMsgBuf;

  FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf, 0, NULL );

  MessageBox( ghFrameWnd, (LPSTR)lpMsgBuf, pTitle ? pTitle : "WSAGetLastError", MB_OK|MB_ICONINFORMATION );

  LocalFree( lpMsgBuf );
}

//UI Callbacks

BOOL CALLBACK CloseAllProc ( HWND hwnd, LPARAM )
{
  SendMessage ( GetParent ( hwnd ), WM_MDIDESTROY, (WPARAM)hwnd, 0 );
  return TRUE;
}

BOOL CALLBACK ConfigDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      CenterWindow ( hwndDlg );
      ReadRegistry ();
      SetControlValues ( hwndDlg );
      return TRUE;
    }

    case WM_COMMAND:
    {
      switch ( LOWORD(wParam) )
      {
        case IDOK:
          WriteRegistry ( hwndDlg );
          EndDialog ( hwndDlg, 1 );
          break;

        case IDCANCEL:
          EndDialog ( hwndDlg, 1 );
          break;

        case IDC_DEFAULTS:
          SetDefaultControlValues ( hwndDlg );
          break;
      }
      break;
    }
  }
  return FALSE;
}

HWND CheckAndActivate ( HWND hEdit )
{
  if ( WaitForSingleObject ( ghSwitchEditEvent, 1000 ) == WAIT_TIMEOUT  )
    return NULL;
  ResetEvent ( ghSwitchEditEvent );
  HWND hwnd;
  if ( hEdit )
    hwnd = ghActiveEdit = hEdit;
  else
    hwnd = ghActiveEdit;
  SetEvent ( ghSwitchEditEvent );
  return hwnd;
}

LRESULT CALLBACK ChildWindowProc ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  HWND hEdit;
  RECT rt;

  switch ( uMsg )
  {
    case WM_CREATE:
    {
      char szBuf[MAX_PATH];
      GetWindowText ( hwnd, szBuf, sizeof(szBuf) );
      SendMessage ( ghClientWnd, WM_MDISETMENU, (WPARAM)ghChildMenu, (LPARAM)GetSubMenu ( ghChildMenu, 1 ) );
      DrawMenuBar ( ghFrameWnd );
      GetClientRect ( hwnd, &rt );
      hEdit = CreateWindow ( "EDIT", NULL,
                             WS_VISIBLE | WS_CHILD | ES_WANTRETURN | ES_MULTILINE |
                             ES_AUTOVSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | WS_HSCROLL,
                             rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top,
                             hwnd, NULL, ghInstance, NULL );
      SendMessage ( hEdit, EM_LIMITTEXT, 0, 0 );
      SetFocus ( hEdit );
      if ( *szBuf == 'O' )
        CheckAndActivate ( hEdit );
      LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
      if ( lpcs && lpcs->lpCreateParams && ((MDICREATESTRUCT*)lpcs->lpCreateParams)->lParam )
      {
        LPSTR pFile = (LPSTR)((MDICREATESTRUCT*)lpcs->lpCreateParams)->lParam;
        LPSTR pParam = (LPSTR)*(int*)(pFile+lstrlen(pFile)+1);
        if ( int(pParam) == 0x01010101 )
        {
          HANDLE hf = CreateFile ( pFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
          delete [] pFile;
          if ( hf == INVALID_HANDLE_VALUE )
            return 0;
          char szBuf[2*1024];
          DWORD dwBytesRead = 0;
          while ( ReadFile ( hf, szBuf, sizeof(szBuf)-1, &dwBytesRead, NULL ) )
          {
            if ( !dwBytesRead )
              break;
            *(szBuf+dwBytesRead) = NULL;
            CheckAndDisplay ( szBuf, dwBytesRead, hEdit );
            dwBytesRead = 0;
          }
          CloseHandle ( hf );
        }
        else
        {
          if ( pParam )
            *(int*)(pParam+lstrlen(pParam)+1) = (int)hwnd;
          else
          {
            *(int*)(pFile+lstrlen(pFile)+1) = 0;
            *(int*)(pFile+lstrlen(pFile)+5) = (int)hwnd;
          }
          StartDebugger ( hwnd, pFile );
        }
      }
      return 0;
    }

    case WM_SETFOCUS:
      SetFocus ( GetWindow ( hwnd, GW_CHILD ) );
      break;

    case WM_COMMAND:
    {
      switch ( LOWORD(wParam) )
      {
        case IDM_SAVE:
        {
          OPENFILENAME ofn;
          CHAR szFile[MAX_PATH];

          hEdit = GetWindow ( hwnd, GW_CHILD );

          ZeroMemory ( &ofn, sizeof(ofn) );
          ofn.lStructSize = sizeof(ofn);
          ofn.hwndOwner = hwnd;
          ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
          ofn.lpstrFilter = "Output Text Files\0*.txt\0All Files\0*.*\0\0";
          ofn.lpstrTitle = "Select Output Text File";
          ofn.lpstrDefExt = "txt";
          ofn.lpstrFile = szFile;
          ofn.nFilterIndex = 1;
          ofn.nMaxFile = MAX_PATH;
          *szFile = NULL;
          if ( !GetSaveFileName ( &ofn ) )
            break;
          if ( !access ( szFile, 0 ) &&
               MessageBox ( hwnd, "Do you want to overwrite ?", szFile, MB_YESNO ) == IDNO )
            break;
          HANDLE hf = CreateFile ( szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL );
          if ( hf == INVALID_HANDLE_VALUE )
            break;
          LPSTR ptr = new char[2*1024+10];
          INT nLineCount = (INT)SendMessage ( hEdit, EM_GETLINECOUNT, 0, 0 );
          DWORD dwBytesRead, dwBytesWrote;
          for ( int i = 0; i < nLineCount; i++ )
          {
            dwBytesRead = (DWORD)SendMessage ( hEdit, EM_GETLINE, (WPARAM)i, (LPARAM)ptr );
            lstrcpy ( ptr + dwBytesRead, "\r\n" );
            dwBytesWrote = 0;
            WriteFile ( hf, ptr, dwBytesRead+2, &dwBytesWrote, NULL );
          }
          delete [] ptr;
          CloseHandle ( hf );
          break;
        }
      }
      break;
    }

    case WM_SIZE:
      GetClientRect ( hwnd, &rt );
      hEdit = GetWindow ( hwnd, GW_CHILD );
      MoveWindow ( hEdit, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, TRUE );
      break;

    case WM_MDIACTIVATE:
    {
      char szBuf[MAX_PATH];
      GetWindowText ( hwnd, szBuf, sizeof(szBuf) );
      if ( *szBuf != 'O' )
        return 0;

      if ( lParam == (LPARAM)hwnd )
        SendMessage ( ghClientWnd, WM_MDISETMENU, (WPARAM)ghChildMenu, (LPARAM)GetSubMenu ( ghChildMenu, 1 ) );
      else
        SendMessage ( ghClientWnd, WM_MDISETMENU, (WPARAM)ghMainMenu, (LPARAM)0 ); //GetSubMenu ( ghMainMenu, 1 ) );

      DrawMenuBar ( ghFrameWnd );
      CheckAndActivate ( GetWindow ( hwnd, GW_CHILD ) );

      return 0;
    }
  }

  return ( DefMDIChildProc ( hwnd, uMsg, wParam, lParam ) );
}

LRESULT CALLBACK FrameWindowProc ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  MDICREATESTRUCT mdic;
  HWND hChild;
  LPSTR pFile = NULL;
  BOOL bDebug = FALSE;

  switch ( uMsg )
  {
    case WM_CREATE:
    {
      CLIENTCREATESTRUCT ccs;

      ZeroMemory ( &ccs, sizeof(ccs) );
      ZeroMemory ( &mdic, sizeof(mdic) );
      ccs.hWindowMenu = NULL; //GetSubMenu ( ghMainMenu, 1 );
      ccs.idFirstChild = IDM_FIRSTCHILD;
      ghClientWnd = CreateWindow ( "MDICLIENT", NULL,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                                   -1, -1, -1, -1, hwnd, (HMENU)1, ghInstance, (LPSTR)&ccs );
      ghfont = CreateFont(10,10,0,0,FW_THIN,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS,PROOF_QUALITY,
                          DEFAULT_PITCH|FF_DONTCARE,"MS Sans Serif");
      return 0;
    }

    case WM_COMMAND:
    {
      switch ( LOWORD(wParam) )
      {
        case 1:
        {
          DisconnectNamedPipe ( ghPipe );
          ConnectNamedPipe ( ghPipe, NULL );
          break;
        }

        case IDM_PROCESS:
        {
          if ( ghProcessInfoWindow && IsWindow ( ghProcessInfoWindow ) )
            SetForegroundWindow ( ghProcessInfoWindow );
          else
            ListProcesses();
          break;
        }

        case IDM_DEBUG:
        {
          bDebug = TRUE;
          //Will proceed with IDM_OPEN & IDM_NEW to create MDI Child Window
        }

        case IDM_OPEN:
        {
          OPENFILENAME ofn;

          ZeroMemory ( &ofn, sizeof(ofn) );
          ofn.lStructSize = sizeof(ofn);
          ofn.hwndOwner = hwnd;
          ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
          pFile = new char[MAX_PATH+20];
          *pFile = NULL;
          if ( bDebug )
          {
            ofn.hInstance = ghInstance;
            ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
            ofn.lpfnHook = BrowseHookProc;
            ofn.lpTemplateName =  "PARAMINPUT_DIALOG";
            ofn.lpstrFilter = "Executable Files\0*.exe\0All Files\0*.*\0\0";
            ofn.lpstrTitle = "Select Application to Debug";
            ofn.lpstrDefExt = "exe";
          }
          else
          {
            ofn.lpstrFilter = "Output Text Files\0*.txt\0All Files\0*.*\0\0";
            ofn.lpstrTitle = "Select Output Text File";
            ofn.lpstrDefExt = "txt";
          }
          ofn.lpstrFile = pFile;
          ofn.nFilterIndex = 1;
          ofn.nMaxFile = MAX_PATH+20;
          if ( !GetOpenFileName ( &ofn ) )
          {
            delete [] pFile;
            return 0;
          }
          *(int*)(pFile+lstrlen(pFile)+1) = 0x01010101;
          if ( bDebug )
          {
            LPSTR pParam = (char*)ofn.lCustData;

            *(int*)(pFile+lstrlen(pFile)+1) = 0;
            if ( lstrlen ( pParam ) < 1 )
              delete [] pParam;
            else
              *(int*)(pFile+lstrlen(pFile)+1) = (int)pParam;
          }
          //Will proceed with IDM_NEW to create MDI Child Window
        }

        case IDM_NEW:
        {
          char szChildName[MAX_PATH+50];

          if ( bDebug )
            wsprintf ( szChildName, "Debugger for [%s]", pFile );
          else
            wsprintf ( szChildName, "Output Window [%d]", ++gnDocCount );

          mdic.szClass = gChildClass;
          mdic.szTitle = szChildName;
          mdic.hOwner = ghInstance;
          mdic.x = CW_USEDEFAULT;
          mdic.y = CW_USEDEFAULT;
          mdic.cx = CW_USEDEFAULT;
          mdic.cy = CW_USEDEFAULT;
          mdic.style = 0;
          mdic.lParam = pFile ? (LPARAM)pFile : 0;
          SendMessage ( ghClientWnd, WM_MDICREATE, 0, (LPARAM)&mdic );
          return 0;
        }

        case IDM_CLOSE:
          hChild = (HWND)SendMessage ( ghClientWnd, WM_MDIGETACTIVE, 0, 0 );
          if ( hChild )
            SendMessage ( ghClientWnd, WM_MDIDESTROY, (WPARAM)hChild, 0 );
          return 0;

        case IDM_EXIT:
          SendMessage ( hwnd, WM_CLOSE, 0, 0 );
          return 0;

        case IDM_CLOSEALL:
          EnumChildWindows ( ghClientWnd, CloseAllProc, 0 );
          return 0;

        case IDM_TILE:
          SendMessage ( ghClientWnd, WM_MDITILE, 0, 0 );
          return 0;

        case IDM_CASCADE:
          SendMessage ( ghClientWnd, WM_MDICASCADE, 0, 0 );
          return 0;

        case IDM_ARRANGE:
          SendMessage ( ghClientWnd, WM_MDIICONARRANGE, 0, 0 );
          return 0;

        case IDM_CONFIG:
          DialogBox ( ghInstance, "CONFIG_DIALOG", NULL, ConfigDialogProc );
          break;

        default:
          hChild = (HWND)SendMessage ( ghClientWnd, WM_MDIGETACTIVE, 0, 0 );
          if ( IsWindow ( hChild ) )
            SendMessage ( hChild, WM_COMMAND, wParam, lParam );
          break;
      }
      break;
    }

    case WM_QUERYENDSESSION:
    case WM_CLOSE:
      SendMessage ( hwnd, WM_COMMAND, IDM_CLOSEALL, 0 );
      if ( GetWindow ( ghClientWnd, GW_CHILD ) )
        return 0;
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefFrameProc ( hwnd, ghClientWnd, uMsg, wParam, lParam );
}

//Process Status Related Functions

BOOL CALLBACK EnumDesktopWindowsDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghEDWWindow = hwndDlg;
      HWND hCombo = GetDlgItem ( hwndDlg, IDC_1COMBOPRIORITY );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"High" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Idle" ); 
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Normal" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Real" );
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_1LISTAPP );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      gnEDWAppListColumn = -1;
      gnEDWAppListAscending = FALSE;
      HIMAGELIST hilEDW = ImageList_Create ( 14, 14, ILC_COLOR4, 1, 100 );
      ImageList_SetBkColor ( hilEDW, COLORREF ( 0x00FFFFFF ) );
      ImageList_AddIcon( hilEDW, LoadIcon ( ghInstance, "IDC_NULLICON" ) );
      ListView_SetImageList( hListWnd, hilEDW, LVSIL_SMALL );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_1LISTAPP );
      LoadAndFillEDWAppList ( hListWnd, FALSE );      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_1LISTAPP );
      DeleteListViewItems ( hListWnd, FALSE );
      ImageList_Destroy ( ListView_GetImageList ( hListWnd, LVSIL_SMALL ) );
      ghEDWWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_1LISTAPP )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_1LISTAPP );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnEDWAppListColumn )
            gnEDWAppListAscending = !gnEDWAppListAscending;
          else
          {
            gnEDWAppListAscending = TRUE;
            gnEDWAppListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)EDWAppListCompareFunc );
          break;
        }

        case LVN_ITEMCHANGED:
        {
          INT nSel = pnml->iItem;
          if ( nSel == -1 )
            break;
          LV_ITEM lvi;
          ZeroMemory ( &lvi, sizeof(lvi) );
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.iItem = nSel;
          SendMessage ( hListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
          if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
            break;

          PAPPLIST pal = (PAPPLIST)lvi.lParam;
          if ( pal )
          {
            char szBuf[512];
            wsprintf ( szBuf, "PID=%u -> %s", pal->dwPID, pal->szAppName );
            SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
            GetPriorityStringFromPriorityClass ( pal->dwPriority, szBuf );
            SendMessage ( GetDlgItem ( hwndDlg, IDC_1COMBOPRIORITY ), CB_SELECTSTRING, 0, (LPARAM)szBuf );
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }

        case IDC_1BUTSETPRIORITY:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_SET_INFORMATION, FALSE, dwPID );
          if ( !hProcess )
          {
            MessageBox ( hwndDlg, "Unable to change the Priority", "Process Info", 0 );
            break;
          }
          SendMessage ( GetDlgItem ( hwndDlg, IDC_1COMBOPRIORITY ), CB_GETLBTEXT,
                        (WPARAM)SendMessage ( GetDlgItem ( hwndDlg, IDC_1COMBOPRIORITY ), CB_GETCURSEL, 0, 0 ),
                        (LPARAM)szBuf );
          DWORD dwPriority;
          GetPriorityClassFromPriorityString ( szBuf, dwPriority );
          SetPriorityClass ( hProcess, dwPriority );
          CloseHandle ( hProcess );
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }

        case IDC_1BUTTERMINATE:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, dwPID );
          if ( hProcess )
          {
            if ( !TerminateProcess ( hProcess, 0 ) )
            {
              CloseHandle ( hProcess );
              hProcess = NULL;
            }
            else
              CloseHandle ( hProcess );
          }
          if ( !hProcess )
          {
            MessageBox ( ghProcessInfoWindow, "Unable to Terminate the process\nRefresh this window & try again.",
                         "Process Info", 0 );
            break;
          }
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }
      }
      break;
    }

  }
  return FALSE;
}

BOOL CALLBACK ListAllWindowsDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghAllWindowWindow = hwndDlg;
      HWND hCombo = GetDlgItem ( hwndDlg, IDC_1COMBOPRIORITY );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"High" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Idle" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Norm" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Real" );
      HWND hWndTree = GetDlgItem ( hwndDlg, IDC_3HWNDTREE );
      HIMAGELIST hil = ImageList_Create ( 16, 16, ILC_COLOR4, 1, 100 );
      ImageList_SetBkColor ( hil, COLORREF ( 0x00FFFFFF ) );
      ImageList_AddIcon( hil, LoadIcon ( ghInstance, "IDC_WINDOW" ) );
      TreeView_SetImageList ( hWndTree, hil, TVSIL_NORMAL );
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hWndTree = GetDlgItem ( hwndDlg, IDC_3HWNDTREE );
      LoadChildWindowsInTree ( hWndTree, GetDesktopWindow(), TVI_ROOT );
      HTREEITEM DesktopItem = TreeView_GetChild ( hWndTree, TVI_ROOT );
      TreeView_Expand ( hWndTree, DesktopItem, TVE_EXPAND );
      break;
    }

    case WM_DESTROY:
    {
      HWND hWndTree = GetDlgItem ( hwndDlg, IDC_3HWNDTREE );
      ImageList_Destroy ( TreeView_GetImageList ( hWndTree, TVSIL_NORMAL ) );
      ghAllWindowWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_3HWNDTREE )
        break;
      HWND hTreeWnd = GetDlgItem ( hwndDlg, IDC_3HWNDTREE );
      NM_TREEVIEW* pnmt = (NM_TREEVIEW*)lParam;
      switch ( pnmh->code )
      {
        case TVN_SELCHANGED:
        {
          char szInBuf[2*MAX_PATH];
          char szOutBuf[2*MAX_PATH];
          LPSTR ptr, ptr1, ptr2;
          pnmt->itemNew.mask = TVIF_TEXT;
          pnmt->itemNew.pszText = szInBuf;
          pnmt->itemNew.cchTextMax = sizeof ( szInBuf );
          TreeView_GetItem ( hTreeWnd, &pnmt->itemNew );
          ptr = strrchr ( szInBuf, '=' );
          ptr1 = strchr ( szInBuf, '\"' );
          ptr++;
          ptr1++;
          ptr2 = strchr ( ptr1, '\"' );
          *ptr2 = NULL;
          DWORD dwPID= atoi ( ptr );
          wsprintf ( szOutBuf, "PID=%u -> %s", dwPID, ptr1 );
          SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szOutBuf );
          HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, dwPID );
          if ( hProcess )
          {
            DWORD dwPriority = GetPriorityClass ( hProcess );
            CloseHandle ( hProcess );
            GetPriorityStringFromPriorityClass ( dwPriority, szOutBuf );
            SendMessage ( GetDlgItem ( hwndDlg, IDC_3COMBOPRIORITY ), CB_SELECTSTRING, 0, (LPARAM)szOutBuf );
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_3BUTSETPRIORITY:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_SET_INFORMATION, FALSE, dwPID );
          if ( !hProcess )
          {
            MessageBox ( hwndDlg, "Unable to change the Priority", "Process Info", 0 );
            break;
          }
          SendMessage ( GetDlgItem ( hwndDlg, IDC_3COMBOPRIORITY ), CB_GETLBTEXT,
                        (WPARAM)SendMessage ( GetDlgItem ( hwndDlg, IDC_3COMBOPRIORITY ), CB_GETCURSEL, 0, 0 ),
                        (LPARAM)szBuf );
          DWORD dwPriority;
          GetPriorityClassFromPriorityString ( szBuf, dwPriority );
          SetPriorityClass ( hProcess, dwPriority );
          CloseHandle ( hProcess );
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }

        case IDC_BUTREFRESH:
        {
          HWND hWndTree = GetDlgItem ( hwndDlg, IDC_3HWNDTREE );
          TreeView_DeleteAllItems ( hWndTree );
          PostMessage ( hwndDlg, WM_LOADDATA, wParam, lParam );
          break;
        }

        case IDC_3BUTTERMINATE:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, dwPID );
          if ( hProcess )
          {
            if ( !TerminateProcess ( hProcess, 0 ) )
            {
              CloseHandle ( hProcess );
              hProcess = NULL;
            }
            else
              CloseHandle ( hProcess );
          }
          if ( !hProcess )
          {
            MessageBox ( ghProcessInfoWindow, "Unable to Terminate the process\nRefresh this window & try again.",
                         "Process Info", 0 );
            break;
          }
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK APIListProcessTimesDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghProcessTimesWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_10LISTPROCESS );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      gnProcessTimeListColumn = -1;
      gnProcessTimeListAscending = FALSE;
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_10LISTPROCESS );
      LoadAndFillProcessTimeList ( hListWnd, TRUE );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_10LISTPROCESS );
      DeleteListViewItems ( hListWnd, FALSE );
      ghProcessTimesWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_10LISTPROCESS )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_10LISTPROCESS );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnProcessTimeListColumn )
            gnProcessTimeListAscending = !gnProcessTimeListAscending;
          else
          {
            gnProcessTimeListAscending = TRUE;
            gnProcessTimeListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)ProcessTimeCompareFunc );
          break;
        }

        case LVN_ITEMCHANGED:
        {
          INT nSel = pnml->iItem;
          if ( nSel == -1 )
            break;
          LV_ITEM lvi;
          ZeroMemory ( &lvi, sizeof(lvi) );
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.iItem = nSel;
          SendMessage ( hListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
          if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
            break;

          PPROCESSTIME ppt = (PPROCESSTIME)lvi.lParam;
          if ( ppt )
          {
            LPSTR pPtr = strrchr ( ppt->szProcessName, '\\' );
            if ( pPtr )
              pPtr++;
            else
              pPtr = ppt->szProcessName;
            char szBuf[512];
            wsprintf ( szBuf, "PID=%u -> %s", ppt->dwPID, pPtr );
            SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK PSAPIListDriversDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghPSDriversWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_11LISTDRIVERS );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      gnPSDriversListColumn = -1;
      gnPSDriversListAscending = FALSE;
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_11LISTDRIVERS );
      LoadAndFillPSDriverList ( hListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_11LISTDRIVERS );
      DeleteListViewItems ( hListWnd, FALSE );
      ghPSDriversWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_11LISTDRIVERS )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_11LISTDRIVERS );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnPSDriversListColumn )
            gnPSDriversListAscending = !gnPSDriversListAscending;
          else
          {
            gnPSDriversListAscending = TRUE;
            gnPSDriversListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListDriversCompareFunc );
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK PSAPIListMemoryUsageDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghPSMemoryUsageWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_9LISTPROCESS );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      gnPSMemoryUsageListColumn = -1;
      gnPSMemoryUsageListAscending = FALSE;
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_9LISTPROCESS );
      LoadAndFillPSMemoryUsageList ( hListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_9LISTPROCESS );
      DeleteListViewItems ( hListWnd, FALSE );
      ghPSMemoryUsageWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_9LISTPROCESS )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_9LISTPROCESS );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnPSMemoryUsageListColumn )
            gnPSMemoryUsageListAscending = !gnPSMemoryUsageListAscending;
          else
          {
            gnPSMemoryUsageListAscending = TRUE;
            gnPSMemoryUsageListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListMemoryUsageCompareFunc );
          break;
        }

        case LVN_ITEMCHANGED:
        {
          INT nSel = pnml->iItem;
          if ( nSel == -1 )
            break;
          LV_ITEM lvi;
          ZeroMemory ( &lvi, sizeof(lvi) );
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.iItem = nSel;
          SendMessage ( hListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
          if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
            break;

          PPSAPIPROCESSLIST ppspl = (PPSAPIPROCESSLIST)lvi.lParam;
          if ( ppspl )
          {
            LPSTR pPtr = strrchr ( ppspl->szProcessName, '\\' );
            if ( pPtr )
              pPtr++;
            else
              pPtr = ppspl->szProcessName;
            char szBuf[512];
            wsprintf ( szBuf, "PID=%u -> %s", ppspl->dwPID, pPtr );
            SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK PSAPIListModulesDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghPSModulesWindow = hwndDlg;
      gnPSModuleListAscending = FALSE;
      gnPSModuleListColumn = -1;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_8LISTMODULE );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_8LISTMODULE );
      LoadAndFillPSModuleList ( hListWnd, TRUE );
			char szBuf[30];
			wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
			SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
			wsprintf ( szBuf, "Module Size : %d", gnModuleSize );
			SetWindowText ( GetDlgItem ( hwndDlg, IDC_MODULESIZE ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_8LISTMODULE );
      DeleteListViewItems ( hListWnd, FALSE );
      ghPSModulesWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_8LISTMODULE );

      if ( pnmh->code == PSN_SETACTIVE )
      {
        PostMessage ( hwndDlg, WM_LOADDATA, wParam, lParam );
        break;
      }
      if ( pnmh->idFrom == IDC_8LISTMODULE )
      {
        switch ( pnmh->code )
        {
          case LVN_COLUMNCLICK:
          {
            if ( pnml->iSubItem == gnPSModuleListColumn )
              gnPSModuleListAscending = !gnPSModuleListAscending;
            else
            {
              gnPSModuleListAscending = TRUE;
              gnPSModuleListColumn = pnml->iSubItem;
            }
            SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListModuleCompareFunc );
            break;
          }
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTCLEARFILTER:
        {
          SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
          //Will Proceed Thru IDC_BUTREFRESH
        }

        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK PSAPIListProcessDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghPSProcessWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_7LISTPROCESS );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      HWND hCombo = GetDlgItem ( hwndDlg, IDC_7COMBOPRIORITY );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"High" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Idle" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Norm" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Real" );
      gnPSProcessListColumn = -1;
      gnPSProcessListAscending = FALSE;
      HIMAGELIST hilEDW = ImageList_Create ( 14, 14, ILC_COLOR4, 1, 100 );
      ImageList_SetBkColor ( hilEDW, COLORREF ( 0x00FFFFFF ) );
      ImageList_AddIcon( hilEDW, LoadIcon ( ghInstance, "IDC_NULLICON" ) );
      ListView_SetImageList( hListWnd, hilEDW, LVSIL_SMALL );
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_7LISTPROCESS );
      LoadAndFillPSProcessList ( hListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_7LISTPROCESS );
      DeleteListViewItems ( hListWnd, FALSE );
      ghPSProcessWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_7LISTPROCESS )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_7LISTPROCESS );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnPSProcessListColumn )
            gnPSProcessListAscending = !gnPSProcessListAscending;
          else
          {
            gnPSProcessListAscending = TRUE;
            gnPSProcessListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListProcessCompareFunc );
          break;
        }

        case LVN_ITEMCHANGED:
        {
          INT nSel = pnml->iItem;
          if ( nSel == -1 )
            break;
          LV_ITEM lvi;
          ZeroMemory ( &lvi, sizeof(lvi) );
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.iItem = nSel;
          SendMessage ( hListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
          if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
            break;

          PPSAPIPROCESSLIST ppspl = (PPSAPIPROCESSLIST)lvi.lParam;
          if ( ppspl )
          {
            LPSTR pPtr = strrchr ( ppspl->szProcessName, '\\' );
            if ( pPtr )
              pPtr++;
            else
              pPtr = ppspl->szProcessName;
            char szBuf[512];
            wsprintf ( szBuf, "PID=%u -> %s", ppspl->dwPID, pPtr );
            SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
            GetPriorityStringFromPriorityClass ( ppspl->dwPriority, szBuf );
            SendMessage ( GetDlgItem ( hwndDlg, IDC_7COMBOPRIORITY ), CB_SELECTSTRING, 0, (LPARAM)szBuf );
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_7BUTSETPRIORITY:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_SET_INFORMATION, FALSE, dwPID );
          if ( !hProcess )
          {
            MessageBox ( hwndDlg, "Unable to change the Priority", "Process Info", 0 );
            break;
          }
          SendMessage ( GetDlgItem ( hwndDlg, IDC_7COMBOPRIORITY ), CB_GETLBTEXT,
                        (WPARAM)SendMessage ( GetDlgItem ( hwndDlg, IDC_7COMBOPRIORITY ), CB_GETCURSEL, 0, 0 ),
                        (LPARAM)szBuf );
          DWORD dwPriority;
          GetPriorityClassFromPriorityString ( szBuf, dwPriority );
          SetPriorityClass ( hProcess, dwPriority );
          CloseHandle ( hProcess );
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }

        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }

        case IDC_7BUTTERMINATE:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }
          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, dwPID );
          if ( hProcess )
          {
            if ( !TerminateProcess ( hProcess, 0 ) )
            {
              CloseHandle ( hProcess );
              hProcess = NULL;
            }
            else
              CloseHandle ( hProcess );
          }
          if ( !hProcess )
          {
            MessageBox ( ghProcessInfoWindow, "Unable to Terminate the process\nRefresh this window & try again.",
                         "Process Info", 0 );
            break;
          }
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK THAPIListHeapsDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghTHHeapWindow = hwndDlg;
      HWND hHeapListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPS );
      HWND hEntryListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPENTRY );
      gnTHHeapEntryAscending = FALSE;
      gnTHHeapEntryColumn = -1;
      gnTHHeapListAscending = FALSE;
      gnTHHeapListColumn = -1;
      SendMessage ( hHeapListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      SendMessage ( hEntryListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hHeapListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPS );
      HWND hEntryListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPENTRY );
      HWND hSelectedWnd = GetDlgItem ( hwndDlg, IDC_SELECTEDHEAP );
      SetWindowText ( hSelectedWnd, "" );
      LoadAndFillTHHeapList ( hHeapListWnd, TRUE );
      LoadAndFillTHHeapEntryList ( hEntryListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hHeapListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPS );
      HWND hEntryListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPENTRY );
      DeleteListViewItems ( hHeapListWnd, TRUE );
      DeleteListViewItems ( hEntryListWnd, TRUE );
      ghTHHeapWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      HWND hHeapListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPS );
      HWND hEntryListWnd = GetDlgItem ( hwndDlg, IDC_4LISTHEAPENTRY );

      if ( pnmh->code == PSN_SETACTIVE )
      {
        PostMessage ( hwndDlg, WM_LOADDATA, wParam, lParam );
        break;
      }
      if ( pnmh->idFrom == IDC_4LISTHEAPS )
      {
        switch ( pnmh->code )
        {
          case LVN_COLUMNCLICK:
          {
            if ( pnml->iSubItem == gnTHHeapListColumn )
              gnTHHeapListAscending = !gnTHHeapListAscending;
            else
            {
              gnTHHeapListAscending = TRUE;
              gnTHHeapListColumn = pnml->iSubItem;
            }
            SendMessage ( hHeapListWnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListHeapCompareFunc );
            break;
          }

          case LVN_ITEMCHANGED:
          {
            INT nSel = pnml->iItem;
            if ( nSel == -1 )
              break;
            LV_ITEM lvi;
            ZeroMemory ( &lvi, sizeof(lvi) );
            lvi.mask = LVIF_PARAM | LVIF_STATE;
            lvi.iItem = nSel;
            SendMessage ( hHeapListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
            if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
              break;
            PHEAPLIST32 phl = (PHEAPLIST32)lvi.lParam;
            if ( !phl )
              break;
            char szBuf[128];
            wsprintf ( szBuf, "PID=%u, HID=%u", phl->th32ProcessID, phl->th32HeapID );
            SetWindowText ( GetDlgItem ( hwndDlg, IDC_SELECTEDHEAP ), szBuf );
            LoadAndFillTHHeapEntryList ( hEntryListWnd, TRUE );
          }
        }
      }
      else if ( pnmh->idFrom == IDC_4LISTHEAPENTRY )
      {
        switch ( pnmh->code )
        {
          case LVN_COLUMNCLICK:
          {
            if ( pnml->iSubItem == gnTHHeapEntryColumn )
              gnTHHeapEntryAscending = !gnTHHeapEntryAscending;
            else
            {
              gnTHHeapEntryAscending = TRUE;
              gnTHHeapEntryColumn = pnml->iSubItem;
            }
            SendMessage ( hEntryListWnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListHeapEntryCompareFunc );
            break;
          }
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTCLEARFILTER:
        {
          SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
          //Will Proceed Thru IDC_BUTREFRESH
        }

        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK THAPIListModulesDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghTHModuleWindow = hwndDlg;
      gnTHModuleListAscending = FALSE;
      gnTHModuleListColumn = -1;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_5LISTMODULE );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_5LISTMODULE );
      LoadAndFillTHModuleList ( hListWnd, TRUE );
			char szBuf[30];
			wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
			SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
			wsprintf ( szBuf, "Module Size : %d", gnModuleSize );
			SetWindowText ( GetDlgItem ( hwndDlg, IDC_MODULESIZE ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_5LISTMODULE );
      DeleteListViewItems ( hListWnd, FALSE );
      ghTHModuleWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_5LISTMODULE );

      if ( pnmh->code == PSN_SETACTIVE )
      {
        PostMessage ( hwndDlg, WM_LOADDATA, wParam, lParam );
        break;
      }
      if ( pnmh->idFrom == IDC_5LISTMODULE )
      {
        switch ( pnmh->code )
        {
          case LVN_COLUMNCLICK:
          {
            if ( pnml->iSubItem == gnTHModuleListColumn )
              gnTHModuleListAscending = !gnTHModuleListAscending;
            else
            {
              gnTHModuleListAscending = TRUE;
              gnTHModuleListColumn = pnml->iSubItem;
            }
            SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListModuleCompareFunc );
            break;
          }
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTCLEARFILTER:
        {
          SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
          //Will Proceed Thru IDC_BUTREFRESH
        }

        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK PSAPIPerformanceInfoDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghPSPerformanceInfoWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_12PAGEFILELIST );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      SetTimer ( hwndDlg, LOCAL_TIMER_ID, 300, NULL );
      return TRUE;
    }

    case WM_TIMER:
    {
      if ( wParam == LOCAL_TIMER_ID )
      {
        HWND hListWnd = GetDlgItem ( hwndDlg, IDC_12PAGEFILELIST );
        LoadAndFillPSPerformanceInfo ( hListWnd, TRUE );
		//LoadGlobalPSTHVectors (); //kannan-3
      }
      break;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_12PAGEFILELIST );
      LoadAndFillPSPerformanceInfo ( hListWnd, TRUE );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_12PAGEFILELIST );
      DeleteListViewItems ( hListWnd, FALSE );
      ghPSPerformanceInfoWindow = NULL;
      break;
    }

    case WM_RBUTTONDOWN:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_12PAGEFILELIST );
      DumpColumnWidth ( hListWnd, 5 );
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK THAPIListProcessDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghTHProcessWindow = hwndDlg;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_2LISTPROCESS );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      HWND hCombo = GetDlgItem ( hwndDlg, IDC_2COMBOPRIORITY );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"High" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Idle" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Norm" );
      SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Real" );
      gnTHProcessListColumn = -1;
      gnTHProcessListAscending = FALSE;
      HIMAGELIST hilEDW = ImageList_Create ( 14, 14, ILC_COLOR4, 1, 100 );
      ImageList_SetBkColor ( hilEDW, COLORREF ( 0x00FFFFFF ) );
      ImageList_AddIcon( hilEDW, LoadIcon ( ghInstance, "IDC_NULLICON" ) );
      ListView_SetImageList( hListWnd, hilEDW, LVSIL_SMALL );
      PostMessage ( hwndDlg, WM_LOADDATA, 0, 0 );
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_2LISTPROCESS );
      LoadAndFillTHProcessList ( hListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_2LISTPROCESS );
      DeleteListViewItems ( hListWnd, FALSE );
      ghTHProcessWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;

      if ( pnmh->idFrom != IDC_2LISTPROCESS )
        break;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_2LISTPROCESS );
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      switch ( pnmh->code )
      {
        case LVN_COLUMNCLICK:
        {
          if ( pnml->iSubItem == gnTHProcessListColumn )
            gnTHProcessListAscending = !gnTHProcessListAscending;
          else
          {
            gnTHProcessListAscending = TRUE;
            gnTHProcessListColumn = pnml->iSubItem;
          }
          SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListProcessCompareFunc );
          break;
        }

        case LVN_ITEMCHANGED:
        {
          INT nSel = pnml->iItem;
          if ( nSel == -1 )
            break;
          LV_ITEM lvi;
          ZeroMemory ( &lvi, sizeof(lvi) );
          lvi.mask = LVIF_PARAM | LVIF_STATE;
          lvi.iItem = nSel;
          SendMessage ( hListWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
          if ( !(pnml->uNewState & (LVIS_FOCUSED | LVIS_SELECTED ) ) )
            break;

          PROCESSENTRY32* pe = (PROCESSENTRY32*)lvi.lParam;
          if ( pe )
          {
            LPSTR pPtr = strrchr ( pe->szExeFile, '\\' );
            if ( pPtr )
              pPtr++;
            else
              pPtr = pe->szExeFile;
            char szBuf[512];
            wsprintf ( szBuf, "PID=%u -> %s", pe->th32ProcessID, pPtr );
            SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
            HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, pe->th32ProcessID );
            if ( hProcess )
            {
              DWORD dwPriority = GetPriorityClass ( hProcess );
              CloseHandle ( hProcess );
              GetPriorityStringFromPriorityClass ( dwPriority, szBuf );
              SendMessage ( GetDlgItem ( hwndDlg, IDC_2COMBOPRIORITY ), CB_SELECTSTRING, 0, (LPARAM)szBuf );
            }
          }
          break;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_2BUTSETPRIORITY:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }

          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_SET_INFORMATION, FALSE, dwPID );
          if ( !hProcess )
          {
            MessageBox ( hwndDlg, "Unable to change the Priority", "Process Info", 0 );
            break;
          }
          SendMessage ( GetDlgItem ( hwndDlg, IDC_2COMBOPRIORITY ), CB_GETLBTEXT,
                        (WPARAM)SendMessage ( GetDlgItem ( hwndDlg, IDC_2COMBOPRIORITY ), CB_GETCURSEL, 0, 0 ),
                        (LPARAM)szBuf );
          DWORD dwPriority;
          GetPriorityClassFromPriorityString ( szBuf, dwPriority );
          SetPriorityClass ( hProcess, dwPriority );
          CloseHandle ( hProcess );
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }
        
        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }

        case IDC_2BUTTERMINATE:
        {
          char szBuf[512];
          LPSTR pPtr;
          GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
          if ( lstrlen ( szBuf ) < 10 )
          {
            MessageBox ( hwndDlg, "Select a process first.", "Process Info", 0 );
            break;
          }
          pPtr = szBuf + 4;
          DWORD dwPID = atoi ( pPtr );
          HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, dwPID );
          if ( hProcess )
          {
            if ( !TerminateProcess ( hProcess, 0 ) )
            {
              CloseHandle ( hProcess );
              hProcess = NULL;
            }
            else
              CloseHandle ( hProcess );
          }
          if ( !hProcess )
          {
            MessageBox ( ghProcessInfoWindow, "Unable to Terminate the process\nRefresh this window & try again.",
                         "Process Info", 0 );
            break;
          }
          Sleep ( 400 );
          PostMessage ( hwndDlg, WM_COMMAND, (WPARAM)IDC_BUTREFRESH, (LPARAM)GetDlgItem ( hwndDlg, IDC_BUTREFRESH ) );
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL CALLBACK THAPIListThreadsDialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_INITDIALOG:
    {
      ghTHThreadWindow = hwndDlg;
      gnTHThreadListColumn = -1;
      gnTHThreadListAscending = FALSE;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_6LISTTHREAD );
      SendMessage ( hListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_FULLROWSELECT);
      return TRUE;
    }

    case WM_LOADDATA:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_6LISTTHREAD );
      LoadAndFillTHThreadList ( hListWnd, TRUE );
      char szBuf[20];
      wsprintf ( szBuf, "Count : %d", gnListCount ) , gnListCount=0;
      SetWindowText ( GetDlgItem ( hwndDlg, IDC_COUNT ), szBuf );
      break;
    }

    case WM_DESTROY:
    {
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_6LISTTHREAD );
      DeleteListViewItems ( hListWnd, FALSE );
      ghTHThreadWindow = NULL;
      break;
    }

    case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR)lParam;
      if ( !pnmh )
        break;
      NM_LISTVIEW* pnml =(NM_LISTVIEW*)lParam;
      HWND hListWnd = GetDlgItem ( hwndDlg, IDC_6LISTTHREAD );

      if ( pnmh->code == PSN_SETACTIVE )
      {
        PostMessage ( hwndDlg, WM_LOADDATA, wParam, lParam );
        break;
      }
      if ( pnmh->idFrom == IDC_6LISTTHREAD )
      {
        switch ( pnmh->code )
        {
          case LVN_COLUMNCLICK:
          {
            if ( pnml->iSubItem == gnTHThreadListColumn )
              gnTHThreadListAscending = !gnTHThreadListAscending;
            else
            {
              gnTHThreadListAscending = TRUE;
              gnTHThreadListColumn = pnml->iSubItem;
            }
            SendMessage ( hListWnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListThreadCompareFunc );
            break;
          }
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      ClientToScreen ( hwndDlg, &pt );
      TrackPopupMenu ( ghPropertySheetRTClickMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndDlg, NULL );
      break;
    }

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);
      if ( wID >= PIPS_RTMENU_BASE && wID < PIPS_RTMENU_MAX )
      {
        PropSheet_SetCurSel ( ghProcessInfoWindow, NULL, wID - PIPS_RTMENU_BASE );
        break;
      }
      switch ( wID )
      {
        case IDC_BUTCLEARFILTER:
        {
          SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
          //Will Proceed Thru IDC_BUTREFRESH
        }

        case IDC_BUTREFRESH:
        {
          LoadGlobalPSTHVectors ();
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

DWORD  WINAPI ProcessInfoThreadFunc ( LPVOID )
{
  PROPSHEETHEADER psheader;
  PROPSHEETPAGE pspage[20];
  MSG msg;
  HWND hBut;
  gnPages = 0;

  if ( ghPropertySheetRTClickMenu )
    DestroyMenu ( ghPropertySheetRTClickMenu );

  ZeroMemory (&psheader, sizeof(psheader));
  ZeroMemory (pspage, sizeof(pspage));
  psheader.dwSize = sizeof(psheader);
  psheader.dwFlags = PSH_MODELESS | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USEHICON;
  psheader.hwndParent = NULL;
  psheader.hInstance = ghInstance;
  psheader.hIcon = NULL;
  psheader.pszCaption = "System Process Information [By Kannan]";
  psheader.pfnCallback = NULL;
  psheader.ppsp = pspage;

  ghPropertySheetRTClickMenu = CreatePopupMenu ();

  //EnumDesktopWindows Application List
  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "ENUMDESKTOPWINDOWS_APPLIST";
  pspage[gnPages].pfnDlgProc = EnumDesktopWindowsDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "\tApplications" );
  gnPages++;

  //Tree For All Windows.
  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "ALLWINDOWLIST";
  pspage[gnPages].pfnDlgProc = ListAllWindowsDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "\tWindow List" );
  gnPages++;

  ghTHProcessWindow = NULL;
  ghTHModuleWindow = NULL;
  ghTHHeapWindow = NULL;
  ghTHThreadWindow = NULL;
  ghEDWWindow = NULL;
  ghAllWindowWindow = NULL;
  ghProcessTimesWindow = NULL;
  ghPSModulesWindow = NULL;
  ghPSProcessWindow = NULL;
  ghPSMemoryUsageWindow = NULL;
  ghPSDriversWindow = NULL;

  //Process Timing for NT Apps
  if (gbIsNT != NULL && (( ghinstToolHelp != NULL) || ( ghinstPSAPI != NULL && pEnumProcesses != NULL)) )
  {
    pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
    pspage[gnPages].dwFlags = 0;
    pspage[gnPages].hInstance = ghInstance;
    pspage[gnPages].pszTemplate = "PROCESS_TIMES";
    pspage[gnPages].pfnDlgProc = APIListProcessTimesDialogProc;
    AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "\tProcess Times" );
    gnPages++;
  }

  if ( !ghinstToolHelp ) goto lblOnlyPSAPI;

  //if ( !pProcess32First || !pProcess32Next ) goto lblOnlyPSAPI;

  //Toolhelp Process Window
  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "THAPI_PROCESSLIST";
  pspage[gnPages].pfnDlgProc = THAPIListProcessDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "THAPI\tProcess List" );
  gnPages++;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "THAPI_HEAPLIST";
  pspage[gnPages].pfnDlgProc = THAPIListHeapsDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "THAPI\tHeap List" );
  gnPages++;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "THAPI_MODULELIST";
  pspage[gnPages].pfnDlgProc = THAPIListModulesDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "THAPI\tModule List" );
  gnPages++;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "THAPI_THREADLIST";
  pspage[gnPages].pfnDlgProc = THAPIListThreadsDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "THAPI\tThread List" );
  gnPages++;

lblOnlyPSAPI:
  if ( ghinstPSAPI == NULL || pEnumProcesses == NULL ) goto lblNoPSAPI;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "PSAPI_PROCESSLIST";
  pspage[gnPages].pfnDlgProc = PSAPIListProcessDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "PSAPI\tProcess List" );
  gnPages++;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "PSAPI_MODULELIST";
  pspage[gnPages].pfnDlgProc = PSAPIListModulesDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "PSAPI\tModule List" );
  gnPages++;

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "PSAPI_MEMORYUSAGE";
  pspage[gnPages].pfnDlgProc = PSAPIListMemoryUsageDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "PSAPI\tMemory Usage" );
  gnPages++;

  if ( EnumDeviceDrivers != NULL && pGetDeviceDriverBaseName != NULL && pGetDeviceDriverFileName != NULL)
  {
    pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
    pspage[gnPages].dwFlags = 0;
    pspage[gnPages].hInstance = ghInstance;
    pspage[gnPages].pszTemplate = "PSAPI_DEVICEDRIVERS";
    pspage[gnPages].pfnDlgProc = PSAPIListDriversDialogProc;
    AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "PSAPI\tDevice Drivers" );
    gnPages++;
  }

  pspage[gnPages].dwSize = sizeof(PROPSHEETPAGE);
  pspage[gnPages].dwFlags = 0;
  pspage[gnPages].hInstance = ghInstance;
  pspage[gnPages].pszTemplate = "PSAPI_PERFORMANCEINFO";
  pspage[gnPages].pfnDlgProc = PSAPIPerformanceInfoDialogProc;
  AppendMenu ( ghPropertySheetRTClickMenu, MF_STRING, PIPS_RTMENU_BASE + gnPages, "PSAPI\tPerformance Info" );
  gnPages++;

lblNoPSAPI:

  psheader.nPages = gnPages;
  ghProcessInfoWindow = (HWND)PropertySheet ( &psheader );
  RECT rt;
  HWND hNewCtrls;
  CenterWindow ( ghProcessInfoWindow );
  GetClientRect ( ghProcessInfoWindow, &rt );
  hNewCtrls = CreateWindow ( "STATIC", "Selected Process: ", WS_CHILDWINDOW | WS_VISIBLE, 10, rt.bottom - 27, 125, 15,
                             ghProcessInfoWindow, NULL, ghInstance, NULL );
  SendMessage ( hNewCtrls, WM_SETFONT, (WPARAM) ghfont, 0 );
  ShowWindow ( hNewCtrls, SW_SHOW );
  hNewCtrls = CreateWindow ( "EDIT", "", WS_CHILDWINDOW | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
                             135, rt.bottom - 28, 270, 18, ghProcessInfoWindow, (HMENU)IDC_TXTSELPROCESS,
                             ghInstance, NULL );
  SendMessage ( hNewCtrls, WM_SETFONT, (WPARAM) ghfont ,  0 );
  ShowWindow ( hNewCtrls, SW_SHOW );
  hBut = GetDlgItem ( ghProcessInfoWindow, 1 );
  ShowWindow ( hBut, SW_HIDE );
  hBut = GetDlgItem ( ghProcessInfoWindow, 2 );
  SetWindowText ( hBut, "&Close" );

  LoadGlobalPSTHVectors ();

  while ( IsWindow ( ghProcessInfoWindow ) )
  {
    if ( gnAppExit )
    {
      DestroyWindow ( ghProcessInfoWindow );
      break;
    }
    while ( PeekMessage ( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
      if ( msg.message == WM_KEYDOWN && msg.wParam == VK_F5 )
        LoadGlobalPSTHVectors ();
      if ( !PropSheet_IsDialogMessage ( ghProcessInfoWindow, &msg  ) )
      {
        TranslateMessage ( &msg );
        IsDialogMessage ( msg.hwnd, &msg );
        DispatchMessage ( &msg );
      }
    }
    if ( !PropSheet_GetCurrentPageHwnd ( ghProcessInfoWindow ) )
    {
      DestroyWindow ( ghProcessInfoWindow );
      break;
    }
    Sleep ( 10 );
  }

  ghProcessInfoWindow = NULL;
  CloseHandle ( ghProcessInfoThread );
  UnLoadGlobalPSTHVectors ();
  ghTHProcessWindow = NULL;
  ghTHModuleWindow = NULL;
  ghTHHeapWindow = NULL;
  ghTHThreadWindow = NULL;
  ghProcessInfoThread = NULL;
  ghEDWWindow = NULL;
  ghAllWindowWindow = NULL;
  ghProcessTimesWindow = NULL;
  ghPSModulesWindow = NULL;
  ghPSProcessWindow = NULL;
  ghPSMemoryUsageWindow = NULL;
  ghPSDriversWindow = NULL;

  if ( ghPropertySheetRTClickMenu )
    DestroyMenu ( ghPropertySheetRTClickMenu );
  ghPropertySheetRTClickMenu = NULL;
  return 0;
}

HTREEITEM InsertItemInTree ( HWND hWndTree, HWND hwnd, HTREEITEM hParent )
{
  TCHAR WindowInfo[MAX_PATH*2];
  TCHAR szClassName[100];
  TCHAR szWindowName[100];
  DWORD dwPID;
  GetClassName( hwnd, szClassName, sizeof(szClassName) );
  GetWindowText( hwnd, szWindowName, sizeof(szWindowName) );
  GetWindowThreadProcessId ( hwnd, &dwPID );
  wsprintf ( WindowInfo, "%08X \"%s\" [%s] PID=%d", hwnd,
             szWindowName, szClassName, dwPID );
  TVINSERTSTRUCT tvitem;
  ZeroMemory ( &tvitem, sizeof ( TVINSERTSTRUCT ) );
  tvitem.hParent = hParent;
  tvitem.hInsertAfter = TVI_SORT;
  tvitem.item.mask = TVIF_TEXT|TVIF_IMAGE;
  HIMAGELIST hilEDW = TreeView_GetImageList ( hWndTree, TVSIL_NORMAL );
  HICON hIconInfo = NULL;
  if ( IsWindowVisible ( hwnd ) )
  {
    hIconInfo = (HICON)SendMessage ( hwnd, WM_GETICON, ICON_SMALL, 0 );
    if ( !hIconInfo )
    {
      hIconInfo = (HICON)SendMessage ( hwnd, WM_GETICON, ICON_BIG, 0 );
      if ( !hIconInfo )
      {
        hIconInfo = (HICON)GetClassLong ( hwnd, GCL_HICONSM );
        if ( !hIconInfo )
          hIconInfo = (HICON)GetClassLong ( hwnd, GCL_HICON );
      }
    }
  }
  if ( hIconInfo )
    ImageList_AddIcon ( hilEDW, (HICON) hIconInfo );

  if ( hIconInfo )
    tvitem.item.iImage = ImageList_GetImageCount ( hilEDW ) - 1;
  else
    tvitem.item.iImage = 0;
  tvitem.item.pszText = WindowInfo; 
  tvitem.item.cchTextMax = lstrlen ( WindowInfo );
  return ( TreeView_InsertItem ( hWndTree, &tvitem ) );
}

int CALLBACK EDWAppListCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  PAPPLIST pal1 = (PAPPLIST)lParam1;
  PAPPLIST pal2 = (PAPPLIST)lParam2;
  int nRet = 0;
  switch ( gnEDWAppListColumn )
  {
    case 0:
      nRet = ( lstrcmpi ( pal1->szAppName, pal2->szAppName ) > 0 ) ? 1 : 0;
      break;

    case 1:
      nRet = pal1->dwPID > pal2->dwPID;
      break;

    case 2:
      nRet = ( lstrcmpi ( pal1->szClassName, pal2->szClassName ) > 0 ) ? 1 : 0;
      break;

    case 3:
      nRet = pal1->hwnd > pal2->hwnd;
      break;

    case 4:
    {
      char szP1[10], szP2[10];
      GetPriorityStringFromPriorityClass ( pal1->dwPriority, szP1 );
      GetPriorityStringFromPriorityClass ( pal2->dwPriority, szP2 );
      nRet = ( lstrcmpi ( szP1, szP2 ) > 0 ) ? 1 : 0;
      break;
    }
  }
  return gnEDWAppListAscending ? nRet : !nRet;
}

INT CALLBACK ProcessTimeCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  INT nRet = 0;
  PPROCESSTIME ppt1 = (PPROCESSTIME)lParam1;
  PPROCESSTIME ppt2 = (PPROCESSTIME)lParam2;
  CHAR szBuf1[255], szBuf2[255];
  LPSTR pPtr1, pPtr2;

  switch ( gnProcessTimeListColumn )
  {
    case 0:
    {
      pPtr1 = strrchr ( ppt1->szProcessName, '\\' );
      pPtr2 = strrchr ( ppt2->szProcessName, '\\' );
      if ( !pPtr1 )
        pPtr1 = ppt1->szProcessName;
      if ( !pPtr2 )
        pPtr2 = ppt2->szProcessName;
      nRet = ( lstrcmpi ( pPtr1, pPtr2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 1:
      nRet = ppt1->dwPID > ppt2->dwPID;
      break;

    case 2:
      wsprintf ( szBuf1, "%hd.%hd", ppt1->wMajorVersion, ppt1->wMinrorVersion );
      wsprintf ( szBuf2, "%hd.%hd", ppt2->wMajorVersion, ppt2->wMinrorVersion );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;

    case 3:
      ConvertFileTime2String ( &ppt1->ftCreation, szBuf1 );
      ConvertFileTime2String ( &ppt2->ftCreation, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;

    case 4:
      ConvertFileTime2String ( &ppt1->ftKernelTime, szBuf1, TRUE );
      ConvertFileTime2String ( &ppt2->ftKernelTime, szBuf2, TRUE );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;

    case 5:
      ConvertFileTime2String ( &ppt1->ftUserTime, szBuf1, TRUE );
      ConvertFileTime2String ( &ppt2->ftUserTime, szBuf2, TRUE );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;
  }
  return gnProcessTimeListAscending ? nRet : !nRet;
}

INT CALLBACK PSAPIListDriversCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  INT nRet = 0;
  PPSAPIDEIVERLIST ppsdl1 = (PPSAPIDEIVERLIST)lParam1;
  PPSAPIDEIVERLIST ppsdl2 = (PPSAPIDEIVERLIST)lParam2;

  switch ( gnPSDriversListColumn )
  {
    case 0:
      nRet = ppsdl1->dwhDriver > ppsdl2->dwhDriver;
      break;

    case 1:
      nRet = ( lstrcmpi ( ppsdl1->szDriverName, ppsdl2->szDriverName ) > 0 ) ? 1 : 0;
      break;

    case 2:
      nRet = ( lstrcmpi ( ppsdl1->szDriverFileName, ppsdl2->szDriverFileName ) > 0 ) ? 1 : 0;
      break;
  }
  return gnPSDriversListAscending ? nRet : !nRet;
}

INT CALLBACK PSAPIListMemoryUsageCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  INT nRet = 0;
  PPSAPIPROCESSLIST ppspl1 = (PPSAPIPROCESSLIST)lParam1;
  PPSAPIPROCESSLIST ppspl2 = (PPSAPIPROCESSLIST)lParam2;
  LPSTR ptr1, ptr2;

  switch ( gnPSMemoryUsageListColumn )
  {
    case 0:
    {
      ptr1 = strrchr ( ppspl1->szProcessName, '\\' );
      ptr2 = strrchr ( ppspl2->szProcessName, '\\' );
      if ( ptr1 )
        ptr1++;
      else
        ptr1 = ppspl1->szProcessName;
      if ( ptr2 )
        ptr2++;
      else
        ptr2 = ppspl2->szProcessName;
      nRet = ( lstrcmpi ( ptr1, ptr2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 1:
      nRet = ppspl1->dwPID > ppspl2->dwPID;
      break;

    case 2:
      nRet = ppspl1->pmc.PageFaultCount > ppspl2->pmc.PageFaultCount;
      break;

    case 3:
      nRet = ppspl1->pmc.PeakWorkingSetSize > ppspl2->pmc.PeakWorkingSetSize;
      break;

    case 4:
      nRet = ppspl1->pmc.WorkingSetSize > ppspl2->pmc.WorkingSetSize;
      break;

    case 5:
      nRet = ppspl1->pmc.QuotaPeakPagedPoolUsage > ppspl2->pmc.QuotaPeakPagedPoolUsage;
      break;

    case 6:
      nRet = ppspl1->pmc.QuotaPagedPoolUsage > ppspl2->pmc.QuotaPagedPoolUsage;
      break;

    case 7:
      nRet = ppspl1->pmc.QuotaPeakNonPagedPoolUsage > ppspl2->pmc.QuotaPeakNonPagedPoolUsage;
      break;

    case 8:
      nRet = ppspl1->pmc.QuotaNonPagedPoolUsage > ppspl2->pmc.QuotaNonPagedPoolUsage;
      break;

    case 9:
      nRet = ppspl1->pmc.PagefileUsage > ppspl2->pmc.PagefileUsage;
      break;

    case 10:
      nRet = ppspl1->pmc.PeakPagefileUsage > ppspl2->pmc.PeakPagefileUsage;
      break;
  }
  return gnPSMemoryUsageListAscending ? nRet : !nRet;
}

INT CALLBACK PSAPIListModuleCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  INT nRet = 0;
  LPSTR ptr1, ptr2;
  PPSAPIMODULELIST ppsml1 = (PPSAPIMODULELIST)lParam1;
  PPSAPIMODULELIST ppsml2 = (PPSAPIMODULELIST)lParam2;
  
  switch ( gnPSModuleListColumn )
  {
    case 0:
      nRet = ppsml1->dwPID > ppsml2->dwPID;
      break;
    
    case 1:
      nRet = ppsml1->hModule > ppsml2->hModule;
      break;

    case 2:
    {
      ptr1 = strrchr ( ppsml1->szModuleName, '\\' );
      ptr2 = strrchr ( ppsml2->szModuleName, '\\' );
      if ( ptr1 )
        ptr1++;
      else
        ptr1 = ppsml1->szModuleName;
      if ( ptr2 )
        ptr2++;
      else
        ptr2 = ppsml2->szModuleName;
      nRet = ( lstrcmpi ( ptr1, ptr2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 3:
      nRet = ( lstrcmpi ( ppsml1->szModuleName, ppsml2->szModuleName ) > 0 ) ? 1 : 0;
      break;

    case 4:
      nRet = ( lstrcmpi ( ppsml1->szProcessName, ppsml2->szProcessName ) > 0 ) ? 1 : 0;
      break;

    case 5:
      nRet = (DWORD)ppsml1->mi.lpBaseOfDll > (DWORD)ppsml2->mi.lpBaseOfDll;
      break;

    case 6:
      nRet = (DWORD)ppsml1->mi.SizeOfImage > (DWORD)ppsml2->mi.SizeOfImage;
      break;

    case 7:
      nRet = (DWORD)ppsml1->mi.EntryPoint > (DWORD)ppsml2->mi.EntryPoint;
      break;
  }
  return gnPSModuleListAscending ? nRet : !nRet;
}

INT CALLBACK PSAPIListProcessCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  INT nRet = 0;
  PPSAPIPROCESSLIST ppspl1 = (PPSAPIPROCESSLIST)lParam1;
  PPSAPIPROCESSLIST ppspl2 = (PPSAPIPROCESSLIST)lParam2;
  LPSTR ptr1, ptr2;

  switch ( gnPSProcessListColumn )
  {
    case 0:
    {
      ptr1 = strrchr ( ppspl1->szProcessName, '\\' );
      ptr2 = strrchr ( ppspl2->szProcessName, '\\' );
      if ( ptr1 )
        ptr1++;
      else
        ptr1 = ppspl1->szProcessName;
      if ( ptr2 )
        ptr2++;
      else
        ptr2 = ppspl2->szProcessName;
      nRet = ( lstrcmpi ( ptr1, ptr2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 1:
      nRet = ppspl1->dwPID > ppspl2->dwPID;
      break;

    case 2:
      nRet = ppspl1->dwPriority > ppspl2->dwPriority;
      break;

    case 3:
      nRet = ( lstrcmpi ( ppspl1->szProcessName, ppspl2->szProcessName ) > 0 ) ? 1 : 0;
      break;
  }
  return gnPSProcessListAscending ? nRet : !nRet;
}

INT CALLBACK THAPIListHeapCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  HEAPLIST32* phl1 = (HEAPLIST32*)lParam1;
  HEAPLIST32* phl2 = (HEAPLIST32*)lParam2;
  int nRet = 0;
  
  switch ( gnTHHeapListColumn )
  {
    case 0:
    {
      nRet = phl1->th32ProcessID > phl2->th32ProcessID;
      break;
    }

    case 1:
    {
      nRet = phl1->th32HeapID > phl2->th32HeapID;
      break;
    }

    case 2:
    {
      char szBuf1[128], szBuf2[128];
      GetStringHeapListFlagFromDWORD ( phl1->dwFlags, szBuf1 );
      GetStringHeapListFlagFromDWORD ( phl2->dwFlags, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;
    }
  }
  return gnTHHeapListAscending ? nRet : !nRet;
}

INT CALLBACK THAPIListHeapEntryCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  PHEAPENTRY32 phe1 = (PHEAPENTRY32)lParam1;
  PHEAPENTRY32 phe2 = (PHEAPENTRY32)lParam2;
  int nRet = 0;

  switch ( gnTHHeapEntryColumn )
  {
    case 0:
    {
      nRet = phe1->th32ProcessID > phe2->th32ProcessID;
      break;
    }

    case 1:
    {
      nRet = phe1->th32HeapID > phe2->th32HeapID;
      break;
    }

    case 2:
    {
      nRet = phe1->hHandle > phe2->hHandle;
      break;
    }

    case 3:
    {
      nRet = phe1->dwAddress > phe2->dwAddress;
      break;
    }

    case 4:
    {
      nRet = phe1->dwBlockSize > phe2->dwBlockSize;
      break;
    }

    case 5:
    {
      nRet = phe1->dwLockCount > phe2->dwLockCount;
      break;
    }

    case 6:
    {
      char szBuf1[128], szBuf2[128];
      GetStringHeapEntryFlagFromDWORD ( phe1->dwFlags, szBuf1 );
      GetStringHeapEntryFlagFromDWORD ( phe2->dwFlags, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;
    }
  }
  return gnTHHeapEntryAscending ? nRet : !nRet;
}

int CALLBACK THAPIListModuleCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  PMODULEENTRY32 pme1 = (PMODULEENTRY32)lParam1;
  PMODULEENTRY32 pme2 = (PMODULEENTRY32)lParam2;
  int nRet = 0;

  switch ( gnTHModuleListColumn )
  {
    case 0:
      nRet = pme1->th32ProcessID > pme2->th32ProcessID;
      break;
      
    case 1:
      nRet = pme1->th32ModuleID > pme2->th32ModuleID;
      break;

    case 2:
      nRet = pme1->GlblcntUsage > pme2->GlblcntUsage;
      break;

    case 3:
      nRet = pme1->ProccntUsage > pme2->ProccntUsage;
      break;

    case 4:
      nRet = pme1->modBaseAddr > pme2->modBaseAddr;
      break;

    case 5:
      nRet = pme1->modBaseSize > pme2->modBaseSize;
      break;

    case 6:
      nRet = pme1->hModule > pme2->hModule;
      break;

    case 7:
      nRet = ( lstrcmpi ( pme1->szModule, pme2->szModule ) > 0 ) ? 1 : 0;
      break;

    case 8:
      nRet = ( lstrcmpi ( pme1->szExePath, pme2->szExePath ) > 0 ) ? 1 : 0;
      break;

    case 9:
    {
      char szBuf1[MAX_PATH], szBuf2[MAX_PATH];
      GetTHProcessNameFromProcessID ( pme1->th32ProcessID, szBuf1 );
      GetTHProcessNameFromProcessID ( pme2->th32ProcessID, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
    }
  }
  return gnTHModuleListAscending ? nRet : !nRet;
}

int CALLBACK THAPIListProcessCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  PROCESSENTRY32* pe1 = (PROCESSENTRY32*)lParam1;
  PROCESSENTRY32* pe2 = (PROCESSENTRY32*)lParam2;
  int nRet = 0;
  LPSTR pPtr1, pPtr2;

  switch ( gnTHProcessListColumn )
  {
    case 0:
    {
      pPtr1 = strrchr ( pe1->szExeFile, '\\' );
      if ( pPtr1 )
        pPtr1++;
      else
        pPtr1 = pe1->szExeFile;
      pPtr2 = strrchr ( pe2->szExeFile, '\\' );
      if ( pPtr2 )
        pPtr2++;
      else
        pPtr2 = pe2->szExeFile;
      nRet = ( lstrcmpi ( pPtr1, pPtr2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 1:
      nRet = pe1->th32ProcessID > pe2->th32ProcessID;
      break;

    case 2:
      nRet = pe1->th32ModuleID > pe2->th32ModuleID;
      break;

    case 3:
      nRet = pe1->cntThreads > pe2->cntThreads;
      break;

    case 4:
      nRet = pe1->pcPriClassBase > pe2->pcPriClassBase;
      break;

    case 5:
      nRet = pe1->cntUsage > pe2->cntUsage;
      break;

    case 6:
      nRet = pe1->th32ParentProcessID > pe2->th32ParentProcessID;
      break;

    case 7:
      nRet = pe1->th32DefaultHeapID > pe2->th32DefaultHeapID;
      break;

    case 8:
      nRet = pe1->dwFlags > pe2->dwFlags;
      break;

    case 9:
      nRet = ( lstrcmpi ( pe1->szExeFile, pe2->szExeFile ) > 0 ) ? 1 : 0;
      break;
  }
  return gnTHProcessListAscending ? nRet : !nRet;
}

int CALLBACK THAPIListThreadCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
  PTHREADENTRY32 pte1 = (PTHREADENTRY32)lParam1;
  PTHREADENTRY32 pte2 = (PTHREADENTRY32)lParam2;
  int nRet = 0;

  switch ( gnTHThreadListColumn )
  {
    case 0:
      nRet = pte1->th32OwnerProcessID > pte2->th32OwnerProcessID;
      break;

    case 1:
      nRet = pte1->th32ThreadID > pte2->th32ThreadID;
      break;

    case 2:
      nRet = pte1->cntUsage > pte2->cntUsage;
      break;

    case 3:
    {
      char szBuf1[128], szBuf2[128];
      GetStringThreadBasePriorityFromDWORD ( pte1->tpBasePri, szBuf1 );
      GetStringThreadBasePriorityFromDWORD ( pte2->tpBasePri, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
      break;
    }

    case 4:
      nRet = pte1->tpDeltaPri > pte2->tpDeltaPri;
      break;

    case 5:
    {
      char szBuf1[MAX_PATH], szBuf2[MAX_PATH];
      GetTHProcessNameFromProcessID ( pte1->th32OwnerProcessID, szBuf1 );
      GetTHProcessNameFromProcessID ( pte2->th32OwnerProcessID, szBuf2 );
      nRet = ( lstrcmpi ( szBuf1, szBuf2 ) > 0 ) ? 1 : 0;
    }
  }
  return gnTHThreadListAscending ? nRet : !nRet;
}

VOID DeleteListViewItems ( HWND hwnd, BOOL bClearParams )
{
  INT nCount;
  if ( bClearParams )
  {
    LV_ITEM lvi;
    ZeroMemory ( &lvi, sizeof(lvi) );
    lvi.mask = LVIF_PARAM;
    nCount = (INT)SendMessage ( hwnd, LVM_GETITEMCOUNT, 0, 0 );
    for ( int i = 0; i < nCount; i++ )
    {
      lvi.iItem = i;
      SendMessage ( hwnd, LVM_GETITEM, 0, (LPARAM)&lvi );
      if ( lvi.lParam )
        delete (LPVOID)lvi.lParam;
    }
  }
  SendMessage ( hwnd, LVM_DELETEALLITEMS, 0, 0 );
  HIMAGELIST hil = ListView_GetImageList ( hwnd, LVSIL_SMALL );
  if ( hil )
  {
    nCount = ImageList_GetImageCount ( hil );
    while ( nCount > 1 )
    {
      if ( !ImageList_Remove ( hil, 1 ) )
        break;
      nCount = ImageList_GetImageCount ( hil );
    }
  }
}

VOID GetStringHeapListFlagFromDWORD ( DWORD dwFlags, LPSTR szBuf )
{
  *szBuf = NULL;
  if ( dwFlags & HF32_DEFAULT )
    lstrcpy ( szBuf, "HF32_DEFAULT" );
  if ( dwFlags & HF32_SHARED )
  {
    if ( *szBuf )
      lstrcat ( szBuf, " | " );
    lstrcat ( szBuf, "HF32_SHARED" );
  }
  wsprintf ( szBuf + lstrlen ( szBuf ), *szBuf ? " (%08X)" : "(%08X)", dwFlags );
}

VOID GetStringHeapEntryFlagFromDWORD ( DWORD dwFlags, LPSTR szBuf )
{
  *szBuf = NULL;
  if ( dwFlags & LF32_FIXED )
    lstrcpy ( szBuf, "LF32_FIXED" );
  if ( dwFlags & LF32_FREE )
  {
    if ( *szBuf )
      lstrcat ( szBuf, " | " );
    lstrcat ( szBuf, "LF32_FREE" );
  }
  if ( dwFlags & LF32_MOVEABLE )
  {
    if ( *szBuf )
      lstrcat ( szBuf, " | " );
    lstrcat ( szBuf, "LF32_MOVEABLE" );
  }
  wsprintf ( szBuf + lstrlen ( szBuf ), *szBuf ? " (%08X)" : "(%08X)", dwFlags );
}

VOID GetTHProcessNameFromProcessID ( DWORD dwPID, LPSTR szBuf )
{
  *szBuf = NULL;
  INT nSize = gvTHProcessList.size ();
  INT i;
  PROCESSENTRY32* pe;
  for ( i = 0; i < nSize; i++ )
  {
    pe = gvTHProcessList[i];
    if ( dwPID == pe->th32ProcessID )
    {
      lstrcpy ( szBuf, pe->szExeFile );
      return;
    }
  }
}

VOID InitProcessHandlers ()
{
  ghinstPSAPI = LoadLibrary("Psapi.dll");
  if (ghinstPSAPI == NULL) goto ToolHelp;
  pEmptyWorkingSet = (LPEMPTYWORKINGSET)GetProcAddress(ghinstPSAPI, "EmptyWorkingSet");
  pEnumDeviceDrivers = (LPENUMDEVICEDRIVERS)GetProcAddress(ghinstPSAPI, "EnumDeviceDrivers");
  pEnumPageFiles = (LPENUMPAGEFILES)GetProcAddress(ghinstPSAPI, "EnumPageFilesA");
  pEnumProcesses = (LPENUMPROCESSES)GetProcAddress(ghinstPSAPI, "EnumProcesses");
  pEnumProcessModules = (LPENUMPROCESSMODULES)GetProcAddress(ghinstPSAPI, "EnumProcessModules");
  if (pEnumProcessModules != NULL)
    gcProcessFinderMethod = 'P';
  pGetDeviceDriverBaseName = (LPGETDEVICEDRIVERBASENAME)GetProcAddress(ghinstPSAPI, "GetDeviceDriverBaseNameA");
  pGetDeviceDriverFileName = (LPGETDEVICEDRIVERFILENAME)GetProcAddress(ghinstPSAPI, "GetDeviceDriverFileNameA");
  pGetMappedFileName = (LPGETMAPPEDFILENAME)GetProcAddress(ghinstPSAPI, "GetMappedFileNameA");
  pGetModuleBaseName = (LPGETMODULEBASENAME)GetProcAddress(ghinstPSAPI, "GetModuleBaseNameA");
  pGetModuleFileNameEx = (LPGETMODULEFILENAMEEX)GetProcAddress(ghinstPSAPI, "GetModuleFileNameExA");
  pGetModuleInformation = (LPGETMODULEINFORMATION)GetProcAddress(ghinstPSAPI, "GetModuleInformation");
  pGetPerformanceInfo = (LPGETPERFORMANCEINFO)GetProcAddress(ghinstPSAPI, "GetPerformanceInfo");
  pGetProcessImageFileName = (LPGETPROCESSIMAGEFILENAME)GetProcAddress(ghinstPSAPI, "GetProcessImageFileNameA");
  pGetProcessMemoryInfo = (LPGETPROCESSMEMORYINFO)GetProcAddress(ghinstPSAPI, "GetProcessMemoryInfo");
  pGetWsChanges = (LPGETWSCHANGES)GetProcAddress(ghinstPSAPI, "GetWsChanges");
  pInitializeProcessForWsWatch = (LPINITIALIZEPROCESSFORWSWATCH)GetProcAddress(ghinstPSAPI, "InitializeProcessForWsWatch");
  pQueryWorkingSet = (LPQUERYWORKINGSET)GetProcAddress(ghinstPSAPI, "QueryWorkingSet");

ToolHelp:
  ghinstToolHelp = LoadLibrary ( "Kernel32.dll" );
  if (ghinstToolHelp == NULL) return;

  pCreateToolhelp32Snapshot = (LPCREATETOOLHELP32SNAPSHOT)GetProcAddress(ghinstToolHelp, "CreateToolhelp32Snapshot");
  if (pCreateToolhelp32Snapshot == NULL)
  {
    FreeLibrary ( ghinstToolHelp );
    ghinstToolHelp = NULL;
	return;
  }
  pHeap32First = (LPHEAP32FIRST)GetProcAddress(ghinstToolHelp, "Heap32First");
  pHeap32ListFirst = (LPHEAP32LISTFIRST)GetProcAddress(ghinstToolHelp, "Heap32ListFirst");
  pHeap32ListNext = (LPHEAP32LISTNEXT)GetProcAddress(ghinstToolHelp, "Heap32ListNext");
  pHeap32Next = (LPHEAP32NEXT)GetProcAddress(ghinstToolHelp, "Heap32Next");
  pModule32First = (LPMODULE32FIRST)GetProcAddress(ghinstToolHelp, "Module32First");
  pModule32Next = (LPMODULE32NEXT)GetProcAddress(ghinstToolHelp, "Module32Next");
  if (gcProcessFinderMethod != 'P' && pModule32First && pModule32Next) //kannan-1
    gcProcessFinderMethod = 'T';
  pProcess32First = (LPPROCESS32FIRST)GetProcAddress(ghinstToolHelp, "Process32First");
  pProcess32Next = (LPPROCESS32NEXT)GetProcAddress(ghinstToolHelp, "Process32Next");
  pThread32First = (LPTHREAD32FIRST)GetProcAddress(ghinstToolHelp, "Thread32First");
  pThread32Next = (LPTHREAD32NEXT)GetProcAddress(ghinstToolHelp, "Thread32Next");
  pToolhelp32ReadProcessMemory = (LPTOOLHELP32READPROCESSMEMORY)GetProcAddress(ghinstToolHelp, "Toolhelp32ReadProcessMemory");
}

VOID ListProcesses ()
{
  SECURITY_ATTRIBUTES sa;
  DWORD dwThreadId;

  ZeroMemory ( &sa, sizeof(sa) );
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  dwThreadId = 0;
  ghProcessInfoThread = CreateThread ( &sa, NULL, ProcessInfoThreadFunc, NULL, 0, &dwThreadId );
}

VOID LoadAndFillEDWAppList ( HWND hwnd, BOOL bLoadColumns )
{
  CHAR szBuf[MAX_PATH];
  LV_COLUMN lvc;
  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;

  bLoadColumns = TRUE;
  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Window Title";
    lvc.cx = 197;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Process ID";
    lvc.cx = 36;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Class Name";
    lvc.cx = 115;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "HWND";
    lvc.cx = 65;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Process Priority";
    lvc.cx = 42;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  PAPPLIST pal;
  HIMAGELIST hilEDW = ListView_GetImageList ( hwnd, LVSIL_SMALL );
  INT i, nSize;

  nSize = gvAppList.size ();
  gnListCount = 0;
  for ( i = 0; i < nSize; i++ )
  {
    pal = gvAppList[i];
    HICON hIconInfo = (HICON)SendMessage ( pal->hwnd, WM_GETICON, ICON_SMALL, 0 );
    if ( !hIconInfo )
    {
      hIconInfo = (HICON)SendMessage ( pal->hwnd, WM_GETICON, ICON_BIG, 0 );
      if ( !hIconInfo )
      {
        hIconInfo = (HICON)GetClassLong ( pal->hwnd, GCL_HICONSM );
        if ( !hIconInfo )
          hIconInfo = (HICON)GetClassLong ( pal->hwnd, GCL_HICON );
      }
    }
    if ( hIconInfo )
      ImageList_AddIcon( hilEDW, (HICON)hIconInfo );

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    if ( hIconInfo )
      lvi.iImage = ImageList_GetImageCount ( hilEDW ) - 1;
    else
      lvi.iImage = 0;
    lvi.pszText = pal->szAppName;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)pal;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;

    lvi.iSubItem = 2;
    lvi.pszText = pal->szClassName;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    wsprintf ( szBuf, "%08X", pal->hwnd );
    lvi.pszText = szBuf;
    lvi.iSubItem = 3;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    wsprintf ( szBuf, "%u", pal->dwPID );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    GetPriorityStringFromPriorityClass ( pal->dwPriority, szBuf );
    lvi.iSubItem = 4;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnEDWAppListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)EDWAppListCompareFunc );
}

VOID LoadAndFillProcessTimeList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process Name";
    lvc.cx = 95;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Process ID";
    lvc.cx = 39;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Process Version";
    lvc.cx = 36;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Creation Time";
    lvc.cx = 120;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Kernel Time";
    lvc.cx = 75;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "User Time";
    lvc.cx = 75;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  INT nSize, i;
  PPROCESSTIME ppt;
  LV_ITEM lvi;
  INT iItem;
  LPSTR pPtr;

  ZeroMemory ( &lvi, sizeof(lvi) );
  nSize = gvProcessTimes.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppt = gvProcessTimes[i];

    pPtr = strrchr ( ppt->szProcessName, '\\' );
    if ( pPtr )
      pPtr++;
    else
      pPtr = ppt->szProcessName;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = pPtr;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)ppt;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppt->dwPID );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%hd.%hd", ppt->wMajorVersion, ppt->wMinrorVersion );
    lvi.iSubItem = 2;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    ConvertFileTime2String ( &ppt->ftCreation, szBuf );
    lvi.iSubItem = 3;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    ConvertFileTime2String ( &ppt->ftKernelTime, szBuf, TRUE );
    lvi.iSubItem = 4;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    ConvertFileTime2String ( &ppt->ftUserTime, szBuf, TRUE );
    lvi.iSubItem = 5;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
  }
  if ( gnProcessTimeListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)ProcessTimeCompareFunc );
}

VOID LoadAndFillPSDriverList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[64];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
	lvc.pszText = "Driver Name";
    lvc.cx = 84;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Driver Address";
    lvc.cx = 117;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "File Name";
    lvc.cx = 234;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  INT nSize, i;
  nSize = gvPSDriverList.size ();
  PPSAPIDEIVERLIST ppsdl;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  gnListCount = 0;

  for ( i = 0; i < nSize; i++ )
  {
    ppsdl = gvPSDriverList[i];
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.pszText = ppsdl->szDriverName;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)ppsdl;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%08X", ppsdl->dwhDriver );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = ppsdl->szDriverFileName;
    lvi.iSubItem = 2;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnPSDriversListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListDriversCompareFunc );
}

VOID LoadAndFillPSMemoryUsageList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process Name";
    lvc.cx = 100;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Process ID";
    lvc.cx = 42;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Page Fault Count";
    lvc.cx = 57;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Peak Working Set Size";
    lvc.cx = 68;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Working Set Size";
    lvc.cx = 64;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Quota Peak Paged Pool Usage";
    lvc.cx = 55;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
    lvc.pszText = "Quota Paged Pool Usage";
    lvc.cx = 55;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 6, (LPARAM)&lvc );
    lvc.pszText = "Quota Peak Non-Paged Pool Usage";
    lvc.cx = 55;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 7, (LPARAM)&lvc );
    lvc.pszText = "Quota Non-Paged Pool Usage";
    lvc.cx = 55;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 8, (LPARAM)&lvc );
    lvc.pszText = "Page File Usage";
    lvc.cx = 67;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 9, (LPARAM)&lvc );
    lvc.pszText = "Peak Page File Usage";
    lvc.cx = 68;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 10, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  INT nSize, i;
  nSize = gvPSProcessList.size ();
  PPSAPIPROCESSLIST ppspl;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  LPSTR pPtr;
  gnListCount = 0;

  for ( i = 0; i < nSize; i++ )
  {
    ppspl = gvPSProcessList[i];

    pPtr = strrchr ( ppspl->szProcessName, '\\' );
    if ( pPtr )
      pPtr++;
    else
      pPtr = ppspl->szProcessName;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = pPtr;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)ppspl;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->dwPID );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.PageFaultCount );
    lvi.iSubItem = 2;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.PeakWorkingSetSize );
    lvi.iSubItem = 3;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.WorkingSetSize );
    lvi.iSubItem = 4;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.QuotaPeakPagedPoolUsage );
    lvi.iSubItem = 5;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.QuotaPagedPoolUsage );
    lvi.iSubItem = 6;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.QuotaPeakNonPagedPoolUsage );
    lvi.iSubItem = 7;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.QuotaNonPagedPoolUsage );
    lvi.iSubItem = 8;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.PagefileUsage );
    lvi.iSubItem = 9;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->pmc.PeakPagefileUsage );
    lvi.iSubItem = 10;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnPSMemoryUsageListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListMemoryUsageCompareFunc );
}

VOID LoadAndFillPSModuleList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process ID";
    lvc.cx = 69;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Module Handle";
    lvc.cx = 87;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Module Name";
    lvc.cx = 85;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Module Path";
    lvc.cx = 101;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Process Name";
    lvc.cx = 93;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Base Address";
    lvc.cx = 80;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
    lvc.pszText = "Module Size";
    lvc.cx = 73;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 6, (LPARAM)&lvc );
    lvc.pszText = "Entry Point";
    lvc.cx = 75;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 7, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  LPSTR pPtr;
  DWORD dwPID = 0;
  GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
  if ( lstrlen ( szBuf ) > 10 )
  {
    pPtr = szBuf + 4;
    dwPID = atoi ( pPtr );
  }

  INT nSize, i;
  nSize = gvPSModuleList.size ();
  PPSAPIMODULELIST ppsml;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  gnListCount = 0;
	gnModuleSize = 0;

  for ( i = 0; i < nSize; i++ )
  {
    ppsml = gvPSModuleList[i];
    if ( dwPID && dwPID != ppsml->dwPID )
      continue;
    wsprintf ( szBuf, "%u", ppsml->dwPID );
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)ppsml;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = szBuf;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );
    
    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;

    lvi.iSubItem = 1;
    wsprintf ( szBuf, "%u", ppsml->hModule );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    
    lvi.iSubItem = 2;
    pPtr = strrchr ( ppsml->szModuleName, '\\' );
    if ( pPtr )
      pPtr++;
    else
      pPtr = ppsml->szModuleName;
    lvi.pszText = pPtr;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    
    lvi.iSubItem = 3;
    lvi.pszText = ppsml->szModuleName;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    
    lvi.iSubItem = 4;
    lvi.pszText = ppsml->szProcessName;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    
    lvi.iSubItem = 5;
    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%08X", ppsml->mi.lpBaseOfDll );
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

		gnModuleSize += ppsml->mi.SizeOfImage;
    lvi.iSubItem = 6;
    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppsml->mi.SizeOfImage );
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.iSubItem = 7;
    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%08X", ppsml->mi.EntryPoint );
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnPSModuleListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListModuleCompareFunc );
}

BOOL WINAPI EnumPageFilesProc ( LPVOID pContext, PENUM_PAGE_FILE_INFORMATION pPageFileInfo, LPCTSTR lpFilename )
{
  HWND hwnd = (HWND)pContext;
  LV_ITEM lvi;
  CHAR szBuf[32];

  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  lvi.mask = LVIF_TEXT;
  lvi.pszText = lpFilename ? (LPSTR)lpFilename : "<Unknown>";
  lvi.iSubItem = 0;
  iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

  lvi.mask = LVIF_TEXT;
  lvi.iItem = iItem;
  lvi.iSubItem = 1;
  lvi.pszText = szBuf;
  wsprintf ( szBuf, "%u", pPageFileInfo->TotalSize );
  SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

  lvi.iSubItem = 2;
  lvi.pszText = szBuf;
  wsprintf ( szBuf, "%u", pPageFileInfo->TotalInUse );
  SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

  lvi.iSubItem = 3;
  lvi.pszText = szBuf;
  wsprintf ( szBuf, "%u", pPageFileInfo->PeakUsage );
  SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
  return TRUE;
}

VOID LoadAndFillPSPerformanceInfo ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Page File Name";
    lvc.cx = 239;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Total Size";
    lvc.cx = 65;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Total In Use";
    lvc.cx = 72;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Peak Usage";
    lvc.cx = 78;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    HWND hDlg = GetParent ( hwnd );
    OSVERSIONINFOEX ovi;
    ZeroMemory ( &ovi, sizeof(OSVERSIONINFOEX) );
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx ( (LPOSVERSIONINFO)&ovi );
    CHAR szOSName[255];
    if ( ovi.dwMajorVersion == 4 && ovi.dwMinorVersion == 0 && ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
      lstrcpy ( szOSName, "Windows 95" );
    else if ( ovi.dwMajorVersion == 4 && ovi.dwMinorVersion == 10 && ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
      lstrcpy ( szOSName, "Windows 98" );
    else if ( ovi.dwMajorVersion == 4 && ovi.dwMinorVersion == 90 && ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
      lstrcpy ( szOSName, "Windows Me" );
    else if ( ovi.dwMajorVersion == 4 && ovi.dwMinorVersion == 0  )
      lstrcpy ( szOSName, "Windows NT 4.0" );
    else if ( ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 0 )
      lstrcpy ( szOSName, "Windows 2000" );
    else if ( ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 1 )
      lstrcpy ( szOSName, "Windows XP" );
    else if ( ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 2 )
      lstrcpy ( szOSName, "Windows .NET Server" );
    else
      lstrcpy ( szOSName, "Unknown Windows" );
    wsprintf ( szBuf, "%s (%u.%u)(Build %u) %s (SP %hd.%hd)", szOSName, ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber, ovi.szCSDVersion, ovi.wServicePackMajor, ovi.wServicePackMinor );
    
	/*if ( ovi.szCSDVersion & VER_SUITE_BACKOFFICE )
      lstrcat ( szBuf, " Back Office" );
    if ( ovi.szCSDVersion & VER_SUITE_BLADE )
      lstrcat ( szBuf, " .NET Web Server" );
    if ( ovi.szCSDVersion & VER_SUITE_DATACENTER )
      lstrcat ( szBuf, " Datacenter Ed." );
    if ( ovi.szCSDVersion & VER_SUITE_ENTERPRISE )
      lstrcat ( szBuf, " Enterprise Ed." );
    if ( ovi.szCSDVersion & VER_SUITE_PERSONAL )
      lstrcat ( szBuf, " Home Ed." );
    if ( ovi.szCSDVersion & VER_SUITE_SMALLBUSINESS )
      lstrcat ( szBuf, " Small Business Svr." );
    if ( ovi.szCSDVersion & VER_SUITE_SMALLBUSINESS_RESTRICTED )
      lstrcat ( szBuf, " Resticted Client" );
    if ( ovi.szCSDVersion & VER_SUITE_TERMINAL )
      lstrcat ( szBuf, " Terminal Svr." );
    if ( ovi.wProductType == VER_NT_WORKSTATION )
      lstrcat ( szBuf, " <Workstation>" );
    if ( ovi.wProductType == VER_NT_DOMAIN_CONTROLLER )
      lstrcat ( szBuf, " <Domain Controller>" );
    if ( ovi.wProductType == VER_NT_SERVER )
      lstrcat ( szBuf, " <Server>" );*/
    
    SetWindowText ( GetDlgItem ( hDlg, IDC_12OSINFO ), szBuf );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  if (pEnumPageFiles != NULL)
      pEnumPageFiles(&EnumPageFilesProc, (LPVOID)hwnd );

  if (pGetPerformanceInfo != NULL)
  {
    PERFORMACE_INFORMATION PerformanceInformation;
    ZeroMemory (&PerformanceInformation, sizeof(PERFORMACE_INFORMATION));

    if (pGetPerformanceInfo(&PerformanceInformation, sizeof(PERFORMACE_INFORMATION)))
    {
      HWND hDlg = GetParent ( hwnd );
      DWORD dwPageSize = (DWORD)PerformanceInformation.PageSize;
      wsprintf ( szBuf, "%u", PerformanceInformation.HandleCount );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALHANDLES ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.ThreadCount );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALTHREADS ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.ProcessCount );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALPROCESSES ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.PhysicalTotal * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALPM ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.PhysicalAvailable * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12AVAILABLEPM ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.SystemCache * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12SYSTEMCACHE ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.CommitTotal * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALCOMMIT ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.CommitLimit * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12LIMITCOMMIT ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.CommitPeak * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12PEAKCOMMIT ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.KernelTotal * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALKERNEL ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.KernelPaged * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALPAGED ), szBuf );
      wsprintf ( szBuf, "%u", PerformanceInformation.KernelNonpaged * dwPageSize / 1024 );
      SetWindowText ( GetDlgItem ( hDlg, IDC_12TOTALNONPAGED ), szBuf );
    }
  }
}

VOID LoadAndFillPSProcessList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process Name";
    lvc.cx = 159;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Process ID";
    lvc.cx = 70;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Priority";
    lvc.cx = 55;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Process Full Path";
    lvc.cx = 150;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  INT nSize, i;
  nSize = gvPSProcessList.size ();
  PPSAPIPROCESSLIST ppspl;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  LPSTR pPtr;
  HIMAGELIST hilEDW = ListView_GetImageList ( hwnd, LVSIL_SMALL );
  gnListCount = 0;

  for ( i = 0; i < nSize; i++ )
  {
    ppspl = gvPSProcessList[i];

    pPtr = strrchr ( ppspl->szProcessName, '\\' );
    HICON hIconInfo = ExtractIcon ( ghInstance, ppspl->szProcessName, 0 );
    if ( hIconInfo )
      ImageList_AddIcon( hilEDW, (HICON)hIconInfo );
    if ( pPtr )
      pPtr++;
    else
      pPtr = ppspl->szProcessName;
    lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    if ( hIconInfo )
      lvi.iImage = ImageList_GetImageCount ( hilEDW ) - 1;
    else
      lvi.iImage = 0;
    lvi.pszText = pPtr;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)ppspl;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.pszText = ppspl->szProcessName;
    lvi.lParam = 0;
    lvi.iSubItem = 3;
    lvi.iItem = iItem;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->dwPID );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    lvi.pszText = szBuf;
    wsprintf ( szBuf, "%u", ppspl->dwPriority );
    lvi.iSubItem = 2;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnPSProcessListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)PSAPIListProcessCompareFunc );
}

VOID LoadAndFillTHHeapEntryList ( HWND hwnd, BOOL bLoadColumns )
{
  HANDLE hSnap; 
  LV_COLUMN lvc;
  char szBuf[512];
  
  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process ID";
    lvc.cx = 44;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Heap ID";
    lvc.cx = 69;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Handle";
    lvc.cx = 69;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Start Address";
    lvc.cx = 70;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Size";
    lvc.cx = 59;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Lock Count";
    lvc.cx = 33;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
    lvc.pszText = "Flags";
    lvc.cx = 87;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 6, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, TRUE );

  LPSTR pPtr;
  GetWindowText ( GetDlgItem ( GetParent ( hwnd ), IDC_SELECTEDHEAP ), szBuf, sizeof(szBuf) );
  if ( lstrlen ( szBuf ) < 10 )
    return;

  pPtr = szBuf + 4;
  DWORD dwPID = atoi ( pPtr );
  DWORD dwHID = atoi ( strchr ( pPtr + 1, '=' ) + 1 );

  hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPHEAPLIST, dwPID ); 

  if ( hSnap == INVALID_HANDLE_VALUE ) 
    return;

  HEAPENTRY32 he32;
  ZeroMemory ( &he32, sizeof(he32) );
  he32.dwSize = sizeof(he32);
  int nCount = 0;
  if ( pHeap32First ( &he32, dwPID, dwHID ) )
  {
    HEAPENTRY32* phe;
    LV_ITEM lvi;
    ZeroMemory ( &lvi, sizeof(lvi) );
    INT iItem;

    do
    {
      if ( nCount > MAX_HEAP_ENTRIES )
        break;
      phe = new HEAPENTRY32;
      memcpy ( phe, &he32, sizeof(HEAPENTRY32) );
      wsprintf ( szBuf, "%u", phe->th32ProcessID );
      lvi.iSubItem = 0;
      lvi.lParam = (LPARAM)phe;
      lvi.mask = LVIF_TEXT | LVIF_PARAM;
      lvi.pszText = szBuf;
      iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );
      lvi.mask = LVIF_TEXT;
      lvi.lParam = 0;
      lvi.iItem = iItem;
      lvi.iSubItem = 1;
      wsprintf ( szBuf, "%u", phe->th32HeapID );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 2;
      wsprintf ( szBuf, "%u", phe->hHandle );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 3;
      wsprintf ( szBuf, "%08X", phe->dwAddress );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 4;
      wsprintf ( szBuf, "%u", phe->dwBlockSize );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 5;
      wsprintf ( szBuf, "%u", phe->dwLockCount );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 6;
      GetStringHeapEntryFlagFromDWORD ( phe->dwFlags, szBuf );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      nCount++;
    } while ( pHeap32Next ( &he32 ) );
    if ( nCount > MAX_HEAP_ENTRIES )
    {
      wsprintf ( szBuf, "Too many Heap Entries\r\nTotal Entries are Trucated to %d", MAX_HEAP_ENTRIES );
      MessageBox ( hwnd, szBuf, "Process Info", 0 );
    }
  }
  CloseHandle ( hSnap );
  if ( gnTHHeapEntryColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListHeapEntryCompareFunc );
}

VOID LoadAndFillTHHeapList ( HWND hwnd, BOOL bLoadColumns )
{
  HANDLE hSnap; 
  HEAPLIST32 hl32;
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process ID";
    lvc.cx = 65;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Heap ID";
    lvc.cx = 74;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Flags";
    lvc.cx = 281;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, TRUE );

  LPSTR pPtr;
  GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
  if ( lstrlen ( szBuf ) < 10 )
    return;

  pPtr = szBuf + 4;
  hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPHEAPLIST, atoi(pPtr) ); 
  ZeroMemory ( &hl32, sizeof(hl32) );

  if ( hSnap == INVALID_HANDLE_VALUE ) 
    return;

  hl32.dwSize = sizeof(hl32);
  if ( pHeap32ListFirst ( hSnap, &hl32 ) )
  {
    HEAPLIST32* phl;
    LV_ITEM lvi;
    ZeroMemory ( &lvi, sizeof(lvi) );
    INT iItem;
    gnListCount = 0;

    do
    {
      phl = new HEAPLIST32;
      memcpy ( phl, &hl32, sizeof(HEAPLIST32) );
      wsprintf ( szBuf, "%u", phl->th32ProcessID );
      lvi.iSubItem = 0;
      lvi.lParam = (LPARAM)phl;
      lvi.mask = LVIF_TEXT | LVIF_PARAM;
      lvi.pszText = szBuf;
      iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );
      lvi.mask = LVIF_TEXT;
      lvi.lParam = 0;
      lvi.iItem = iItem;
      lvi.iSubItem = 1;
      wsprintf ( szBuf, "%u", phl->th32HeapID );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      lvi.iSubItem = 2;
      GetStringHeapListFlagFromDWORD ( phl->dwFlags, szBuf );
      lvi.pszText = szBuf;
      SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
      gnListCount++;
    } while ( pHeap32ListNext ( hSnap, &hl32 ) );
  }
  CloseHandle (hSnap);
  if ( gnTHHeapListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListHeapCompareFunc );
}

VOID LoadAndFillTHModuleList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process ID";
    lvc.cx = 43;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Module ID";
    lvc.cx = 28;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Global Usage Count";
    lvc.cx = 30;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Local Usage Count";
    lvc.cx = 30;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Base Address";
    lvc.cx = 66;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Module Size";
    lvc.cx = 54;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
    lvc.pszText = "Handle";
    lvc.cx = 80;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 6, (LPARAM)&lvc );
    lvc.pszText = "Module Name";
    lvc.cx = 104;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 7, (LPARAM)&lvc );
    lvc.pszText = "Module Path";
    lvc.cx = 250;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 8, (LPARAM)&lvc );
    lvc.pszText = "Process Name";
    lvc.cx = 250;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 9, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  LPSTR pPtr;
  DWORD dwPID = 0;
  GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
  if ( lstrlen ( szBuf ) > 10 )
  {
    pPtr = szBuf + 4;
    dwPID = atoi ( pPtr );
  }

  INT nSize, i;
  nSize = gvTHModuleList.size ();
  PMODULEENTRY32 pme;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  gnListCount = 0;
	gnModuleSize = 0;

  for ( i = 0; i < nSize; i++ )
  {
    pme = gvTHModuleList[i];
    if ( dwPID && dwPID != pme->th32ProcessID )
      continue;
    wsprintf ( szBuf, "%u", pme->th32ProcessID );
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)pme;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = szBuf;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );
    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    wsprintf ( szBuf, "%u", pme->th32ModuleID );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 2;
    wsprintf ( szBuf, "%u", pme->GlblcntUsage );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 3;
    wsprintf ( szBuf, "%u", pme->ProccntUsage );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 4;
    wsprintf ( szBuf, "%08X", pme->modBaseAddr );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
		gnModuleSize += pme->modBaseSize;
    lvi.iSubItem = 5;
    wsprintf ( szBuf, "%u", pme->modBaseSize );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 6;
    wsprintf ( szBuf, "%u", pme->hModule );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 7;
    wsprintf ( szBuf, "%s", pme->szModule ? pme->szModule : "" );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 8;
    wsprintf ( szBuf, "%s", pme->szExePath ? pme->szExePath : "" );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 9;
    GetTHProcessNameFromProcessID ( pme->th32ProcessID, szBuf );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnTHModuleListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListModuleCompareFunc );
}

VOID LoadAndFillTHProcessList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];
  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Image Name";
    lvc.cx = 97;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Process ID";
    lvc.cx = 36;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Module ID";
    lvc.cx = 18;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Thread Count";
    lvc.cx = 24;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Thread Priority";
    lvc.cx = 24;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Ref. Count";
    lvc.cx = 18;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
    lvc.pszText = "Parent PID";
    lvc.cx = 36;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 6, (LPARAM)&lvc );
    lvc.pszText = "Def. Heap ID";
    lvc.cx = 18;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 7, (LPARAM)&lvc );
    lvc.pszText = "Flags";
    lvc.cx = 71;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 8, (LPARAM)&lvc );
    lvc.pszText = "Image Full Path";
    lvc.cx = 100;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 9, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );
  
  INT nSize = gvTHProcessList.size ();
  if ( pCreateToolhelp32Snapshot == NULL || nSize == 0)
    return;

  LPSTR pPtr;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  PROCESSENTRY32* pe;
  HIMAGELIST hilEDW = ListView_GetImageList ( hwnd, LVSIL_SMALL );
  gnListCount = 0;
  for ( int i = 0; i < nSize; i++ )
  {
    pe = gvTHProcessList[i];
    pPtr = strrchr ( pe->szExeFile, '\\' );
    HICON hIconInfo = ExtractIcon ( ghInstance, pe->szExeFile, 0 );
    if ( hIconInfo )
      ImageList_AddIcon( hilEDW, (HICON)hIconInfo );
    if ( pPtr )
      pPtr++;
    else
      pPtr = pe->szExeFile;
    lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    if ( hIconInfo )
      lvi.iImage = ImageList_GetImageCount ( hilEDW ) - 1;
    else
      lvi.iImage = 0;
    lvi.pszText = pPtr;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)pe;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );

    lvi.mask = LVIF_TEXT;
    lvi.pszText = pe->szExeFile;
    lvi.lParam = 0;
    lvi.iSubItem = 9;
    lvi.iItem = iItem;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.pszText = szBuf;

    wsprintf ( szBuf, "%u", pe->th32ProcessID );
    lvi.iSubItem = 1;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%u", pe->th32ModuleID );
    lvi.iSubItem = 2;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%u", pe->cntThreads );
    lvi.iSubItem = 3;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%d", pe->pcPriClassBase );
    lvi.iSubItem = 4;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%u", pe->cntUsage );
    lvi.iSubItem = 5;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%u", pe->th32ParentProcessID );
    lvi.iSubItem = 6;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "%u", pe->th32DefaultHeapID );
    lvi.iSubItem = 7;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );

    wsprintf ( szBuf, "0x%08X", pe->dwFlags );
    lvi.iSubItem = 8;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  } 
  if ( gnTHProcessListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListProcessCompareFunc );
}

VOID LoadAndFillTHThreadList ( HWND hwnd, BOOL bLoadColumns )
{
  LV_COLUMN lvc;
  char szBuf[512];

  ZeroMemory ( &lvc, sizeof(lvc) );
  lvc.mask = LVCF_TEXT;
  lvc.pszText = szBuf;
  lvc.cchTextMax = sizeof(szBuf);
  *szBuf = NULL;
  bLoadColumns = TRUE;

  if ( ListView_GetColumn ( hwnd, 0, &lvc ) && *szBuf )
    bLoadColumns = FALSE;

  ZeroMemory ( &lvc, sizeof(lvc) );

  if ( bLoadColumns )
  {
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = "Process ID";
    lvc.cx = 50;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
    lvc.pszText = "Thread ID";
    lvc.cx = 40;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );
    lvc.pszText = "Usage Count";
    lvc.cx = 30;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&lvc );
    lvc.pszText = "Base Priority";
    lvc.cx = 250;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 3, (LPARAM)&lvc );
    lvc.pszText = "Delta Priority";
    lvc.cx = 40;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 4, (LPARAM)&lvc );
    lvc.pszText = "Process Name";
    lvc.cx = 250;
    SendMessage ( hwnd, LVM_INSERTCOLUMN, 5, (LPARAM)&lvc );
  }
  else
    DeleteListViewItems ( hwnd, FALSE );

  LPSTR pPtr;
  DWORD dwPID = 0;
  GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
  if ( lstrlen ( szBuf ) > 10 )
  {
    pPtr = szBuf + 4;
    dwPID = atoi ( pPtr );
  }

  INT nSize = gvTHThreadList.size ();
  if (pCreateToolhelp32Snapshot == NULL || nSize == 0)
    return;

  PTHREADENTRY32 pte;
  LV_ITEM lvi;
  ZeroMemory ( &lvi, sizeof(lvi) );
  INT iItem;
  gnListCount = 0;
  for ( int i = 0; i < nSize; i++ )
  {
    pte = gvTHThreadList[i];
    if ( dwPID && dwPID != pte->th32OwnerProcessID )
      continue;
    wsprintf ( szBuf, "%u", pte->th32OwnerProcessID );
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)pte;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = szBuf;
    iItem = (INT)SendMessage ( hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi );
    lvi.mask = LVIF_TEXT;
    lvi.lParam = 0;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    wsprintf ( szBuf, "%u", pte->th32ThreadID );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 2;
    wsprintf ( szBuf, "%u", pte->cntUsage );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 3;
    GetStringThreadBasePriorityFromDWORD ( pte->tpBasePri, szBuf );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 4;
    wsprintf ( szBuf, "%u", pte->tpDeltaPri );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    lvi.iSubItem = 5;
    GetTHProcessNameFromProcessID ( pte->th32OwnerProcessID, szBuf );
    lvi.pszText = szBuf;
    SendMessage ( hwnd, LVM_SETITEM, 0, (LPARAM)&lvi );
    gnListCount++;
  }
  if ( gnTHThreadListColumn != -1 )
    SendMessage ( hwnd, LVM_SORTITEMS, 0, (LPARAM)THAPIListThreadCompareFunc );
}

VOID LoadChildWindowsInTree ( HWND hWndTree, HWND hwnd, HTREEITEM hParent )
{
  if ( !hwnd )
    return;

  HTREEITEM hNewItem = InsertItemInTree ( hWndTree, hwnd, hParent );

  HWND hNewWnd = GetWindow( hwnd, GW_CHILD );
  if ( hNewWnd )
    LoadChildWindowsInTree ( hWndTree, hNewWnd, hNewItem );
  hNewWnd = GetWindow ( hwnd, GW_HWNDNEXT );
  if ( hNewWnd )
    LoadChildWindowsInTree ( hWndTree, hNewWnd, hParent );
}

VOID LoadGlobalEDWAppList ()
{
  HWND hwndDesktop = GetDesktopWindow ();
  HWND hWnd = GetWindow ( hwndDesktop, GW_CHILD );
  PAPPLIST pal;
  HANDLE hProcess;
  while ( hWnd )
  {
    if ( IsWindowVisible ( hWnd ) )
    {
      pal = new APPLIST;
      pal->hwnd = hWnd;
      GetWindowText ( hWnd, pal->szAppName, MAX_PATH*2 );
      GetClassName ( hWnd, pal->szClassName, MAX_PATH );
      if ( !*pal->szAppName && !lstrcmpi ( pal->szClassName, "Shell_TrayWnd" ) )
        lstrcpy ( pal->szAppName, "Shell Tray" );
      /*DWORD dwThreadId = */GetWindowThreadProcessId ( hWnd, &pal->dwPID );
      hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, pal->dwPID );
      if (hProcess)
      {
        pal->dwPriority = GetPriorityClass ( hProcess );
        CloseHandle ( hProcess );
      }
      else
        pal->dwPriority = NORMAL_PRIORITY_CLASS;
      gvAppList.push_back ( pal );
    }
    hWnd = GetWindow ( hWnd, GW_HWNDNEXT );
  }
  if ( ghEDWWindow )
    PostMessage ( ghEDWWindow, WM_LOADDATA, 0, 0 );
}

VOID LoadGlobalProcessTimesVector ()
{
  INT i, nSize;
  nSize = gvTHProcessList.size ();
  if ( nSize )
  {
    PPROCESSENTRY32 ppe;
    for ( i = 0; i < nSize; i++ )
    {
      ppe = gvTHProcessList[i];
      LoadGlobalProcessTimesVector ( ppe->th32ProcessID, ppe->szExeFile );
    }
    goto lblDisplayProcessTimes;
  }
  nSize = gvPSProcessList.size ();
  if ( nSize )
  {
    PPSAPIPROCESSLIST ppspl;
    for ( i = 0; i < nSize; i++ )
    {
      ppspl = gvPSProcessList[i];
      LoadGlobalProcessTimesVector ( ppspl->dwPID, ppspl->szProcessName );
    }
    goto lblDisplayProcessTimes;
  }
  nSize = gvAppList.size ();
  if ( nSize )
  {
    PAPPLIST pal;
    for ( i = 0; i < nSize; i++ )
    {
      pal = gvAppList[i];
      LoadGlobalProcessTimesVector ( pal->dwPID, pal->szAppName );
    }
    goto lblDisplayProcessTimes;
  }
  return;
lblDisplayProcessTimes:
  if ( ghProcessTimesWindow )
    PostMessage ( ghProcessTimesWindow, WM_LOADDATA, 0, 0 );
}

VOID LoadGlobalProcessTimesVector ( DWORD dwPID, LPSTR szProcessName )
{
  PPROCESSTIME ppt;
  FILETIME ftExit;
  ppt = new PROCESSTIME;
  ZeroMemory ( ppt, sizeof(PROCESSTIME) );
  ppt->dwPID = dwPID;
  *ppt->szProcessName = NULL;
  if ( szProcessName && *szProcessName )
    lstrcpy ( ppt->szProcessName, szProcessName );
  DWORD dwVer = GetProcessVersion ( dwPID );
  ppt->wMinrorVersion = LOWORD(dwVer);
  ppt->wMajorVersion = HIWORD(dwVer);
  HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, dwPID );
  if ( hProcess )
  {
    GetProcessTimes ( hProcess, &ppt->ftCreation, &ftExit, &ppt->ftKernelTime, &ppt->ftUserTime );
    CloseHandle ( hProcess );
  }
  gvProcessTimes.push_back ( ppt );
}

VOID LoadGlobalPSModuleVector ( DWORD dwPID, LPSTR szProcessName )
{
  if ( !pGetModuleFileNameEx || !pEnumProcessModules )
    return;

  HMODULE hModules[1024];
  HANDLE hProcess;
  DWORD cbNeeded;
  UINT i;
  PPSAPIMODULELIST ppsml;
  CHAR szModuleName[MAX_PATH];

  hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID );
  if (NULL == hProcess)
    return;

  if( pEnumProcessModules ( hProcess, hModules, sizeof(hModules), &cbNeeded ) )
  {
    cbNeeded /= sizeof(HMODULE);
    for ( i = 0; i < cbNeeded; i++ )
    {
      ppsml = new PSAPIMODULELIST;
      ZeroMemory ( ppsml, sizeof(PSAPIMODULELIST) );
      ppsml->dwPID = dwPID;
      ppsml->hModule = hModules[i];
      lstrcpy ( ppsml->szProcessName, szProcessName );
      if ( pGetModuleFileNameEx ( hProcess, hModules[i], szModuleName, sizeof(szModuleName) ) )
        lstrcpy ( ppsml->szModuleName, szModuleName );
      else
        lstrcpy ( ppsml->szModuleName, "<Unknown>" );

      if (pGetModuleInformation != NULL)
        pGetModuleInformation ( hProcess, hModules[i], &ppsml->mi, sizeof(MODULEINFO) );

      gvPSModuleList.push_back ( ppsml );
    }
  }
  CloseHandle( hProcess );
}

VOID LoadGlobalPSVectors ()
{
  if ( !pEnumProcesses || !pEnumProcessModules )
    return;

  DWORD dwPIDs[MAX_PATH], cbNeeded, nProcesses;
  PPSAPIPROCESSLIST pspl;
  HANDLE hProcess;
  HMODULE hMod;
  char szBuf[512];
  DWORD dwPID = 0;
  BOOL bResetSelection = FALSE;

  *szBuf = NULL;
  if ( ghProcessInfoWindow )
  {
    GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
    if ( lstrlen ( szBuf ) > 10 )
      dwPID = atoi ( szBuf + 4 );
  }

  if ( !pEnumProcesses( dwPIDs, sizeof(dwPIDs), &cbNeeded ) )
    return;

  nProcesses = cbNeeded / sizeof(DWORD);
  
  for ( DWORD i = 0; i < nProcesses; i++ )
  {
    pspl = new PSAPIPROCESSLIST;
    pspl->dwPID = dwPIDs[i];
    if ( pspl->dwPID == dwPID )
      bResetSelection = TRUE;
    *pspl->szProcessName = NULL;
    gvPSProcessList.push_back ( pspl );
    pspl->dwPriority = 0;

    ZeroMemory ( &pspl->pmc, sizeof(PROCESS_MEMORY_COUNTERS) );
    pspl->pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
    hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pspl->dwPID );
    if ( hProcess )
    {
      pspl->dwPriority = GetPriorityClass ( hProcess );
      if ( pEnumProcessModules ( hProcess, &hMod, sizeof(hMod), &cbNeeded ) )
		pGetModuleFileNameEx ( hProcess, hMod, pspl->szProcessName, MAX_PATH );
        //pGetModuleBaseName ( hProcess, hMod, pspl->szProcessName, MAX_PATH );
		
      pGetProcessMemoryInfo ( hProcess, &pspl->pmc, sizeof(PROCESS_MEMORY_COUNTERS) );
      CloseHandle ( hProcess );
    }
    if ( !*pspl->szProcessName )
    {
      if ( !pspl->dwPID )
        lstrcpy ( pspl->szProcessName, "[System Process]" );
      else if ( pspl->dwPID <= 10 )
        lstrcpy ( pspl->szProcessName, "System" );
      else
        lstrcpy ( pspl->szProcessName, "<System Process/Service>" );
    }
    LoadGlobalPSModuleVector ( pspl->dwPID, pspl->szProcessName );
  }

  if (EnumDeviceDrivers != NULL && pGetDeviceDriverBaseName != NULL && pGetDeviceDriverFileName != NULL)
  {
    PPSAPIDEIVERLIST ppsdl;
    DWORD aDrivers[1024];
    DWORD cbNeeded;

    if ( !EnumDeviceDrivers ( (LPVOID*)aDrivers, sizeof(aDrivers), &cbNeeded ) )
      goto lblExitEnumDrivers;

    DWORD cDrivers = cbNeeded / sizeof(aDrivers[0]);

    for ( unsigned i = 0; i < cDrivers; i++ )
    {
      ppsdl = new PSAPIDEIVERLIST;
      ZeroMemory ( ppsdl, sizeof(PSAPIDEIVERLIST) );
      ppsdl->dwhDriver = aDrivers[i];
      if ( pGetDeviceDriverBaseName ( (LPVOID)aDrivers[i], ppsdl->szDriverName, MAX_PATH ) )
      {
        if ( !pGetDeviceDriverFileName ( (LPVOID)aDrivers[i], ppsdl->szDriverFileName, MAX_PATH ) )
          lstrcpy ( ppsdl->szDriverFileName, "<Unknown>" );
      }
      else
      {
        lstrcpy ( ppsdl->szDriverName, "<Unknown>" );
        lstrcpy ( ppsdl->szDriverFileName, "<Unknown>" );
      }
      gvPSDriverList.push_back ( ppsdl );
    }
  }
lblExitEnumDrivers:
  if ( ghProcessInfoWindow )
  {
    if ( bResetSelection )
      SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
    else
      SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
  }

  if ( ghPSProcessWindow )
    PostMessage ( ghPSProcessWindow, WM_LOADDATA, 0, 0 );
  if ( ghPSModulesWindow )
    PostMessage ( ghPSModulesWindow, WM_LOADDATA, 0, 0 );
  if ( ghPSMemoryUsageWindow )
    PostMessage ( ghPSMemoryUsageWindow, WM_LOADDATA, 0, 0 );
  if ( ghPSDriversWindow )
    PostMessage ( ghPSDriversWindow, WM_LOADDATA, 0, 0 );
}

VOID LoadGlobalPSTHVectors ( BOOL bManualRefresh )
{
  UnLoadGlobalPSTHVectors ();
  LoadGlobalEDWAppList ();
  LoadGlobalPSVectors ();
  LoadGlobalTHVectors ();
  LoadGlobalProcessTimesVector ();

  if ( ghAllWindowWindow )
    PostMessage ( ghAllWindowWindow, WM_LOADDATA, 0, 0 );

  if ( !bManualRefresh )
    return;
}

VOID LoadGlobalTHModuleVector ( DWORD dwPID )
{
  if (pCreateToolhelp32Snapshot == NULL)
    return;

  PMODULEENTRY32 pme;
  MODULEENTRY32 me32;
  HANDLE hSnap;
  
  hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPMODULE, dwPID );
  if ( hSnap == INVALID_HANDLE_VALUE ) 
    return;

  me32.dwSize = sizeof(me32);
  if ( pModule32First ( hSnap, &me32 ) )
  {
    do
    {
      pme = new MODULEENTRY32;
      memcpy ( pme, &me32, sizeof(MODULEENTRY32) );
      gvTHModuleList.push_back ( pme );
    } while ( pModule32Next ( hSnap, &me32 ) );
  }
  CloseHandle (hSnap);
}

VOID LoadGlobalTHVectors ()
{
  if (pCreateToolhelp32Snapshot == NULL)
    return;

  HANDLE hSnap; 
  PROCESSENTRY32 pe32;
  PROCESSENTRY32* pe;
  BOOL bResetSelection = FALSE;
  char szBuf[512];
  DWORD dwPID = 0;

  *szBuf = NULL;
  if ( ghProcessInfoWindow )
  {
    GetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf, sizeof(szBuf) );
    if ( lstrlen ( szBuf ) > 10 )
      dwPID = atoi ( szBuf + 4 );
  }

  //if (true) return; //kannan-1
  hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS, 0 );
  if ( hSnap == INVALID_HANDLE_VALUE ) 
    goto LoadThread;

  ZeroMemory ( &pe32, sizeof(pe32) );
  pe32.dwSize = sizeof(PROCESSENTRY32);
  if ( pProcess32First ( hSnap, &pe32 ) ) 
  {
    do 
    {
      pe = new PROCESSENTRY32;
      memcpy ( pe, &pe32, sizeof(PROCESSENTRY32) );
      if ( pe32.th32ProcessID == dwPID )
        bResetSelection = TRUE;
      gvTHProcessList.push_back ( pe );
      LoadGlobalTHModuleVector ( pe32.th32ProcessID );
    } 
    while ( pProcess32Next ( hSnap, &pe32 ) ); 
  }
  CloseHandle (hSnap);

LoadThread:
  THREADENTRY32 te32;
  PTHREADENTRY32 pte;

  hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPTHREAD, 0 );
  if ( hSnap == INVALID_HANDLE_VALUE ) 
    return;

  ZeroMemory ( &te32, sizeof(te32) );
  te32.dwSize = sizeof(te32);

  if ( pThread32First ( hSnap, &te32 ) )
  {
    do
    {
      pte = new THREADENTRY32;
      memcpy ( pte, &te32, sizeof(THREADENTRY32) );
      gvTHThreadList.push_back ( pte );
    } while ( pThread32Next ( hSnap, &te32 ) );
  }
  CloseHandle (hSnap);
  
  if ( ghProcessInfoWindow )
  {
    if ( bResetSelection )
      SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), szBuf );
    else
      SetWindowText ( GetDlgItem ( ghProcessInfoWindow, IDC_TXTSELPROCESS ), "" );
  }
  if ( ghTHProcessWindow )
    PostMessage ( ghTHProcessWindow, WM_LOADDATA, 0, 0 );
  if ( ghTHModuleWindow )
    PostMessage ( ghTHModuleWindow, WM_LOADDATA, 0, 0 );
  if ( ghTHHeapWindow )
    PostMessage ( ghTHHeapWindow, WM_LOADDATA, 0, 0 );
  if ( ghTHThreadWindow )
    PostMessage ( ghTHThreadWindow, WM_LOADDATA, 0, 0 );
}

VOID UnLoadGlobalEDWAppList ()
{
  if ( ghEDWWindow )
  {
    HWND hList = GetDlgItem ( ghEDWWindow, IDC_1LISTAPP );
    DeleteListViewItems ( hList, FALSE );
  }
  PAPPLIST pal;
  INT nSize, i;
  nSize = gvAppList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    pal = gvAppList[i];
    delete pal;
  }
  gvAppList.erase ( gvAppList.begin (), gvAppList.end () );
}

VOID UnLoadGlobalProcessTimesVector ()
{
  PPROCESSTIME ppt;
  INT nSize, i;

  if ( ghProcessTimesWindow )
  {
    HWND hList = GetDlgItem ( ghProcessTimesWindow, IDC_10LISTPROCESS );
    DeleteListViewItems ( hList, FALSE );
  }

  nSize = gvProcessTimes.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppt = gvProcessTimes[i];
    delete ppt;
  }
  gvProcessTimes.erase ( gvProcessTimes.begin (), gvProcessTimes.end () );
}

VOID UnLoadGlobalPSVectors ()
{
  if ( !pEnumProcesses || !pEnumProcessModules )
    return;

  PPSAPIPROCESSLIST ppspl;
  PPSAPIMODULELIST ppsml;
  PPSAPIDEIVERLIST ppsdl;
  int nSize, i;

  if ( ghPSDriversWindow )
  {
    HWND hList = GetDlgItem ( ghPSDriversWindow, IDC_11LISTDRIVERS );
    DeleteListViewItems ( hList, FALSE );
  }

  if ( ghPSModulesWindow )
  {
    HWND hList = GetDlgItem ( ghPSModulesWindow, IDC_8LISTMODULE );
    DeleteListViewItems ( hList, FALSE );
  }

  if ( ghPSProcessWindow )
  {
    HWND hList = GetDlgItem ( ghPSProcessWindow, IDC_7LISTPROCESS );
    DeleteListViewItems ( hList, FALSE );
  }

  if ( ghPSMemoryUsageWindow )
  {
    HWND hList = GetDlgItem ( ghPSMemoryUsageWindow, IDC_9LISTPROCESS );
    DeleteListViewItems ( hList, FALSE );
  }

  nSize = gvPSProcessList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppspl = gvPSProcessList[i];
    delete ppspl;
  }
  gvPSProcessList.erase ( gvPSProcessList.begin (), gvPSProcessList.end () );

  nSize = gvPSModuleList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppsml = gvPSModuleList[i];
    delete ppsml;
  }
  gvPSModuleList.erase ( gvPSModuleList.begin (), gvPSModuleList.end () );

  nSize = gvPSDriverList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppsdl = gvPSDriverList[i];
    delete ppsdl;
  }
  gvPSDriverList.erase ( gvPSDriverList.begin (), gvPSDriverList.end () );
}

VOID UnLoadGlobalPSTHVectors ()
{
  UnLoadGlobalEDWAppList ();
  UnLoadGlobalPSVectors ();
  UnLoadGlobalTHVectors ();
  UnLoadGlobalProcessTimesVector ();
}

VOID UnLoadGlobalTHVectors ()
{
  if (pCreateToolhelp32Snapshot == NULL)
    return;
  int nSize, i;
  PPROCESSENTRY32 ppe;
  PTHREADENTRY32 pte;
  PMODULEENTRY32 pme;

  HWND hList;
  if ( ghTHProcessWindow )
  {
    hList = GetDlgItem ( ghTHProcessWindow, IDC_2LISTPROCESS );
    DeleteListViewItems ( hList, FALSE );
  }
  if ( ghTHModuleWindow )
  {
    hList = GetDlgItem ( ghTHModuleWindow, IDC_5LISTMODULE );
    DeleteListViewItems ( hList, FALSE );
  }
  if ( ghTHHeapWindow )
  {
    hList = GetDlgItem ( ghTHHeapWindow, IDC_4LISTHEAPS );
    DeleteListViewItems ( hList, TRUE );
    hList = GetDlgItem ( ghTHHeapWindow, IDC_4LISTHEAPENTRY );
    DeleteListViewItems ( hList, TRUE );
  }
  if ( ghTHThreadWindow )
  {
    hList = GetDlgItem ( ghTHThreadWindow, IDC_6LISTTHREAD );
    DeleteListViewItems ( hList, FALSE );
  }

  nSize = gvTHProcessList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    ppe = gvTHProcessList[i];
    delete ppe;
  }
  gvTHProcessList.erase ( gvTHProcessList.begin (), gvTHProcessList.end () );

  nSize = gvTHThreadList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    pte = gvTHThreadList[i];
    delete pte;
  }
  gvTHThreadList.erase ( gvTHThreadList.begin (), gvTHThreadList.end () );

  nSize = gvTHModuleList.size ();
  for ( i = 0; i < nSize; i++ )
  {
    pme = gvTHModuleList[i];
    delete pme;
  }
  gvTHModuleList.erase ( gvTHModuleList.begin (), gvTHModuleList.end () );
}


//Pipe Related Functions

DWORD  WINAPI PipeThreadFunc ( LPVOID )
{
  DWORD dwAvailableBytes;
  CHAR szBuffer[1024];
  DWORD dwBytesRead;

  ConnectNamedPipe ( ghPipe, NULL );
  while ( 1 )
  {
    dwAvailableBytes = 0;
    PeekNamedPipe ( ghPipe, NULL, 0, NULL, &dwAvailableBytes, NULL );
    if ( gnAppExit ) return 0;
    if ( !dwAvailableBytes )
    {
      Sleep ( 1000 );
      continue;
    }
    do
    {
      if ( dwAvailableBytes > 1020 )
        dwAvailableBytes = 1020;
      ReadFile ( ghPipe, szBuffer, dwAvailableBytes, &dwBytesRead, NULL );
      *(szBuffer+dwBytesRead) = NULL;
      CheckAndDisplay ( szBuffer, dwBytesRead );
    }
    while ( PeekNamedPipe ( ghPipe, NULL, 0, NULL, &dwAvailableBytes, NULL ) && dwAvailableBytes );
  }
}

//TCP/IP Related Functions

DWORD WINAPI TCPIPWinSockThreadFunc ( LPVOID )
{
  SOCKET sock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  WSAEventSelect ( sock, ghWinSockUDPEvent, FD_READ | FD_ACCEPT );

  //Pending Handle TCP/IP Protocol

  return 0;
}

//UDP Related Functions

DWORD WINAPI UDPWinSockThreadFunc ( LPVOID )
{
  CHAR szBuf[2048];
  DWORD dwRet;
  WSANETWORKEVENTS wsane;
  ResetUDPServer ();
  while ( 1 )
  {
    dwRet = WSAWaitForMultipleEvents ( 1, &ghWinSockUDPEvent, FALSE, 500, FALSE );
    if ( dwRet == WSA_WAIT_TIMEOUT )
    {
      if ( gnAppExit )
        break;
      if ( gnResetUDPServer )
      {
        gnResetUDPServer = FALSE;
        ResetUDPServer ();
      }
    }
    else if ( dwRet == WSA_WAIT_EVENT_0 )
    {
      WSAEnumNetworkEvents ( gsockUDP, ghWinSockUDPEvent, &wsane );
      if ( wsane.lNetworkEvents & FD_READ )
      {
        WSABUF wsab;
        wsab.buf = szBuf;
        wsab.len = sizeof(szBuf) - 5;
        DWORD dwBytesReceived = 0;
        DWORD dwFlags = 0;
        while ( 1 )
        {
          WSARecv ( gsockUDP, &wsab, 1, &dwBytesReceived, &dwFlags, NULL, NULL );
          if ( dwBytesReceived )
          {
            *(szBuf+dwBytesReceived) = NULL;
            CheckAndDisplay ( szBuf, dwBytesReceived );
          }
          else
            break;
          if ( dwFlags & MSG_PARTIAL )
          {
            dwFlags = 0;
            continue;
          }
          break;
        }
      }
    }
  }
  return 0;
}

VOID ResetUDPServer ()
{
  if ( gsockUDP )
    closesocket ( gsockUDP );
  gsockUDP = WSASocket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0 );
  if ( WSAEventSelect ( gsockUDP, ghWinSockUDPEvent, FD_READ ) )
    WSAPrintError ( "WSAEventSelect" );
  SOCKADDR_IN sa;
  sa.sin_port = htons ( (short)gbServerUDPInputPort );
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  if ( bind ( gsockUDP, (LPSOCKADDR)&sa, sizeof(struct sockaddr) ) )
    WSAPrintError ( "bind" );
}

//Debugger Related Functions

DWORD WINAPI DebuggerThreadFunc ( LPVOID pVoid )
{
  SECURITY_ATTRIBUTES sa;
  STARTUPINFO sinfo;
  PROCESS_INFORMATION pinfo;
  DEBUG_EVENT de;
  char szProcessName[MAX_PATH];
  LPSTR pFile;
  LPSTR pParam;
  HWND hwnd, hEdit;
  vector <PMODULEENTRY32> zvTHModules;
  vector <PPSAPIMODULELIST> zvPSModules;
  BOOL bFirstProcess = TRUE;
  char szBuf[1024];

  pFile = (LPSTR)pVoid;
  zvTHModules.reserve ( 100 );
  zvPSModules.reserve ( 100 );
  pParam = (LPSTR)*(int*)(pFile+lstrlen(pFile)+1);
  if ( pParam )
    hwnd = (HWND)*(int*)(pParam+lstrlen(pParam)+1);
  else
    hwnd = (HWND)*(int*)(pFile+lstrlen(pFile)+5);
  ZeroMemory ( &sa, sizeof(sa) );
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  ZeroMemory ( &sinfo, sizeof(sinfo) );
  ZeroMemory ( &pinfo, sizeof(pinfo) );
  sinfo.cb = sizeof(sinfo);
  hEdit = GetWindow ( hwnd, GW_CHILD );
  lstrcpy ( szProcessName, pFile );
  //MessageBox(NULL, pFile, "Process", 0); //kannan-1
  MessageBox(NULL, pParam, "pParam", 0); //kannan-1
  if  ( !CreateProcess ( pFile, pParam, &sa, &sa, FALSE, DEBUG_PROCESS | NORMAL_PRIORITY_CLASS, NULL, NULL,
                         &sinfo, &pinfo ) )
  {
    wsprintf ( szBuf, "Unable to start %s %s\r\n", pFile, pParam ? pParam : "" );
    delete [] pFile;
    if ( pParam )
      delete [] pParam;

    CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
    DbgDeleteModuleVector ( zvTHModules, zvPSModules );
    return 0;
  }

  while ( 1 )
  {
    if ( gnAppExit || !IsWindow ( hwnd ) )
    {
      TerminateProcess ( pinfo.hProcess, 0 );
      CloseHandle ( pinfo.hProcess );
      CloseHandle ( pinfo.hThread );
      delete [] pFile;
      if ( pParam )
        delete [] pParam;
      DbgDeleteModuleVector ( zvTHModules, zvPSModules );
      return 0;
    }
    ZeroMemory ( &de, sizeof(de) );

    if ( !WaitForDebugEvent ( &de, 500 ) )
      continue;

    switch ( de.dwDebugEventCode )
    {
      case EXCEPTION_DEBUG_EVENT:
      {
        EXCEPTION_RECORD* er;
        INT nExceptionCount = 1;

        er = &de.u.Exception.ExceptionRecord;
        char szStr[2*1024];
        LPSTR ptr = szStr;
        wsprintf ( ptr, "Exception Raised %s\r\n", de.u.Exception.dwFirstChance ? "For the First Time" : "Again" );
        ptr += lstrlen ( ptr );
        while ( er )
        {
          switch ( er->ExceptionCode )
          {
            case EXCEPTION_ACCESS_VIOLATION:
              wsprintf ( ptr, "  [%d]\tThe thread tried to read from or write to a virtual address for which it does not have the appropriate access.", nExceptionCount );
              break;

            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
              wsprintf ( ptr, "  [%d]\tThe thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.", nExceptionCount );
              break;

            case EXCEPTION_BREAKPOINT:
              wsprintf ( ptr, "  [%d]\tA breakpoint was encountered.", nExceptionCount );
              break;

            case EXCEPTION_DATATYPE_MISALIGNMENT:
              wsprintf ( ptr, "  [%d]\tThe thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.", nExceptionCount );
              break;

            case EXCEPTION_FLT_DENORMAL_OPERAND:
              wsprintf ( ptr, "  [%d]\tOne of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.", nExceptionCount );
              break;

            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
              wsprintf ( ptr, "  [%d]\tThe thread tried to divide a floating-point value by a floating-point divisor of zero.", nExceptionCount );
              break;

            case EXCEPTION_FLT_INEXACT_RESULT:
              wsprintf ( ptr, "  [%d]\tThe result of a floating-point operation cannot be represented exactly as a decimal fraction.", nExceptionCount );
              break;

            case EXCEPTION_FLT_INVALID_OPERATION:
              wsprintf ( ptr, "  [%d]\tThis exception represents any floating-point exception not included in this list.", nExceptionCount );
              break;

            case EXCEPTION_FLT_OVERFLOW:
              wsprintf ( ptr, "  [%d]\tThe exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.", nExceptionCount );
              break;

            case EXCEPTION_FLT_STACK_CHECK:
              wsprintf ( ptr, "  [%d]\tThe stack overflowed or underflowed as the result of a floating-point operation.", nExceptionCount );
              break;

            case EXCEPTION_FLT_UNDERFLOW:
              wsprintf ( ptr, "  [%d]\tThe exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.", nExceptionCount );
              break;

            case EXCEPTION_ILLEGAL_INSTRUCTION:
              wsprintf ( ptr, "  [%d]\tThe thread tried to execute an invalid instruction.", nExceptionCount );
              break;

            case EXCEPTION_IN_PAGE_ERROR:
              wsprintf ( ptr, "  [%d]\tThe thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.", nExceptionCount );
              break;

            case EXCEPTION_INT_DIVIDE_BY_ZERO:
              wsprintf ( ptr, "  [%d]\tThe thread tried to divide an integer value by an integer divisor of zero.", nExceptionCount );
              break;

            case EXCEPTION_INT_OVERFLOW:
              wsprintf ( ptr, "  [%d]\tThe result of an integer operation caused a carry out of the most significant bit of the result.", nExceptionCount );
              break;

            case EXCEPTION_INVALID_DISPOSITION:
              wsprintf ( ptr, "  [%d]\tAn exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.", nExceptionCount );
              break;

            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
              wsprintf ( ptr, "  [%d]\tThe thread tried to continue execution after a noncontinuable exception occurred.", nExceptionCount );
              break;

            case EXCEPTION_PRIV_INSTRUCTION:
              wsprintf ( ptr, "  [%d]\tThe thread tried to execute an instruction whose operation is not allowed in the current machine mode.", nExceptionCount );
              break;

            case EXCEPTION_SINGLE_STEP:
              wsprintf ( ptr, "  [%d]\tA trace trap or other single-instruction mechanism signaled that one instruction has been executed.", nExceptionCount );
              break;

            case EXCEPTION_STACK_OVERFLOW:
              wsprintf ( ptr, "  [%d]\tThe thread used up its stack.", nExceptionCount );
              break;
          }
          ptr += lstrlen ( ptr );
          wsprintf ( ptr, "\r\n    \t%s", !er->ExceptionFlags ? "You can continue this application." : "You can not continue this application."  );
          ptr += lstrlen ( ptr );
          char szModule[MAX_PATH];
          if ( -1 == DbgFindModuleFromIP ( zvTHModules, zvPSModules, (DWORD)er->ExceptionAddress, szModule ) )
            lstrcpy ( szModule, "<Unknown Module>" );
          wsprintf ( ptr, "\r\n    \tThis Exception occured at 0x%08X in module %s", er->ExceptionAddress, szModule );
          ptr += lstrlen ( ptr );
          wsprintf ( ptr, "\r\n    \tThis Exception has %d Parameters\r\n", er->NumberParameters  );
          ptr += lstrlen ( ptr );
          nExceptionCount++;
          er = er->ExceptionRecord;
        }
        CheckAndDisplay ( szStr, lstrlen ( szStr ), hEdit );
        //if ( IDYES == MessageBox ( 0, "Do you Wish to Continue ?", "Debugger", MB_YESNO ) ) //kannan-3
          //ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        //else
          //ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED );

		ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE ); //kannan-1
        break;
      }

      case CREATE_THREAD_DEBUG_EVENT:
      {
        wsprintf ( szBuf, "Starting new Thread [Handle = 0x%08X], [Start Address = 0x%08X]\r\n", de.u.CreateThread.hThread,
                   de.u.CreateThread.lpStartAddress );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        break;
      }

      case CREATE_PROCESS_DEBUG_EVENT:
      {
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        Sleep ( 100 );
        char szFileName[MAX_PATH];
        DbgGetMoreModules ( zvTHModules, zvPSModules, de.dwProcessId );
        if ( bFirstProcess )
          lstrcpy ( szFileName, szProcessName );
        else if ( -1 == DbgFindModule ( zvTHModules, zvPSModules, (DWORD)de.u.CreateProcessInfo.lpBaseOfImage, szFileName ) )
        {
          if ( de.u.CreateProcessInfo.lpImageName && *(LPSTR)de.u.CreateProcessInfo.lpImageName )
            lstrcpy ( szFileName, (LPSTR)de.u.CreateProcessInfo.lpImageName );
          else
            lstrcpy ( szFileName, "<Unknown EXE>" );
        }

        wsprintf ( szBuf, "Starting new Process %s [%s], [%s], [Process Handle = 0x%08X], [Start Address = 0x%08X], [File Handle = 0x%08X]\r\n",
                   szFileName,
                   de.u.CreateProcessInfo.fUnicode ? "Unicode EXE" : "ANSI EXE",
                   de.u.CreateProcessInfo.nDebugInfoSize ? "Has Debug Info" : "Has No Debug Info",
                   de.u.CreateProcessInfo.hProcess, de.u.CreateProcessInfo.lpBaseOfImage, de.u.CreateProcessInfo.hFile );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        bFirstProcess = FALSE;
        break;
      }

      case EXIT_THREAD_DEBUG_EVENT:
      {
        wsprintf ( szBuf, "Exiting Thread ( Exit Code = %d [%X] )\r\n", pFile, de.u.ExitThread.dwExitCode,
                   de.u.ExitThread.dwExitCode );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        break;
      }

      case EXIT_PROCESS_DEBUG_EVENT:
      {
        wsprintf ( szBuf, "Exiting Process ( Exit Code = %d [%X] )\r\n", de.u.ExitProcess.dwExitCode,
                   de.u.ExitProcess.dwExitCode );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        DbgDeleteModuleVector ( zvTHModules, zvPSModules );
        break;
      }

      case LOAD_DLL_DEBUG_EVENT:
      {
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        char szFileName[MAX_PATH];
        for ( INT i = 0; i < 4; i++ )
        {
          DbgGetMoreModules ( zvTHModules, zvPSModules, de.dwProcessId );
          if ( -1 == DbgFindModule ( zvTHModules, zvPSModules, (DWORD)de.u.LoadDll.lpBaseOfDll, szFileName ) )
          {
            if ( de.u.LoadDll.lpImageName && *(LPSTR)de.u.LoadDll.lpImageName )
              lstrcpy ( szFileName, (LPSTR)de.u.LoadDll.lpImageName );
            else
            {
              if ( !GetFilenameFromHandle ( de.u.LoadDll.hFile, pinfo.hProcess, szFileName ) )
                lstrcpy ( szFileName, "<Unknown DLL>" );
            }
            Sleep ( 100 );
          }
          else
            break;
        }
        wsprintf ( szBuf, "Loading Dll %s [%s], [%s], [Loading at 0x%08X], [File Handle = 0x%08X]\r\n",
                   szFileName,
                   de.u.LoadDll.fUnicode ? "Unicode DLL" : "ANSI DLL",
                   de.u.LoadDll.nDebugInfoSize ? "Has Debug Info" : "Has No Debug Info",
                   de.u.LoadDll.lpBaseOfDll, de.u.LoadDll.hFile );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        break;
      }

      case UNLOAD_DLL_DEBUG_EVENT:
      {
        char szFileName[MAX_PATH];
        INT nIndex = DbgFindModule ( zvTHModules, zvPSModules, (DWORD)de.u.UnloadDll.lpBaseOfDll, szFileName );
        if ( -1 == nIndex )
          lstrcpy ( szFileName, "<Unknown Dll>" );
        else
          DbgRemoveModule ( zvTHModules, zvPSModules, nIndex );
        wsprintf ( szBuf, "Unloading Dll %s [From 0x%08X]\r\n",
                   szFileName, de.u.UnloadDll.lpBaseOfDll );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        break;
      }

      case OUTPUT_DEBUG_STRING_EVENT:
      {
        LPSTR ptr = new char[de.u.DebugString.nDebugStringLength+4];
        DWORD dwBytesRead = 0;
        if ( ReadProcessMemory ( pinfo.hProcess, (LPCVOID)de.u.DebugString.lpDebugStringData, (LPVOID)ptr, de.u.DebugString.nDebugStringLength, &dwBytesRead ) )
        {
          *(ptr+de.u.DebugString.nDebugStringLength) = NULL;
          *(ptr+de.u.DebugString.nDebugStringLength+1) = NULL;
          if ( de.u.DebugString.fUnicode )
          {
            //Unicode
            CheckAndDisplay ( "OutputDebugString: <Unicode String>", 35, hEdit );
            LPSTR pAnsiPtr = new char[de.u.DebugString.nDebugStringLength+2];
            INT nLen;
            nLen = WideCharToMultiByte ( CP_OEMCP, 0, (LPCWSTR)ptr, de.u.DebugString.nDebugStringLength+1,
                                         pAnsiPtr, de.u.DebugString.nDebugStringLength, NULL, NULL );
            *( pAnsiPtr + nLen ) = NULL;
            CheckAndDisplay ( pAnsiPtr, nLen, hEdit );
            delete [] pAnsiPtr;
            CheckAndDisplay ( "\r\n", 2, hEdit );
          }
          else
          {
            //ANSI
            CheckAndDisplay ( "OutputDebugString: ", 19, hEdit );
            CheckAndDisplay ( ptr, lstrlen ( ptr ), hEdit );
            CheckAndDisplay ( "\r\n", 2, hEdit );
          }
        }
        delete [] ptr;
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        break;
      }

      case RIP_EVENT:
      {
        wsprintf ( szBuf, "RIP (System Debugging) Event [Error Code 0x%08X] [Reason = %s]\r\n",
                   de.u.RipInfo.dwError, ( de.u.RipInfo.dwType == SLE_ERROR ) ?
                   "Invalid data was passed to the function that failed. This caused the application to fail." :
                   ( ( de.u.RipInfo.dwType == SLE_MINORERROR ) ?
                     "Invalid data was passed to the function, but the error probably will not cause the application to fail." :
                   ( ( de.u.RipInfo.dwType == SLE_WARNING ) ?
                     "Potentially invalid data was passed to the function, but the function completed processing." :
                     "Only Error Code was set" )) );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
        break;
      }

      default:
      {
        wsprintf ( szBuf, "Unknown Debugger Event ( Event Code = %d [%X] )\r\n", de.dwDebugEventCode, de.dwDebugEventCode );
        CheckAndDisplay ( szBuf, lstrlen ( szBuf ), hEdit );
        ContinueDebugEvent ( de.dwProcessId, de.dwThreadId, DBG_CONTINUE );
      }
    }
  }
}

INT DbgFindModule ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwAddress, LPSTR szBuf )
{
  INT nSize, i;
  switch ( gcProcessFinderMethod )
  {
    case 'T':
    {
      nSize = xvTHModules.size ();
      PMODULEENTRY32 pme;
      for ( i = 0; i < nSize; i++ )
      {
        pme = xvTHModules[i];
        if ( !pme )
          continue;
        if ( (DWORD)pme->modBaseAddr == xdwAddress )
        {
          if ( szBuf )
            lstrcpy ( szBuf, pme->szExePath );
          return i;
        }
      }
      break;
    }

    case 'P':
    {
      nSize = xvPSModules.size ();
      PPSAPIMODULELIST ppsml;
      for ( i = 0; i < nSize; i++ )
      {
        ppsml = xvPSModules[i];
        if ( !ppsml )
          continue;
        if ( xdwAddress == (DWORD)ppsml->mi.lpBaseOfDll )
        {
          if ( szBuf )
            lstrcpy ( szBuf, ppsml->szModuleName );
          return i;
        }
      }
      break;
    }
  }
  if ( szBuf )
    *szBuf = NULL;
  return -1;
}

INT DbgFindModuleFromIP ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwAddress, LPSTR szBuf )
{
  INT nSize, i;
  switch ( gcProcessFinderMethod )
  {
    case 'T':
    {
      nSize = xvTHModules.size ();
      PMODULEENTRY32 pme;
      for ( i = 0; i < nSize; i++ )
      {
        pme = xvTHModules[i];
        if ( !pme )
          continue;
        if ( xdwAddress >= (DWORD)pme->modBaseAddr && xdwAddress <= ((DWORD)pme->modBaseAddr + pme->modBaseSize)  )
        {
          if ( szBuf )
            lstrcpy ( szBuf, pme->szExePath );
          return i;
        }
      }
      break;
    }

    case 'P':
    {
      nSize = xvPSModules.size ();
      PPSAPIMODULELIST ppsml;
      for ( i = 0; i < nSize; i++ )
      {
        ppsml = xvPSModules[i];
        if ( !ppsml )
          continue;
        if ( xdwAddress >= (DWORD)ppsml->mi.lpBaseOfDll && xdwAddress <= ( (DWORD)ppsml->mi.lpBaseOfDll + ppsml->mi.SizeOfImage )  )
        {
          if ( szBuf )
            lstrcpy ( szBuf, ppsml->szModuleName );
          return i;
        }
      }
      break;
    }
  }
  if ( szBuf )
    *szBuf = NULL;
  return -1;
}

VOID DbgDeleteModuleVector ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules )
{
  INT nSize, i;
  switch ( gcProcessFinderMethod )
  {
    case 'T':
    {
      nSize = xvTHModules.size ();
      PMODULEENTRY32 pme;
      for ( i = 0; i < nSize; i++ )
      {
        pme = xvTHModules[i];
        if ( pme )
          delete pme;
      }
      break;
    }

    case 'P':
    {
      PPSAPIMODULELIST ppsml;
      nSize = xvPSModules.size ();
      for ( i = 0; i < nSize; i++ )
      {
        ppsml = xvPSModules[i];
        if ( ppsml )
          delete ppsml;
      }
      break;
    }
  }
  
  xvTHModules.erase ( xvTHModules.begin (), xvTHModules.end () );
  xvPSModules.erase ( xvPSModules.begin (), xvPSModules.end () );
}

VOID DbgGetMoreModules ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwPID )
{
  switch ( gcProcessFinderMethod )
  {
    case 'T':
    {
      MODULEENTRY32 me32;
      PMODULEENTRY32 pme;
      HANDLE hSnap;

      me32.dwSize = sizeof(me32);
      hSnap = pCreateToolhelp32Snapshot ( TH32CS_SNAPMODULE, xdwPID );
      if ( hSnap == INVALID_HANDLE_VALUE ) 
        return;
      if ( pModule32First ( hSnap, &me32 ) )
      {
        do
        {
          if ( -1 == DbgFindModule ( xvTHModules, xvPSModules, (DWORD)me32.modBaseAddr ) )
          {
            pme = new MODULEENTRY32;
            memcpy ( pme, &me32, sizeof(MODULEENTRY32) );
            xvTHModules.push_back ( pme );
          }
        } while ( pModule32Next ( hSnap, &me32 ) );
      }
      CloseHandle (hSnap);
      break;
    }

    case 'P':
    {
      CHAR szModuleName[MAX_PATH];
      HMODULE hModules[1024];
      HANDLE hProcess;
      DWORD cbNeeded;
      UINT i;
      PPSAPIMODULELIST ppsml;
      PSAPIMODULELIST psml;
      hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, xdwPID );
      if (NULL == hProcess)
        break;
      if( !pEnumProcessModules ( hProcess, hModules, sizeof(hModules), &cbNeeded ) )
      {
        CloseHandle ( hProcess );
        break;
      }
      cbNeeded /= sizeof(HMODULE);
      for ( i = 0; i < cbNeeded; i++ )
      {
        pGetModuleInformation ( hProcess, hModules[i], &psml.mi, sizeof(MODULEINFO) );
        if ( -1 == DbgFindModule ( xvTHModules, xvPSModules, (DWORD)psml.mi.lpBaseOfDll ) )
        {
          ppsml = new PSAPIMODULELIST;
          ZeroMemory ( ppsml, sizeof(PSAPIMODULELIST) );
          ppsml->dwPID = xdwPID;
          ppsml->hModule = hModules[i];
          //lstrcpy ( ppsml->szProcessName, szProcessName );
          if ( pGetModuleFileNameEx ( hProcess, hModules[i], szModuleName, sizeof(szModuleName) ) )
            lstrcpy ( ppsml->szModuleName, szModuleName );
          else
            lstrcpy ( ppsml->szModuleName, "<Unknown>" );
          ppsml->mi = psml.mi;
          xvPSModules.push_back ( ppsml );
        }
      }
      CloseHandle ( hProcess );
      break;
    }
  }
}

VOID DbgRemoveModule ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, INT nIndex )
{
  switch ( gcProcessFinderMethod )
  {
    case 'T':
    {
      PMODULEENTRY32 pme;
      pme = xvTHModules[nIndex];
      delete pme;
      xvTHModules[nIndex] = NULL;
      break;
    }

    case 'P':
    {
      PPSAPIMODULELIST pme;
      pme = xvPSModules[nIndex];
      delete pme;
      xvPSModules[nIndex] = NULL;
      break;
    }
  }
}

VOID StartDebugger ( HWND, LPSTR pFile )
{
  SECURITY_ATTRIBUTES sa;
  DWORD dwThreadId;

  ZeroMemory ( &sa, sizeof(sa) );
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  dwThreadId = 0;

  HANDLE hThread = CreateThread ( &sa, NULL, DebuggerThreadFunc, (LPVOID)pFile, 0, &dwThreadId );
  CloseHandle ( hThread );
}

//Configuration Dialog Related Functions

VOID SetControlValues ( HWND hwndDlg )
{
  CHAR szBuf[10];

  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERPIPENAME ), gszServerName );
  wsprintf ( szBuf, "%d", gbServerTCPIPInputPort );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERTCPIPPORT ), szBuf );
  wsprintf ( szBuf, "%d", gbServerUDPInputPort );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERUDPPORT ), szBuf );
  CheckDlgButton ( hwndDlg, IDC_AUTOLINEFEED, gbAutoLineFeed ? BST_CHECKED  : BST_UNCHECKED );
  CheckDlgButton ( hwndDlg, IDC_ALWAYSONTOP, gbAlwaysOnTop ? BST_CHECKED  : BST_UNCHECKED );
  CheckDlgButton ( hwndDlg, IDC_MINIMIZETOSHELLTRAY, gbMinimizeToShellTray ? BST_CHECKED  : BST_UNCHECKED );
  CheckDlgButton ( hwndDlg, IDC_READONLYWINDOW, gbReadOnlyWindows ? BST_CHECKED : BST_UNCHECKED );

  HWND hCombo = GetDlgItem ( hwndDlg, IDC_COMMUNICATIONMETHOD );
  SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"Named Pipe (Good For NT Family)" );
  SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"TCP/IP Using Winsock" );
  SendMessage ( hCombo, CB_ADDSTRING, 0, (LPARAM)"UDP Using Winsock" );
  SendMessage ( hCombo, CB_SETCURSEL, gnClientPreferedMethod, 0 );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERNAME ), gszClientServerName );
  wsprintf ( szBuf, "%d", gnClientOutPort );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTOUTPORT ), szBuf );
  wsprintf ( szBuf, "%d", gnClientServerInPort );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERINPORT ), szBuf );
  CheckDlgButton ( hwndDlg, IDC_PAUSERESUME, gbSupportPause ? BST_CHECKED : BST_UNCHECKED );
}

VOID SetDefaultControlValues ( HWND hwndDlg )
{
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERPIPENAME ), "\\\\.\\pipe\\Debugger" );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERTCPIPPORT ), "8585" );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_SERVERUDPPORT ), "8586" );
  CheckDlgButton ( hwndDlg, IDC_AUTOLINEFEED, BST_CHECKED );
  CheckDlgButton ( hwndDlg, IDC_ALWAYSONTOP, BST_UNCHECKED );
  CheckDlgButton ( hwndDlg, IDC_MINIMIZETOSHELLTRAY, BST_UNCHECKED );
  CheckDlgButton ( hwndDlg, IDC_READONLYWINDOW, BST_UNCHECKED );

  HWND hCombo = GetDlgItem ( hwndDlg, IDC_COMMUNICATIONMETHOD );
  SendMessage ( hCombo, CB_SETCURSEL, 0, 0 );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERNAME ), "\\\\.\\pipe\\Debugger" );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTOUTPORT ), "8587" );
  SetWindowText ( GetDlgItem ( hwndDlg, IDC_CLIENTSERVERINPORT ), "8585" );
  CheckDlgButton ( hwndDlg, IDC_PAUSERESUME, BST_UNCHECKED );
}

INT WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow )
{
  WNDCLASSEX wc;
  MSG msg;

  ghInstance = hInstance;

  InitCommonControls ();

  OSVERSIONINFO ovi;
  ZeroMemory ( &ovi, sizeof(OSVERSIONINFO) );
  ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx ( (LPOSVERSIONINFO)&ovi );
  if ( ovi.dwPlatformId == 2 )
  {
    gbIsNT = TRUE;
    gdwNTVer = ovi.dwMajorVersion + ovi.dwMinorVersion - 3;
  }

  strlwr ( lpCmdLine );
  if ( !lstrcmp ( lpCmdLine, "-config" ) )
    DialogBox ( hInstance, "CONFIG_DIALOG", NULL, ConfigDialogProc );

  InitProcessHandlers ();
  
  ProcessInfoThreadFunc(NULL); //kannan
  if (true) return 0; //kannan

  ZeroMemory ( &wc, sizeof(wc) );
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = FrameWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 4;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon ( NULL, IDI_HAND );
  wc.hCursor = LoadCursor ( NULL, IDC_ARROW );
  wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
  wc.lpszClassName = gFrameClass;
  wc.hIconSm = LoadIcon ( NULL, IDI_HAND );

  if ( !RegisterClassEx ( &wc ) )
    return 0;

  wc.lpfnWndProc = ChildWindowProc;
  wc.hbrBackground = (HBRUSH)GetStockObject ( WHITE_BRUSH );
  wc.lpszClassName = gChildClass;

  if ( !RegisterClassEx ( &wc ) )
    return 0;

  ghMainMenu = LoadMenu ( hInstance, "FRAME_MENU" );
  ghChildMenu = LoadMenu ( hInstance, "CHILD_MENU" );

  gvTHProcessList.reserve ( 1000 );
  gvTHThreadList.reserve ( 1000 );
  gvTHModuleList.reserve ( 2000 );
  gvPSProcessList.reserve ( 1000 );
  gvPSModuleList.reserve ( 2000 );
  gvProcessTimes.reserve ( 1000 );
  gvAppList.reserve ( 1000 );
  gvPSDriverList.reserve ( 1000 );

  ghFrameWnd = CreateWindow ( gFrameClass, gFrameCaption, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
                              ghMainMenu, hInstance, NULL );
  if (ghFrameWnd == NULL)
  {
    DestroyMenu ( ghChildMenu );
    return 0;
  }

  ShowWindow ( ghFrameWnd, nCmdShow );
  UpdateWindow ( ghFrameWnd );
  //StartCommunicationServers ();
  PostMessage ( ghFrameWnd, WM_COMMAND,  MAKELPARAM(IDM_NEW, 0), 0 );
  while ( GetMessage ( &msg, NULL, 0, 0 ) )
  {
    if ( !TranslateMDISysAccel ( ghClientWnd, &msg ) )
    {
      TranslateMessage ( &msg );
      DispatchMessage ( &msg );
    }
  }
  DestroyMenu ( ghChildMenu );
  InterlockedIncrement ( (PLONG)&gnAppExit );
  //StopCommunicationServers ();
  if (ghProcessInfoThread != NULL)
  {
    TerminateThread ( ghProcessInfoThread, 0 );
    CloseHandle ( ghProcessInfoThread );
    ghProcessInfoThread = NULL;
  }
  if (ghinstToolHelp != NULL)
    FreeLibrary ( ghinstToolHelp );
  if (ghinstPSAPI != NULL)
    FreeLibrary ( ghinstPSAPI );
  UnLoadGlobalPSTHVectors ();
  if (ghPropertySheetRTClickMenu != NULL)
    DestroyMenu ( ghPropertySheetRTClickMenu );
  ghPropertySheetRTClickMenu = NULL;
  return msg.wParam;
}

