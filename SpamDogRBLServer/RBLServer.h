// RBLServer.h: interface for the CRBLServer class.
//
//////////////////////////////////////////////////////////////////////
#include "../Swsocket2/CoreClass.h"

#if !defined(AFX_RBLSERVER_H__1B29BE40_5077_4827_B219_8EB7313C0194__INCLUDED_)
#define AFX_RBLSERVER_H__1B29BE40_5077_4827_B219_8EB7313C0194__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRBLServer : public CBaseServer  
{
private:
	void ParseStr(char *DomainStr , int DataLen, char* ResultStr);
public:
	CRBLServer();
	virtual ~CRBLServer();

protected:
	virtual int OnAfterAccept(SOCKET_OBJ* sock); //�s�u��i�H�e���Ĥ@�� command
	virtual int OnConnect(sockaddr* RemoteIp); //�b �t�ܸ귽 ���e , �i�ڵ� client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

};

#endif // !defined(AFX_RBLSERVER_H__1B29BE40_5077_4827_B219_8EB7313C0194__INCLUDED_)
