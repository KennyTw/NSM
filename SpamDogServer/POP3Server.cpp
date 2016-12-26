// POP3Server.cpp: implementation of the POP3Server class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "POP3Server.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPOP3Server::CPOP3Server(CRITICAL_SECTION*  mDBSection) : CBaseServer(IPPROTO_TCP,110)
{

}

CPOP3Server::~CPOP3Server()
{

}

int CPOP3Server::OnAfterAccept(SOCKET_OBJ* sock)
{

	return NO_ERROR;
}
int CPOP3Server::OnConnect(sockaddr* RemoteIp)
{

	return NO_ERROR;
}
int CPOP3Server::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{

	return NO_ERROR;
}	
void CPOP3Server::OnBeforeDisconnect(SOCKET_OBJ* sock)
{}
void CPOP3Server::OnDisconnect(int SockHandle)
{}
int CPOP3Server::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{

	return NO_ERROR;
}