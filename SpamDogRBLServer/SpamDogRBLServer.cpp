// SpamDogRBLServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include "RBLServer.h"


int main(int argc, char* argv[])
{
	
	CRBLServer rbl;
	rbl.StartServer();

	/*
	int rc = NO_ERROR;
	char Result[255]={0};

	CDnsClient dns;

	//194.248.123.124


	HANDLE handle  = dns.Resolve("192.168.1.3","124.123.248.194.rbl.softworking.com",qtA,Result,&rc);	 
	DWORD dwWaitResult = WaitForSingleObject(handle,INFINITE);

	printf("%s\r\n",Result);
 */

	getch();

	

	return 0;
}

