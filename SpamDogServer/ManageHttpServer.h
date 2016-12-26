// ManageHttpServer.h: interface for the CManageHttpServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MANAGEHTTPSERVER_H__29CB8123_6807_4D33_9A47_7062E56F1248__INCLUDED_)
#define AFX_MANAGEHTTPSERVER_H__29CB8123_6807_4D33_9A47_7062E56F1248__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Swsocket2/CoreClass.h"

#pragma warning (disable:4786)
#include <string>
#include <map>
#include <algorithm>  

//#define _dbgManageHttpServer

using namespace std;

typedef map<string, string> StrPair;


typedef struct _HttpClientRequestData
{
	int  ClientRequestDataSize;
	int  CurrentPos;
	int  HttpRequest;
	char filename[255];
	char *RequestData;
	map<string, string> *post;
	map<string, string> *querystring;
	map<string, string> *cookies;

} HttpClientRequestData;


class CManageHttpServer : public CBaseServer  
{

#define HR_NOT_DEFINE -1
#define HR_UNKNOWN     0
#define HR_GET         1
#define HR_POST        2


public:	
	char m_ManagedServiceName[MAX_PATH];

	char m_ServicePath[MAX_PATH];
	int GetHexStrValue(char *strHex);
	
	CManageHttpServer(int port);
	virtual ~CManageHttpServer();

	char m_PendingFolder[MAX_PATH];
	char m_MailFolder[MAX_PATH];

	char m_username[255];
	char m_password[255];

private:	

	char * GetLoginUser(char *HttpHeader, char *username);
	//char * GetSetCookiesHeader(char *name, char *value, char *setheader);
	char * GetSetCookiesHeader(char *name, char *value, char *setheader, char *path=0, int expired_sec=0);
	char * GetCookies(char *header, char *name, char *value);
	char * GetCodeDesc(int Code, char *desc);
	
	int ParseThenMoveFile(char *postdata, char *source, char *destination);
	int PrepareHeader(char *buf, int buflen, int status, int ContentLength, char* ContentType, char* OtherHeader=0);
	int GetFileList(char *folder, char *buf);	

protected:
	int ExecuteWait(char *exe, char *param);
	void RestartServiceByName(char *ServiceName);
	void SaveToIniFileFromPostData(StrPair *post, char *iniFile);
	char * GetContentStartPos(char *Content);
	int GetRealContent(char *dest, char *Content);
	int GetRealContentLength(char *Content);
	int ParseClientContentLength(char *Content);
	void GetIniHtmlEditStr(char *output, char *iniKeyValue, char *Section);
	void GetHtmlDecodeStr(char *decodestr, char *HtmlEncodeStr);
	int ParseCookies(char *Content, StrPair *CookiesMap);
	int ParsePost(char *Content, StrPair *PostMap);
	int ParseQueryString(char *Content, StrPair *QueryStringMap);
	virtual int OnAfterAccept(SOCKET_OBJ* sock); 
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	
	

};

#endif // !defined(AFX_MANAGEHTTPSERVER_H__29CB8123_6807_4D33_9A47_7062E56F1248__INCLUDED_)
