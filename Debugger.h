
//Author: Kannan Balasubramanian
//Last Updated date: June 10, 2008


#ifndef _DEBUGWND_H
#define _DEBUGWND_H
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include <io.h>
#include <stdio.h>
#include "Debugger.rh"
#include <vector>
#include <tlhelp32.h>

using namespace std;

#define ULONG_PTR ULONG
#define WM_LOADDATA (WM_USER+10)

#define HF32_DEFAULT      1  // process's default heap
#define HF32_SHARED       2  // is shared heap

typedef HANDLE (WINAPI *LPCREATETOOLHELP32SNAPSHOT)(DWORD,DWORD);
LPCREATETOOLHELP32SNAPSHOT pCreateToolhelp32Snapshot = NULL;

typedef BOOL (WINAPI *LPPROCESS32FIRST)(HANDLE,LPPROCESSENTRY32);
LPPROCESS32FIRST pProcess32First = NULL;

typedef BOOL (WINAPI *LPPROCESS32NEXT)(HANDLE,LPPROCESSENTRY32);
LPPROCESS32FIRST pProcess32Next = NULL;

typedef BOOL (WINAPI *LPTHREAD32FIRST)(HANDLE,LPTHREADENTRY32);
LPTHREAD32FIRST pThread32First = NULL;

typedef BOOL (WINAPI *LPTHREAD32NEXT)(HANDLE,LPTHREADENTRY32);
LPTHREAD32NEXT pThread32Next = NULL;

typedef BOOL (WINAPI *LPTOOLHELP32READPROCESSMEMORY)(DWORD, LPCVOID, LPVOID, DWORD, LPDWORD); 
LPTOOLHELP32READPROCESSMEMORY pToolhelp32ReadProcessMemory = NULL;

typedef BOOL (WINAPI *LPHEAP32FIRST)(LPHEAPENTRY32, DWORD, DWORD);
LPHEAP32FIRST pHeap32First = NULL;

typedef BOOL (WINAPI *LPHEAP32NEXT)(LPHEAPENTRY32);
LPHEAP32NEXT pHeap32Next = NULL;

typedef BOOL (WINAPI *LPHEAP32LISTFIRST)(HANDLE, LPHEAPLIST32);
LPHEAP32LISTFIRST pHeap32ListFirst = NULL;

typedef BOOL (WINAPI *LPHEAP32LISTNEXT)(HANDLE, LPHEAPLIST32);
LPHEAP32LISTNEXT pHeap32ListNext = NULL;

typedef BOOL (WINAPI *LPMODULE32FIRST)(HANDLE, LPMODULEENTRY32);
LPMODULE32FIRST pModule32First = NULL;

typedef BOOL (WINAPI *LPMODULE32NEXT)(HANDLE, LPMODULEENTRY32);
LPMODULE32NEXT pModule32Next = NULL;

//Win NT Process Status Structures & Functions (PSAPI)
//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/perfmon/toolhelp_0lpj.asp

typedef BOOL (CALLBACK* PENUM_PAGE_CALLBACK)( LPVOID, PENUM_PAGE_FILE_INFORMATION, LPCTSTR );

typedef BOOL (WINAPI *LPGETPERFORMANCEINFO)(PPERFORMACE_INFORMATION, DWORD);
LPGETPERFORMANCEINFO pGetPerformanceInfo = NULL;

typedef BOOL (WINAPI *LPEMPTYWORKINGSET)(HANDLE);
LPEMPTYWORKINGSET pEmptyWorkingSet = NULL;

typedef BOOL (WINAPI *LPENUMDEVICEDRIVERS)(LPVOID, DWORD, LPDWORD);
LPENUMDEVICEDRIVERS pEnumDeviceDrivers = NULL;

typedef BOOL (WINAPI *LPENUMPAGEFILES)(PENUM_PAGE_CALLBACK, LPVOID);
LPENUMPAGEFILES pEnumPageFiles = NULL;

typedef BOOL (WINAPI *LPENUMPROCESSES)(DWORD *, DWORD, DWORD *);
LPENUMPROCESSES pEnumProcesses = NULL;

