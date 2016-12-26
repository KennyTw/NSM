// HttpAdminServer.cpp: implementation of the CHttpAdminServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HttpAdminServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHttpAdminServer::CHttpAdminServer(): CBaseServer(IPPROTO_TCP,80)
{

}

CHttpAdminServer::~CHttpAdminServer()
{

}


int CHttpAdminServer::OnConnect(sockaddr* RemoteIp)
{
	return NO_ERROR;

}
int CHttpAdminServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
    Sleep(10);

	return NO_ERROR;
}

void CHttpAdminServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}