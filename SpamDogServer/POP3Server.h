// POP3Server.h: interface for the POP3Server class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_POP3SERVER_H__280DDBDF_3FEA_4ECA_A3BD_B644A95ABD72__INCLUDED_)
#define AFX_POP3SERVER_H__280DDBDF_3FEA_4ECA_A3BD_B644A95ABD72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Swsocket2/CoreClass.h"

class CPOP3Server : public CBaseServer  
{
public:
	CPOP3Server(CRITICAL_SECTION*  mDBSection);
	virtual ~CPOP3Server();

	#define PO_NORMAL       1
	#define PO_DATA       2

protected:
	virtual int OnAfterAccept(SOCKET_OBJ* sock); //連線後可以送的第一個 command
	virtual int OnConnect(sockaddr* RemoteIp); //在 配至資源 之前 , 可拒絕 client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );	
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	virtual void OnDisconnect(int SockHandle);
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);

};

#endif // !defined(AFX_POP3SERVER_H__280DDBDF_3FEA_4ECA_A3BD_B644A95ABD72__INCLUDED_)
