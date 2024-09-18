// DirectPlayLauncher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define INITGUID

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <wtypes.h>
#include <cguid.h>
#include "dplay.h"
#include "dplobby.h"

#define NAMEMAX 			200 		// maximum size of a string name

DEFINE_GUID( MY_SESSION_GUID, 
0xd559fc00, 0xdc12, 0x11cf, 0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e );//same as dplaunch sample. make testing easier

#include <shellapi.h>

char* WideToMultiByte(wchar_t* input)
{
	size_t convertedChars = 0;

	size_t origsize = wcslen(input) + 1;
	size_t newsize = origsize * 2;

	char* nstring = new char[newsize];

	wcstombs_s(&convertedChars, nstring, newsize, input, _TRUNCATE);

	return nstring;
}

int _tmain(int argc, TCHAR** argv[])
{
	printf("Usage: DirectPlayLauncher.exe <GUID> <Session Name> <Player Name> <Host/Join> [Server]>\n");

	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

  	if( NULL == szArglist )
  	{
  		printf("CommandLineToArgvW error\n");
   		return 0;
  	}
   	//else for(int i=0; i<nArgs; i++) printf("%i: %ws\n", i, szArglist[i]);

	if (nArgs < 5)
	{
		printf("Not enough arguments\n");
		return 0;
	}

	HRESULT	hr = CoInitialize( NULL );
	if( FAILED(hr) )
	{
		printf( "CoInitialize error %i\n", hr );
		return 0;
	}

	LPDIRECTPLAYLOBBY3A pDPLobby = NULL;

	hr = CoCreateInstance( CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER,
						   IID_IDirectPlayLobby3A, (VOID**)&pDPLobby );
	if( FAILED(hr) )
	{
		printf( "CoCreateInstance error %i\n", hr );
		CoUninitialize();
		return 0;
	}

	GUID guidSession;
	guidSession = MY_SESSION_GUID;

	GUID gameGuid;
	hr = CLSIDFromString(szArglist[1], (LPCLSID)&gameGuid);

	if (FAILED(hr))
	{
		printf("CLSIDFromString error %i\n", hr);
		goto finish;
	}

	DPSESSIONDESC2 sessionInfo;
	ZeroMemory( &sessionInfo, sizeof(DPSESSIONDESC2) );
	sessionInfo.dwSize  = sizeof(DPSESSIONDESC2);
	sessionInfo.dwFlags = 0;// DPSESSION_xxx flags
	sessionInfo.guidInstance     = guidSession;//the instance must match for all players
	sessionInfo.guidApplication  = gameGuid;
	sessionInfo.dwMaxPlayers     = 0;				  // Maximum # players allowed in session
	sessionInfo.dwCurrentPlayers = 0;				  // Current # players in session (read only)
	sessionInfo.lpszSessionNameA = WideToMultiByte(szArglist[2]);//the session must match for all players
	sessionInfo.lpszPasswordA    = NULL;			  // ANSI password of the session (optional)
	sessionInfo.dwReserved1 = 0;
	sessionInfo.dwReserved2 = 0;
	sessionInfo.dwUser1     = 0;					  // For use by the application
	sessionInfo.dwUser2     = 0;
	sessionInfo.dwUser3     = 0;
	sessionInfo.dwUser4     = 0;

	DPNAME playerName;
	ZeroMemory( &playerName, sizeof(DPNAME) );
	playerName.dwSize  = sizeof(DPNAME);
	playerName.dwFlags = 0;// Not used. Must be zero.

	playerName.lpszShortNameA = WideToMultiByte(szArglist[3]); // ANSI short or friendly name
	playerName.lpszLongNameA  = WideToMultiByte(szArglist[3]); // ANSI long or formal name
	
	DPLCONNECTION connectInfo;
	ZeroMemory( &connectInfo, sizeof(DPLCONNECTION) );
	connectInfo.dwSize = sizeof(DPLCONNECTION);
	connectInfo.lpSessionDesc = &sessionInfo;
	connectInfo.lpPlayerName  = &playerName;
	connectInfo.guidSP = DPSPGUID_TCPIP;//only interested in TCP/IP

	if( wcscmp(szArglist[4], L"Host") == 0 )
	{
		connectInfo.dwFlags = DPLCONNECTION_CREATESESSION;
	}
	else//Join
	{
		if (nArgs < 6)
		{
			printf("Need Server\n");
			goto finish;
		}

		connectInfo.dwFlags = DPLCONNECTION_JOINSESSION;

		DPCOMPOUNDADDRESSELEMENT addressElements[2];

		// All DPADDRESS's must have a service provider chunk
		addressElements[0].guidDataType = DPAID_ServiceProvider;
		addressElements[0].dwDataSize   = sizeof(GUID);
		addressElements[0].lpData       = &connectInfo.guidSP;//only interested in TCP/IP

		CHAR inetdata[NAMEMAX];
		char* server = WideToMultiByte(szArglist[5]);

		for (int i = 0; i < lstrlenA(server); i++)
			inetdata[i] = server[i];

		addressElements[1].guidDataType = DPAID_INet;
		addressElements[1].dwDataSize   = lstrlenA(server) + 1;
		addressElements[1].lpData       = inetdata;

		DWORD dwElementCount = 2;

		DWORD dwAddressSize;

		// See how much room is needed to store this address
		hr = pDPLobby->CreateCompoundAddress( addressElements, dwElementCount,
		                                  NULL, &dwAddressSize );

		//if( hr != DPERR_BUFFERTOOSMALL )
		//{
		//	printf("CreateCompoundAddress unexpected %i\n", hr);
		//	goto finish;
		//}

		VOID* pAddress;
		pAddress = GlobalAllocPtr( GHND, dwAddressSize );

		if( pAddress == NULL )
		{
			printf("GlobalAllocPtr error\n");
			goto finish;
		}

		hr = pDPLobby->CreateCompoundAddress( addressElements, dwElementCount,
						                  pAddress, &dwAddressSize );
		if( FAILED(hr) )
		{
			printf("CreateCompoundAddress error %i\n", hr);
			GlobalFreePtr( pAddress );
			goto finish;
		}

		connectInfo.lpAddress     = pAddress;
		connectInfo.dwAddressSize = dwAddressSize;
	}

	DWORD processId;

	HANDLE lobbyEvents = CreateEvent(NULL, FALSE, FALSE, NULL);

	hr = pDPLobby->RunApplication( 0, &processId, &connectInfo, lobbyEvents );

	if (FAILED(hr))
	{
		printf("RunApplication error %i\n", hr);
		CloseHandle(lobbyEvents);
		goto finish;
	}

	printf("Game launched with PID %i\n", processId);

	while (true)//handle lobby messages
	{
		DWORD waitres = WaitForSingleObject(lobbyEvents, INFINITE);

		if (waitres != WAIT_OBJECT_0)
			break;

		DWORD message_flags = 0;
		DWORD data_size = 0;

		hr = pDPLobby->ReceiveLobbyMessage(0, processId, &message_flags, NULL, &data_size);
			
		if (hr != DPERR_BUFFERTOOSMALL)
		{
			printf("ReceiveLobbyMessage unexpected %i\n", hr);
			break;
		}

		void* data = calloc(1, data_size);

		hr = pDPLobby->ReceiveLobbyMessage(0, processId, &message_flags, data, &data_size);

		//byte* dataBytes = new byte[data_size];
		//memcpy(dataBytes, data, data_size);//debug bits

		free(data);

		//for (int i = 0; i < data_size; i++)
		//	printf("%02X ", dataBytes[i]);//debug bits
		//printf("\n");

		//delete[] dataBytes;

		if (FAILED(hr))
		{
			printf("ReceiveLobbyMessage error %i\n", hr);
			break;
		}

		if (message_flags == DPLSYS_CONNECTIONSETTINGSREAD)//ok to finish
		{
			printf("DPLSYS_CONNECTIONSETTINGSREAD\n");
			break;
		}

		if (message_flags == DPLSYS_DPLAYCONNECTSUCCEEDED)
		{
			printf("DPLSYS_DPLAYCONNECTSUCCEEDED\n");
			continue;
		}

		if (message_flags == DPLSYS_APPTERMINATED)
		{
			printf("DPLSYS_APPTERMINATED\n");
			continue;
		}

		if (message_flags == DPLSYS_NEWSESSIONHOST)
		{
			printf("DPLSYS_NEWSESSIONHOST\n");
			continue;
		}

		if (message_flags == DPLSYS_DPLAYCONNECTFAILED)
		{
			printf("DPLSYS_DPLAYCONNECTFAILED\n");
			continue;
		}

		if (message_flags == DPLSYS_GETPROPERTY)
		{
			printf("DPLSYS_GETPROPERTY\n");
			continue;
		}

		printf("Got some other flag: %i\n", message_flags);
	}

	CloseHandle(lobbyEvents);

finish:
	LocalFree(szArglist);//done with command line arguments

	pDPLobby->Release();

	CoUninitialize();

	return 0;
}