typedef BOOL (WINAPI *LPENUMPROCESSMODULES)(HANDLE, HMODULE *, DWORD, LPDWORD);
LPENUMPROCESSMODULES pEnumProcessModules = NULL;

typedef DWORD (WINAPI *LPGETDEVICEDRIVERBASENAME)(LPVOID, LPTSTR, DWORD);
LPGETDEVICEDRIVERBASENAME pGetDeviceDriverBaseName = NULL;

typedef DWORD (WINAPI *LPGETDEVICEDRIVERFILENAME)(LPVOID, LPTSTR, DWORD);
LPGETDEVICEDRIVERFILENAME pGetDeviceDriverFileName = NULL;

typedef DWORD (WINAPI *LPGETMAPPEDFILENAME)(HANDLE, LPVOID, LPTSTR, DWORD);
LPGETMAPPEDFILENAME pGetMappedFileName = NULL;

typedef DWORD (WINAPI *LPGETMODULEBASENAME)(HANDLE, HMODULE, LPTSTR, DWORD);
LPGETMODULEBASENAME pGetModuleBaseName = NULL;

typedef DWORD (WINAPI *LPGETMODULEFILENAMEEX)(HANDLE, HMODULE, LPTSTR, DWORD);
LPGETMODULEFILENAMEEX pGetModuleFileNameEx = NULL;

typedef BOOL (WINAPI *LPGETMODULEINFORMATION)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
LPGETMODULEINFORMATION pGetModuleInformation = NULL;

typedef DWORD (WINAPI *LPGETPROCESSIMAGEFILENAME)(HANDLE, LPTSTR, DWORD);
LPGETPROCESSIMAGEFILENAME pGetProcessImageFileName = NULL;

typedef BOOL (WINAPI *LPGETPROCESSMEMORYINFO)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
LPGETPROCESSMEMORYINFO pGetProcessMemoryInfo = NULL;

typedef BOOL (WINAPI *LPGETWSCHANGES)(HANDLE, PPSAPI_WS_WATCH_INFORMATION, DWORD);
LPGETWSCHANGES pGetWsChanges = NULL;

typedef BOOL (WINAPI *LPINITIALIZEPROCESSFORWSWATCH)(HANDLE);
LPINITIALIZEPROCESSFORWSWATCH pInitializeProcessForWsWatch = NULL;

typedef BOOL (WINAPI *LPQUERYWORKINGSET)(HANDLE, PVOID, DWORD);
LPQUERYWORKINGSET pQueryWorkingSet = NULL;

//My Local Structures, Variables & Defines
struct _tagAppList
{
  CHAR szAppName[2*MAX_PATH];
  CHAR szClassName[MAX_PATH];
  HWND hwnd;
  DWORD dwPID;
  DWORD dwPriority;
};
typedef _tagAppList APPLIST, *PAPPLIST;

struct _tagPSAPIProcessList
{
  DWORD dwPID;
  CHAR szProcessName[MAX_PATH];
  DWORD dwPriority;
  PROCESS_MEMORY_COUNTERS pmc;
};

typedef _tagPSAPIProcessList PSAPIPROCESSLIST, *PPSAPIPROCESSLIST;

struct _tagPSAPIModuleList
{
  DWORD dwPID;
  HMODULE hModule;
  CHAR szModuleName[MAX_PATH];
  CHAR szProcessName[MAX_PATH];
  MODULEINFO mi;
};

typedef _tagPSAPIModuleList PSAPIMODULELIST, *PPSAPIMODULELIST;

struct _tagProcessTime
{
  DWORD dwPID;
  CHAR szProcessName[MAX_PATH];
  WORD wMinrorVersion;
  WORD wMajorVersion;
  FILETIME ftCreation;
  FILETIME ftKernelTime;
  FILETIME ftUserTime;
};

typedef _tagProcessTime PROCESSTIME, *PPROCESSTIME;

