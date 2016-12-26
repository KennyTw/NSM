// MsnConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>

#include "MsnClient.h"


int main(int argc, char* argv[])
{
    CMsnClient *LoginClient = new CMsnClient();

	HANDLE hand = LoginClient->MSNLogin("207.46.96.153",1863);
    WaitForSingleObject(hand , INFINITE);

	char NewIP[15];
	int Newport;

	if (LoginClient->LastStatus == MSN_NEED_RELOGIN)
	{
		strcpy(NewIP,LoginClient->XFRIP);
		Newport = LoginClient->XFRport;
		delete LoginClient;

		LoginClient = new CMsnClient();

		HANDLE hand = LoginClient->MSNLogin(NewIP,Newport);
        //WaitForSingleObject(hand , INFINITE);


	
	}


	printf("OK Input\r\n");
	int c ; 
	while (( c = _getch()) != 'e')
	{
		if (c == 'm')
		{
			LoginClient->SendMsg("kenny@ezfly.com","hello");
		}
	}

	

	return 0;
}

