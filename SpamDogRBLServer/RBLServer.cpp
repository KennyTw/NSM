// RBLServer.cpp: implementation of the CRBLServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RBLServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRBLServer::CRBLServer() : CBaseServer(IPPROTO_UDP,53)
{

}

CRBLServer::~CRBLServer()
{

}

int CRBLServer::OnAfterAccept(SOCKET_OBJ* sock) //連線後可以送的第一個 command
{
		return NO_ERROR;

}
int CRBLServer::OnConnect(sockaddr* RemoteIp) //在 配至資源 之前 , 可拒絕 client
{
		return NO_ERROR;

}

void CRBLServer::ParseStr(char *DomainStr , int DataLen, char* ResultStr)
{

	int len = 0; 
	ResultStr[0] = 0;

	for (int i = 0 ; i < DataLen ; i++)
	{

		len = *(DomainStr + i);

		if ((len & 0xC0) == 0xC0 || len == 0)
			break;

		if (len > 0)
		{
		    strncat(ResultStr,DomainStr + i + 1,len);
			strcat(ResultStr,".");			

			i = i + len;
		}

		
	
	}

	int ResLen = strlen(ResultStr);
	if (ResultStr[ResLen-1] == '.')
		ResultStr[ResLen-1] = 0;

	

}

int CRBLServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{

	int qtype = buf->buf[BytesRead-3];

	if (qtype == 1)
	{	


    	printf("RemoteIp: %s\n",inet_ntoa(((sockaddr_in *) &sock->addr)->sin_addr)); 		

		BYTE ResponseHeader[] = {0x85,0x80,0,1,0,1,0,0,0,0};
		BYTE RBLContent[] = {0xC0,0x0C, 0 ,1 ,0 ,1 ,0 ,0 ,0x0E ,0x10 ,0, 04, 0x7F,0,0 ,05};
		char SendQuery[255]={0};

		char QueryStr[255]={0};
		ParseStr(buf->buf+12,BytesRead - 12,QueryStr);

		//放入 id
		memcpy(SendQuery,buf->buf,1);
		memcpy(SendQuery+1,buf->buf+1,1);

		memcpy(SendQuery+2,ResponseHeader,10);
		memcpy(SendQuery+12,buf->buf+12,BytesRead - 12);
		memcpy(SendQuery+BytesRead,RBLContent,16);

	
		SendBufferData(sock,SendQuery,BytesRead + 16);

		printf("%s\r\n",QueryStr);


	
	}


	return NO_ERROR;
}
void CRBLServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}