struct _tagPSAPIDriverList
{
  DWORD dwhDriver;
  CHAR szDriverName[MAX_PATH];
  CHAR szDriverFileName[MAX_PATH];
};

typedef _tagPSAPIDriverList PSAPIDEIVERLIST, *PPSAPIDEIVERLIST;

#define IDM_FIRSTCHILD     1000
#define MAX_DEBUG_PROCESS  20
#define MAX_HEAP_ENTRIES   1000
#define PIPS_RTMENU_BASE   50
#define PIPS_RTMENU_MAX    80
#define LOCAL_TIMER_ID     100

//General & UI Variables
BOOL      gbAlwaysOnTop           = FALSE;
BOOL      gbMinimizeToShellTray   = FALSE;
BOOL      gbReadOnlyWindows       = FALSE;
HANDLE    ghSwitchEditEvent       = NULL;
HFONT     ghfont                  = NULL;
HINSTANCE ghInstance              = NULL;
HMENU     ghChildMenu             = NULL;
HMENU     ghMainMenu              = NULL;
HWND      ghActiveEdit            = NULL;
HWND      ghClientWnd             = NULL;
HWND      ghFrameWnd              = NULL;
INT       gnAppExit               = 0;
INT       gnDocCount              = 0;
LPSTR     gChildClass             = "DebugChildWindow";
LPSTR     gFrameCaption           = "Debugger";
LPSTR     gFrameClass             = "DebugWindow";

//Client Side Variables just for Configuration Dialog
BOOL      gbAutoLineFeed          = TRUE;
CHAR      gszClientServerName[MAX_PATH] = "\\\\.\\pipe\\Debugger";
INT       gnClientOutPort         = 8587;
INT       gnClientPreferedMethod  = 0;
INT       gnClientServerInPort    = 8585;
BOOL      gbSupportPause          = FALSE;


//Process Status Related Variables
vector    <PPSAPIDEIVERLIST>      gvPSDriverList;
vector    <PPSAPIPROCESSLIST>     gvPSProcessList;
vector    <PPSAPIMODULELIST>      gvPSModuleList;
vector    <PPROCESSENTRY32>       gvTHProcessList;
vector    <PTHREADENTRY32>        gvTHThreadList;
vector    <PMODULEENTRY32>        gvTHModuleList;
vector    <PPROCESSTIME>          gvProcessTimes;
vector    <PAPPLIST>              gvAppList;
BOOL      gbIsNT                  = FALSE;
DWORD     gdwNTVer                = 0; //1 = NT 4.0; 2 = 2000; 3 = XP
DWORD     gnPages                 = 0;
HANDLE    ghProcessInfoThread     = NULL;
HINSTANCE ghinstToolHelp          = NULL;
HINSTANCE ghinstPSAPI             = NULL;
HMENU     ghPropertySheetRTClickMenu = NULL;
HWND      ghEDWWindow             = NULL;
HWND      ghAllWindowWindow       = NULL;
HWND      ghProcessInfoWindow     = NULL;
HWND      ghProcessTimesWindow    = NULL;
HWND      ghPSDriversWindow       = NULL;
HWND      ghPSModulesWindow       = NULL;
HWND      ghPSProcessWindow       = NULL;
HWND      ghPSMemoryUsageWindow   = NULL;
HWND      ghPSPerformanceInfoWindow = NULL;
HWND      ghTHProcessWindow       = NULL;
HWND      ghTHModuleWindow        = NULL;
HWND      ghTHHeapWindow          = NULL;
HWND      ghTHThreadWindow        = NULL;

