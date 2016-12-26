// HttpAdminServer.h: interface for the CHttpAdminServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HTTPADMINSERVER_H__35D01902_A268_41E6_84A2_931F5E37C778__INCLUDED_)
#define AFX_HTTPADMINSERVER_H__35D01902_A268_41E6_84A2_931F5E37C778__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Swsocket2/CoreClass.h"

class CHttpAdminServer : public CBaseServer  
{
public:
	CHttpAdminServer();
	virtual ~CHttpAdminServer();

protected:
	//virtual int OnAfterAccept(SOCKET_OBJ* sock); //�s�u��i�H�e���Ĥ@�� command
	virtual int OnConnect(sockaddr* RemoteIp); //�b �t�ܸ귽 ���e , �i�ڵ� client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
};

#endif // !defined(AFX_HTTPADMINSERVER_H__35D01902_A268_41E6_84A2_931F5E37C778__INCLUDED_)
