// HttpServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HttpAdminServer.h"
#include <conio.h>

int main(int argc, char* argv[])
{
	CHttpAdminServer *srv = new CHttpAdminServer();
    srv->StartServer();

	getch();

	return 0;
}