//Sort Variables
INT       gnEDWAppListAscending    = FALSE;
INT       gnEDWAppListColumn       = -1;
INT       gnProcessTimeListAscending   = FALSE;
INT       gnProcessTimeListColumn      = -1;
INT       gnPSDriversListAscending     = FALSE;
INT       gnPSDriversListColumn        = -1;
INT       gnPSMemoryUsageListAscending = FALSE;
INT       gnPSMemoryUsageListColumn    = -1;
INT       gnPSModuleListAscending  = FALSE;
INT       gnPSModuleListColumn     = -1;
INT       gnPSProcessListAscending = FALSE;
INT       gnPSProcessListColumn    = -1;
INT       gnTHHeapEntryAscending   = FALSE;
INT       gnTHHeapEntryColumn      = -1;
INT       gnTHHeapListAscending    = FALSE;
INT       gnTHHeapListColumn       = -1;
INT       gnTHModuleListAscending  = FALSE;
INT       gnTHModuleListColumn     = -1;
INT       gnTHProcessListAscending = FALSE;
INT       gnTHProcessListColumn    = -1;
INT       gnTHThreadListAscending  = FALSE;
INT       gnTHThreadListColumn     = -1;
INT       gnListCount              = 0;
INT       gnModuleSize             = 0;

//Debugger Related Variables
CHAR      gcProcessFinderMethod   = 'N'; //'N' = None, 'P' = PSAPI, 'T' = THAPI

//Pipe Related Variables
CHAR      gszServerName[MAX_PATH] = "\\\\.\\pipe\\Debugger";
HANDLE    ghPipe                  = NULL;
HANDLE    ghPipeThread            = NULL;

//TCP/IP Related Variables
HANDLE    ghWinSockTCPIPThread    = NULL;
INT       gbServerTCPIPInputPort  = 8585;
WSAEVENT  ghWinSockTCPIPEvent     = NULL;

//UDP Related Variables
BOOL      gnResetUDPServer        = FALSE;
HANDLE    ghWinSockUDPThread      = NULL;
INT       gbServerUDPInputPort    = 8586;
SOCKET    gsockUDP                = NULL;
WSAEVENT  ghWinSockUDPEvent       = NULL;

//Local Functions

//General Functions
BOOL          StartCommunicationServers       ();
BOOL          StopCommunicationServers        ();
UINT CALLBACK BrowseHookProc                  ( HWND   hwnd , UINT uMsg, WPARAM wParam, LPARAM lParam );
VOID          ReadRegistry                    ();
VOID          WriteRegistry                   ( HWND   hwndDlg );

//Helper Functions
BOOL          CheckAndDisplay                 ( LPSTR  pStr, DWORD nBytes, HWND hwnd = NULL );
BOOL          GetFilenameFromHandle           ( HANDLE hFile, HANDLE hProcess, LPSTR szBuf );
INT           CenterWindow                    ( HWND   hWnd );
unsigned      DateToJulianDay                 ( unsigned m, unsigned d, unsigned y );
VOID          ConvertFileTime2String          ( FILETIME* pft, LPSTR szBuf, BOOL bDuration = FALSE );
VOID          DoWin2000SocketFix              ( SOCKET sock );
VOID          GetPriorityClassFromPriorityString ( LPSTR pStr, DWORD& dwPriority );
VOID          GetPriorityStringFromPriorityClass ( DWORD dwPriority, LPSTR pStr );
VOID          GetStringThreadBasePriorityFromDWORD ( DWORD dwBasePriority, LPSTR szBuf );
VOID          PrintError                      ();
VOID          WSAPrintError                   ( LPSTR  pTitle = NULL );
   
