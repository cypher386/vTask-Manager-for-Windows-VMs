#include <windows.h>
#include <stdio.h>
#include "psapi.h"

void PrintProcessNameAndID( DWORD processID )
{
	char szProcessName[MAX_PATH] = "unknown";

		// Get a handle to the process.

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
																 PROCESS_VM_READ,
																 FALSE, processID );

		// Get the process name.

	if (NULL != hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
														 &cbNeeded) )
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, 
												 sizeof(szProcessName) );
		}
		else return;
	}
	else return;

		// Print the process name and identifier.

	printf( "%s (Process ID: %u)\n", szProcessName, processID );

	CloseHandle( hProcess );
}

void main( )
{
		// Get the list of process identifiers.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return;

		// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.

	for ( i = 0; i < cProcesses; i++ )
		PrintProcessNameAndID( aProcesses[i] );
}