//UI Callbacks
BOOL    CALLBACK CloseAllProc                 ( HWND   hwnd, LPARAM lParam );
BOOL    CALLBACK ConfigDialogProc             ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
HWND             CheckAndActivate             ( HWND   hEdit = NULL );
LRESULT CALLBACK ChildWindowProc              ( HWND   hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK FrameWindowProc              ( HWND   hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

//Process Status Related Functions
BOOL    CALLBACK EnumDesktopWindowsDialogProc ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK ListAllWindowsDialogProc     ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK APIListProcessTimesDialogProc( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK PSAPIListDeriverDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK PSAPIListMemoryUsageDialogProc ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK PSAPIListModulesDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK PSAPIListProcessDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK PSAPIPerformanceInfoDialogProc ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK THAPIListHeapsDialogProc     ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK THAPIListModulesDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK THAPIListProcessDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL    CALLBACK THAPIListThreadsDialogProc   ( HWND   hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
DWORD   WINAPI   ProcessInfoThreadFunc        ( LPVOID );
HTREEITEM        InsertItemInTree             ( HWND   hWndTree, HWND hwnd, HTREEITEM hParent );
INT     CALLBACK EDWAppListCompareFunc        ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK ProcessTimeCompareFunc       ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK PSAPIListDriversCompareFunc  ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK PSAPIListMemoryUsageCompareFunc ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK PSAPIListModuleCompareFunc   ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK PSAPIListProcessCompareFunc  ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK THAPIListHeapCompareFunc     ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK THAPIListHeapEntryCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK THAPIListModuleCompareFunc   ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK THAPIListProcessCompareFunc  ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
INT     CALLBACK THAPIListThreadCompareFunc   ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
VOID             DeleteListViewItems          ( HWND   hwnd, BOOL bClearParams = FALSE );
VOID             GetStringHeapEntryFlagFromDWORD ( DWORD dwFlags, LPSTR szBuf );
VOID             GetStringHeapListFlagFromDWORD  ( DWORD dwFlags, LPSTR szBuf );
VOID             GetTHProcessNameFromProcessID   ( DWORD dwPID, LPSTR szBuf );
VOID             InitProcessHandlers          ();
VOID             ListProcesses                ();
VOID             LoadAndFillEDWAppList        ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillProcessTimeList   ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillPSDriverList      ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillPSMemoryUsageList ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillPSModuleList      ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillPSPerformanceInfo ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillPSProcessList     ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillTHHeapEntryList   ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillTHHeapList        ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillTHModuleList      ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillTHProcessList     ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadAndFillTHThreadList      ( HWND   hwnd, BOOL bLoadColumns = FALSE );
VOID             LoadChildWindowsInTree       ( HWND   hWndTree, HWND hwnd, HTREEITEM hParent );
VOID             LoadGlobalEDWAppList         ();
VOID             LoadGlobalProcessTimesVector ();
VOID             LoadGlobalProcessTimesVector ( DWORD dwPID, LPSTR szProcessName );
VOID             LoadGlobalPSModuleVector     ( DWORD dwPID );
VOID             LoadGlobalPSVectors          ();
VOID             LoadGlobalPSTHVectors        ( BOOL bManualRefresh = FALSE );
VOID             LoadGlobalTHModuleVector     ( DWORD dwPID );
VOID             LoadGlobalTHVectors          ();
VOID             UnLoadGlobalEDWAppList       ();
VOID             UnLoadGlobalProcessTimesVector ();
VOID             UnLoadGlobalPSVectors        ();
VOID             UnLoadGlobalPSTHVectors      ();
VOID             UnLoadGlobalTHVectors        ();
VOID             PSDialogRefresh              ();  //Pending
VOID             THDialogRefresh              ();

//Pipe Related Functions
DWORD   WINAPI   PipeThreadFunc               ( LPVOID );

//TCP/IP Related Functions
DWORD   WINAPI   TCPIPWinSockThreadFunc       ( LPVOID );

//UDP Related Functions
DWORD   WINAPI   UDPWinSockThreadFunc         ( LPVOID );
VOID             ResetUDPServer               ();

//Debugger Related Functions
DWORD   WINAPI   DebuggerThreadFunc           ( LPVOID );
INT              DbgFindModule                ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwAddress, LPSTR szBuf = NULL );
INT              DbgFindModuleFromIP          ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwAddress, LPSTR szBuf = NULL );
VOID             DbgDeleteModuleVector        ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules );
VOID             DbgGetMoreModules            ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, DWORD xdwPID );
VOID             DbgRemoveModule              ( vector <PMODULEENTRY32>& xvTHModules, vector <PPSAPIMODULELIST>& xvPSModules, INT nIndex );
VOID             StartDebugger                ( HWND   hwnd, LPSTR pFile );

//Configuration Dialog Related Functions
VOID             SetControlValues             ( HWND   hwndDlg );
VOID             SetDefaultControlValues      ( HWND   hwndDlg );

#endif //_DEBUGWND_H
