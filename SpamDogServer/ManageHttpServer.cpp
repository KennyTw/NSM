// ManageHttpServer.cpp: implementation of the CManageHttpServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ManageHttpServer.h"
#include <shellapi.h>



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CManageHttpServer::CManageHttpServer(int port): CBaseServer(IPPROTO_TCP, port)
{
	strcpy(m_ManagedServiceName, "");
}

CManageHttpServer::~CManageHttpServer()
{

}

int CManageHttpServer::PrepareHeader(char *buf, int buflen, int status, int ContentLength, char* ContentType, char* OtherHeader)
{	
	char buffer[30];

	memset(buf, 0, buflen);
	strcpy(buf, "HTTP/1.1 ");
	strcat(buf, _itoa(status, buffer, 10));
	strcat(buf, " ");
	strcat(buf, GetCodeDesc(status, buffer));
	strcat(buf, "\r\nServer: SW-HTTPServer 1.0\r\n");
	
	if (OtherHeader!=0)
	{
		if(*OtherHeader!=0)
		{
			strcat(buf, OtherHeader);			
		}
	}
	// 401 authenticate
	//if(status==401)
	//	strcat(buf, "WWW-Authenticate: Negotiate\r\n");
	// should find some func. to get the format
	//strcat(buf, "Date: Fri, 12 Aug 2005 05:15:37 GMT\r\nContent-Length: ");
	// Last-Modified: Sat, 14 May 2005 17:40:20 GMT
                         
	             
	struct tm *newtime;
	long ltime;

	time( &ltime );
   
	newtime = gmtime( &ltime );   
	memset(buffer, 0, sizeof(buffer));
	strncpy(buffer, asctime(newtime), 24);
	
	strcat(buf, "Date: ");

	strcat(buf, buffer);
	strcat(buf, " GMT\r\n");
	strcat(buf, "Last-Modified: ");
	strcat(buf, buffer);	
	strcat(buf, " GMT\r\n");
	
	// connection always close
	strcat(buf, "Connection: close\r\n");

	// always no cache
	strcat(buf, "Cache-Control: no-cache, no-store, must-revalidate\r\n");

	strcat(buf, "Content-Length: ");
	strcat(buf, _itoa(ContentLength, buffer, 10));
	strcat(buf, "\r\nContent-Type: ");
	strcat(buf, ContentType);
	strcat(buf, "\r\n\r\n");

	return strlen(buf);
}

 
int CManageHttpServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	
	int rc = NO_ERROR;	
	
	CBaseServer::OnDataRead(sock,buf,BytesRead);

#ifdef _dbgManageHttpServer
	printf("CLIENT REQUEST DATA:\n%s\n",buf->buf);	
#endif

	char cookievalue[255];
	int i, offset, index;	

	HttpClientRequestData *httpclientrequestdata = (HttpClientRequestData *) sock->UserData;


//========================== general process ==========	
	
	// get action and filename 
	// and get cookie and get querystring
	if(httpclientrequestdata->HttpRequest==HR_NOT_DEFINE)
	{
		char action[10];
		for(i=0;i<sizeof(action);i++)
		{
			if (buf->buf[i]!=' ') // not space 
			  action[i] = buf->buf[i];
			else
			{
				action[i] = 0;
				break;
			}
		}
		if(stricmp(action, "get")==0)
			httpclientrequestdata->HttpRequest = HR_GET;
		else if(stricmp(action, "post")==0)
			httpclientrequestdata->HttpRequest = HR_POST;
		else
			httpclientrequestdata->HttpRequest = HR_UNKNOWN;

		// get filename
		char filename[255];
		offset = strlen(action)+1;
		for(i=0;i<sizeof(filename);i++)
		{
			index = i+offset;
			if (buf->buf[index]!=' ' && buf->buf[index]!='?') // not space and ?
			{
				filename[i] = buf->buf[index];
			}
			else
			{
				filename[i] = 0;
				break;
			}
		}
		strcpy(httpclientrequestdata->filename, filename);

		ParseQueryString(buf->buf, httpclientrequestdata->querystring);	
		ParseCookies(buf->buf, httpclientrequestdata->cookies);

	}

	// check post data
	// if post, we may need to reconstruct the userdata
	//map<string, string> post;
	//map<string, string> querystring;
	//map<string, string> cookies;


	
	if(httpclientrequestdata->HttpRequest==HR_POST || httpclientrequestdata->ClientRequestDataSize>0)
	{
		// there are some post data....				
//========================== "reconstruct" client request data =========	

		
		if(httpclientrequestdata->ClientRequestDataSize==0)
		{
			// first into OnDataRead
			httpclientrequestdata->ClientRequestDataSize = ParseClientContentLength(buf->buf);
			if(httpclientrequestdata->ClientRequestDataSize>0)
			{
				int iRealLength = GetRealContentLength(buf->buf);
				//printf("datalength = %d, reallength = %d\n", httpclientrequestdata->ClientRequestDataSize, iRealLength);
				if(iRealLength < httpclientrequestdata->ClientRequestDataSize)
				{
					// need to get more data..
					// use userdata to put the ClientRequestData
					httpclientrequestdata->RequestData = new char[httpclientrequestdata->ClientRequestDataSize+1];
					httpclientrequestdata->CurrentPos = 0;
					
					if(iRealLength>0)
					{
						strncpy(httpclientrequestdata->RequestData, GetContentStartPos(buf->buf), iRealLength);
						httpclientrequestdata->CurrentPos = iRealLength;
					}

					// we need to get chunk (the other post data....)
					return NO_ERROR;
				}
				else
				{
					// one time get all post data
					ParsePost(buf->buf, httpclientrequestdata->post);
				}
			}			
		}
		else
		{
			// post data second get in ....
			// we need to concat the other post data
			strncpy(&httpclientrequestdata->RequestData[httpclientrequestdata->CurrentPos],
				   buf->buf, strlen(buf->buf));
			httpclientrequestdata->CurrentPos += strlen(buf->buf);
			if(httpclientrequestdata->CurrentPos < httpclientrequestdata->ClientRequestDataSize)
			{
				// still need more post data
				return NO_ERROR;
			}
			else
			{
				// post data all in finished
				httpclientrequestdata->RequestData[httpclientrequestdata->CurrentPos] = 0;
				ParsePost(httpclientrequestdata->RequestData, httpclientrequestdata->post);
			}
		}
//====================================================================
		
 		
	}
	




// ============================ individual process =========




	GetLoginUser(buf->buf, cookievalue);


	// request /(root) -> response default page..
	//if(stricmp(filename, "")==0 || *cookievalue==0)
	char *filename = httpclientrequestdata->filename;
	if(stricmp(filename, "/")==0)
	{
		
		char buffer[2000];
		char content[2000];
		//char cookie[1000];

		strcpy(content, "<html><head><title>Login!</title></head><body><form action=list method=post>username:<input type=text name=username><br>\npassword:<input type=password name=password><br>\n<input type=submit><input type=reset></form></body></html>");
		PrepareHeader(buffer, 2000, 200, strlen(content), "text/html");
		strcat(buffer, content);

		//printf("return default page!\n%s\n--end\n", buffer);

		rc = SendLn(sock, buffer); 		
	}
	else if(stricmp(filename, "/ini")==0)
	{
		char buffer[2000];
		char content[2000];
		char tmpcontent[2000];

		// we need to traverse ini file
		//SpamDogServer.ini
		char iniFile[MAX_PATH];
		char strSections[2000];
		char strKeys[2000];
		int len, len2;
		sprintf(iniFile, "%s\\SpamDogServer.ini", m_ServicePath);
		
		strcpy(content, "<html>ini file<br>\n");
		strcat(content, "<body><form method=post action=/inisave>");
		
		// sections
		GetPrivateProfileSectionNames(strSections, sizeof(strSections), iniFile);
		len = sizeof(strSections);
		for(int i=0;i<len;i++)
		{
			if (strSections[i]!=0)
			{
				sprintf(tmpcontent, "[%s]<br>\n", &strSections[i]);
				strcat(content, tmpcontent);
				
				// keys
				GetPrivateProfileSection(&strSections[i], strKeys, sizeof(strKeys), iniFile);
				len2 = sizeof(strKeys);
				for(int i2=0;i2<len2;i2++)
				{
					if(strKeys[i2]!=0)
					{
						//sprintf(tmpcontent, "%s<br>\n", &strKeys[i2]);
						GetIniHtmlEditStr(tmpcontent, &strKeys[i2], &strSections[i]);
						sprintf(tmpcontent, "%s<br>\n", tmpcontent);
						strcat(content, tmpcontent);

						for(int j2=i2;j2<len;j2++)
						{
							if(strKeys[j2]!=0)
								continue;
							else
							{
								i2 = j2-1;
								break;
							}
						}
					}
					else if(strKeys[i2+1]==0)
						break;

				}


				for(int j=i;j<len;j++)
				{
					if(strSections[j]!=0)
						continue;
					else
					{
						i = j-1;
						break;
					}
				}
			}
			else if(strSections[i+1]==0)
				break;
		}
		
		strcat(content, "<input type=submit value='save and service restart'>");
		strcat(content, "<input type=reset value='original setting'><br>");
		strcat(content, "</form></body>");
		strcat(content, "</html>");
		PrepareHeader(buffer, 2000, 200, strlen(content), "text/html");
		strcat(buffer, content);
		rc = SendLn(sock, buffer); 		
		
		
	}
	else if(stricmp(filename, "/inisave")==0)
	{
		// save to ini file
		char iniFile[MAX_PATH];
		sprintf(iniFile, "%s\\SpamDogServer.ini", m_ServicePath);
		SaveToIniFileFromPostData(httpclientrequestdata->post, iniFile);

		// restart service
		//RestartServiceByName("W3SVC");
		if(stricmp(m_ManagedServiceName, "")!=0)
		{
			RestartServiceByName(m_ManagedServiceName);
		}
		
		char buffer[2000];
		char content[2000];
		strcpy(content, "<html>ok!<br>\n");
		strcat(content, "ini file saved!<br>\n");
		strcat(content, "<a href=/ini>back to inifile</a><br>\n");
		strcat(content, "</html>");
		PrepareHeader(buffer, 2000, 200, strlen(content), "text/html");
		strcat(buffer, content);
		rc = SendLn(sock, buffer); 
		
	}
	else if(stricmp(filename, "/list")==0)
	{
		// list m_pending folder file
		char buffer[20000];
		char content[20000];
		char cookieheader[255];

		if(httpclientrequestdata->HttpRequest==HR_POST)
		{
			// comes the auth info and we need to always set cookie
			char mybuf[255];
			strcpy(cookieheader, GetSetCookiesHeader("logined", "tim@test.com", mybuf));
			strcat(cookieheader, GetSetCookiesHeader("date", "20051004", mybuf));
			
		}
		else
		{
			strcpy(cookieheader, "");
		}
		
		strcpy(content, "<html>\n<body>\n<form method=post action=domove>");

		GetFileList(m_PendingFolder, buffer);
		strcat(content, buffer);
		
		strcat(content, "<input type=submit><input type=reset></form><br>\n");

		strcat(content, "your cookie = ");		
		strcat(content, cookievalue);
		strcat(content, "<br>\n");

		strcat(content, "</body>\n</html>");
		
		PrepareHeader(buffer, 20000, 200, strlen(content), "text/html", cookieheader);
		
		strcat(buffer, content);

		printf("==buffer begin==\n%s==buffer end==\n", buffer);

		printf("return list!\n");

		rc = SendLn(sock, buffer); 
	
	}
	else if(httpclientrequestdata->HttpRequest==HR_POST && stricmp(filename, "/domove")==0)
	{
		//printf("domove-->\n%s\n", buf->buf);
		char buffer[2000];
		char content[2000];

		// parse post data & move file

		int iMovedFiles = ParseThenMoveFile(buf->buf, m_PendingFolder, m_MailFolder);

		

		strcpy(content, "<html>ok!<br>\n");
		strcat(content, _itoa(iMovedFiles, buffer, 10));
		strcat(content, " file(s) moved!<br>\n");
		strcat(content, "<a href=\\list>back to list!</a><br>\n");
		strcat(content, "</html>");
		PrepareHeader(buffer, 2000, 200, strlen(content), "text/html");
		strcat(buffer, content);
		rc = SendLn(sock, buffer); 
	}
	else
	{
		char buffer[2000];
		char content[2000];
		strcpy(content, "<html><head><title>Bad request!</title></head><body>Bad Request!<br>\n<a href=/>To Root</a></body></html>");
		PrepareHeader(buffer, 2000, 400, strlen(content), "text/html");
		strcat(buffer, content);

		//printf("return default page!\n");

		rc = SendLn(sock, buffer); 	
	}
	
	
	//return rc;
	//return IO_NOTPOSTRECV;
	//return NO_ERROR;
	return SOCKET_ERROR;
}

int CManageHttpServer::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{
	//printf("OnDataWrite ok\n");

	return SOCKET_ERROR;
}

int CManageHttpServer::OnAfterAccept(SOCKET_OBJ* sock)
{
	HANDLE ProcessHeap = GetProcessHeap();
	sock->UserData =  (char *) CHeap::GetHeap(sizeof(HttpClientRequestData),&ProcessHeap);
	HttpClientRequestData *httpclientrequestdata = (HttpClientRequestData *) sock->UserData;

	// init. httpclientrequestdata data
	httpclientrequestdata->ClientRequestDataSize = 0;
	httpclientrequestdata->CurrentPos = 0;
	httpclientrequestdata->HttpRequest = HR_NOT_DEFINE;
	httpclientrequestdata->RequestData = NULL;

	httpclientrequestdata->querystring = new map<string, string>;
	httpclientrequestdata->cookies = new map<string, string>;
	httpclientrequestdata->post = new map<string, string>;


	return NO_ERROR;
}

void CManageHttpServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{
	HttpClientRequestData *httpclientrequestdata = (HttpClientRequestData *) sock->UserData;

	// free httpclientrequestdata allocate data
	delete(httpclientrequestdata->RequestData);
	delete(httpclientrequestdata->cookies);
	delete(httpclientrequestdata->querystring);
	delete(httpclientrequestdata->post);
}


int CManageHttpServer::GetFileList(char *folder, char *buf)
{
	WIN32_FIND_DATA fd;
	fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	char findpattern[MAX_PATH];
	strcpy(findpattern, folder);
	strcat(findpattern, "\\*.eml");

	strcpy(buf, "");
	HANDLE h = FindFirstFile(findpattern, &fd);
	if(h!=INVALID_HANDLE_VALUE)
	{
		do
		{
			// there should add some file info like subject. to. etc.
			/*
			strcat(buf, "<input type=checkbox name='");
			strcat(buf, fd.cFileName);
			strcat(buf, "' value=1>");
			strcat(buf, fd.cFileName);			
			strcat(buf, "<br>\n");
			*/
			strcat(buf, "<input type=checkbox name='move' value='");
			strcat(buf, fd.cFileName);
			strcat(buf, "'>");
			strcat(buf, fd.cFileName);			
			strcat(buf, "<br>\n");

		}
		while(FindNextFile(h, &fd));
		
	}
	FindClose(h);	

	return strlen(buf);

}

int CManageHttpServer::ParseThenMoveFile(char *postdata, char *source, char *destination)
{
	int iMovedFiles = 0;
	char *start, *end;
	char filename[MAX_PATH];
	char sourceFilename[MAX_PATH];
	char destinationFilename[MAX_PATH];
	int maxlen = strlen(postdata)-3;
	bool bFound=false;

	for(int i=0;i<maxlen;i++)
	{
		if(postdata[i]==13 && postdata[i+1]==10 && postdata[i+2]==13 && postdata[i+3]==10)
		{
			bFound=true;
			break;
		}
	}

	if(bFound==false) return iMovedFiles;
	
	start = &postdata[i+3];
	
	
	do
	{
		start = strstr(start, "move=");
		if(start==NULL) break;

		end = strstr(start, "&");
		if(end==NULL)
			end = postdata + strlen(postdata)-1;
		else
			end = end - 1;

		memset(filename, 0, sizeof(filename));
		strncpy(filename, start+5, end - (start+5) + 1);

		//printf("%s\n", filename);
		strcpy(sourceFilename, source);
		strcat(sourceFilename, "\\");
		strcat(sourceFilename, filename);

		strcpy(destinationFilename, destination);
		strcat(destinationFilename, "\\");
		strcat(destinationFilename, filename);

		MoveFileEx(sourceFilename, destinationFilename, MOVEFILE_REPLACE_EXISTING);
		
		iMovedFiles++;

		start = end;
		


	} while(start!=NULL);

	return iMovedFiles;

}

char * CManageHttpServer::GetCodeDesc(int Code, char *desc)
{
	switch(Code)
	{
	case 200:
		strcpy(desc, "OK");
		break;
	case 401:
		strcpy(desc, "Unauthorized");
		break;
	case 403:
		strcpy(desc, "Forbidden");
		break;
	case 404:
		strcpy(desc, "Not found");
		break;
	case 500:		
	default:
		strcpy(desc, "Internal Server Error");
	}
	return desc;
}

char * CManageHttpServer::GetCookies(char *header, char *name, char *value)
{
	char *start, *end;
	char namekey[255];
	
	strcpy(value, "");

	start = strstr(header, "Cookie: ");
	if(start!=NULL)
	{
		start += 8;
		strcpy(namekey, name);
		strcat(namekey, "=");
		start = strstr(start, namekey);
		if(start!=NULL)
		{
			start += strlen(namekey);
			for(end=start;end!=header+strlen(header)-1;end++)
			{
				if(*end==';' || *end=='\r' || *end=='\n' )
				{
					end--;
					break;
				}
			}
			
			//end = strstr(start, "; ");
			//if(end==NULL)
			//	end = header+strlen(header)-1;

			strncpy(value, start, end - start + 1);
			value[end-start+1]=0;
		}
	}
	
	return value;

	

}

char * CManageHttpServer::GetSetCookiesHeader(char *name, char *value, char *setheader, char *path, int expired_sec)
{
	// Set-Cookie: b=b+123+456; path=/
	char pathdata[MAX_PATH];
	if(path==0)
	{		
		strcpy(pathdata, "/");
	}
	else if(*path==0)
	{
		strcpy(pathdata, "/");
	}
	else
	{
		strcpy(pathdata, path);
	}
	
	// expires setting
	// expires=Thu, 03-Nov-2005 04:15:11 GMT
			   
	if(expired_sec==0)
	{
		sprintf(setheader, "Set-Cookie: %s=%s; path=%s\r\n", name, value, pathdata);
	}
	else
	{
		struct tm *newtime;
		long ltime;		

		time( &ltime );

		// set the expires time 
		// since ltime is a long type
		ltime += expired_sec; 		

		newtime = gmtime( &ltime );   
		sprintf(setheader, "Set-Cookie: %s=%s; path=%s; expires=%s GMT\r\n", name, value, pathdata, asctime(newtime));
	}


	return setheader;

}

char * CManageHttpServer::GetLoginUser(char *HttpHeader, char *username)
{
	strcpy(username, GetCookies(HttpHeader, "logined", username));
	return username;
}


int CManageHttpServer::ParseCookies(char *Content, StrPair *CookiesMap)
{
	int ret = 0;

	char *start, *end, *pos, *ContentEnd;
	char keychar[255];
	char valuechar[255];

	ContentEnd = Content+strlen(Content)-1;
	
	bool bKey, bValue;
	
	start = strstr(Content, "Cookie: ");

	if(start!=NULL)
	{
		start += 8;
		
		for(pos=start;pos<=ContentEnd;pos++)
		{
			//if(*pos=='\r' && *(pos+1)=='\n' && *(pos+2)=='\r' && *(pos+3)=='\n')
			if(*pos=='\r' && *(pos+1)=='\n')
			{
				ContentEnd = pos-1;
				break;
			}
		}

		
		for(pos=start;pos<=ContentEnd;pos++)
		{
			if(*pos==' ')
			{
				start = pos + 1;
				continue;
			}

			bKey = false;

			if(*pos=='=')
			{
				bKey = true;
				end = pos - 1;
				// from start to end is key
				memset(keychar, 0, sizeof(keychar));
				strncpy(keychar, start, end - start + 1);
				keychar[end-start+1]=0;

				// search value
				start = pos + 1;
				bValue = false;
				for(pos=start;pos<=ContentEnd;pos++)
				{
					if(*pos==';' || *pos=='\r' || *pos=='\n' )
					{
						bValue = true;
						end = pos - 1;
						break;
					}
				}

				if(bKey)
				{
					if(bValue==false)
					{
						end = pos -1;
					}

					// from start to end is value
					memset(valuechar, 0, sizeof(valuechar));
					strncpy(valuechar, start, end - start + 1);
					valuechar[end-start+1]=0; 
					
					// found key value pair
					CookiesMap->insert(pair<string, string>(keychar, valuechar));
					start = pos + 1;
					ret++;
				}
			}
		}	
	}


#ifdef _dbgManageHttpServer
	map<string, string>::iterator mp;
	string out = "Cookies:\n";
	for(mp=CookiesMap->begin();mp!=CookiesMap->end();mp++)
	{
		out = out + mp->first + "=" + mp->second + "\n";
	}
	printf("%s\n", out.c_str());
#endif
	
	return ret;
}

int CManageHttpServer::ParsePost(char *Content, StrPair *PostMap)
{
	int ret = 0;

	char *start, *end, *pos, *ContentEnd;
	char keychar[255];
	char keychar2[255];
	char valuechar[255];
	char valuechar2[255];
	string key, value;
	bool bKey, bValue, bPost;

	ContentEnd = Content+strlen(Content)-1;	
	
	bPost = false;
	pos = strstr(Content, "\r\n\r\n");
	if(pos) // p!=NULL
	{
		bPost = true;
		start = pos + strlen("\r\n\r\n");		
	}
	else
	{
		bPost = true;
		start = Content;
	}		

	if(bPost)
	{		
		for(pos=start;pos<=ContentEnd;pos++)
		{
			bKey = false;
			if(*pos=='=')
			{
				bKey = true;
				end = pos - 1;
				// from start to end is key
				memset(keychar, 0, sizeof(keychar));
				memset(keychar2, 0, sizeof(keychar2));
				strncpy(keychar, start, end - start + 1);
				keychar[end-start+1]=0;
				GetHtmlDecodeStr(keychar2, keychar);

				// search value
				start = pos + 1;
				bValue = false;
				for(pos=start;pos<=ContentEnd;pos++)
				{
					if(*pos=='&' || *pos=='\r' || *pos=='\n' )
					{
						bValue = true;
						end = pos - 1;
						break;
					}
				}

				if(bKey)
				{
					if(bValue==false)
					{
						end = pos - 1;
					}

					// from start to end is value
					memset(valuechar, 0, sizeof(valuechar));
					memset(valuechar2, 0, sizeof(valuechar2));
					strncpy(valuechar, start, end - start + 1);
					valuechar[end-start+1]=0; 
					GetHtmlDecodeStr(valuechar2, valuechar);

					// found key value pair
					PostMap->insert(pair<string, string>(keychar2, valuechar2));
					start = pos + 1;
					ret++;
				}
			}
		}	
	}

#ifdef _dbgManageHttpServer
	map<string, string>::iterator mp;
	string out = "PostData:\n";
	for(mp=PostMap->begin();mp!=PostMap->end();mp++)
	{
		out = out + mp->first + "=" + mp->second + "\n";
	}
	printf("%s\n", out.c_str());
#endif
	
	return ret;
}


int CManageHttpServer::ParseQueryString(char *Content, StrPair *QueryStringMap)
{
	int ret = 0;

	char *start, *end, *pos, *ContentEnd;
	char keychar[255];
	char valuechar[255];
	char valuechar2[255];
	string key, value;
	bool bKey, bValue;

	ContentEnd = Content+strlen(Content)-1;
	
	for(pos=Content;pos<=ContentEnd;pos++)
	{
		// find out the first line
		if(*pos=='\r' && *(pos+1)=='\n')
		{
			ContentEnd = pos;			
			break;
		}
	}

	start = strstr(Content, "?");
	if(start!=NULL && start < ContentEnd)
	{
		start = start + 1; // skip '?'
		for(pos=start;pos<=ContentEnd;pos++)
		{
			bKey = false;
			if(*pos=='=')
			{
				bKey = true;
				end = pos - 1;
				// from start to end is key
				memset(keychar, 0, sizeof(keychar));
				strncpy(keychar, start, end - start + 1);
				keychar[end-start+1]=0;

				// search value
				start = pos + 1;
				bValue = false;
				for(pos=start;pos<=ContentEnd;pos++)
				{
					if(*pos=='&' || *pos==' ' || *pos=='\r' || *pos=='\n' )
					{
						bValue = true;
						end = pos - 1;
						break;
					}
				}

				if(bKey)
				{
					if(bValue==false)
					{
						end = pos - 1;
					}

					// from start to end is value
					memset(valuechar, 0, sizeof(valuechar));
					memset(valuechar2, 0, sizeof(valuechar2));
					strncpy(valuechar, start, end - start + 1);
					valuechar[end-start+1]=0; 
					GetHtmlDecodeStr(valuechar2, valuechar);

					// found key value pair
					QueryStringMap->insert(pair<string, string>(keychar, valuechar2));
					start = pos + 1;
					ret++;
				}
			}
		}	
	}

#ifdef _dbgManageHttpServer
	map<string, string>::iterator mp;
	string out = "QueryString:\n";
	for(mp=QueryStringMap->begin();mp!=QueryStringMap->end();mp++)
	{
		out = out + mp->first + "=" + mp->second + "\n";
	}
	printf("%s\n", out.c_str());
#endif
	
	return ret;
}

void CManageHttpServer::GetHtmlDecodeStr(char *decodestr, char *HtmlEncodeStr)
{
	int i, j, len;
	char aChar, tmpHex[3];
	len = strlen(HtmlEncodeStr);
	memset(tmpHex, 0, 3);
	for(i=0, j=0;i<len;i++, j++)
	{
		if(HtmlEncodeStr[i]=='%')
		{
			// %0A%9D type like, always 2 digits
			tmpHex[0] = HtmlEncodeStr[i+1];
			tmpHex[1] = HtmlEncodeStr[i+2];
			aChar = GetHexStrValue(tmpHex);
			i+=2;
			decodestr[j] = aChar;
		}
		else
		{
			decodestr[j] = HtmlEncodeStr[i];
		}
	}

}

int CManageHttpServer::GetHexStrValue(char *strHex)
{
	int i, len, ret, tmp;
	// upper case and check if valid

	len = strlen(strHex);
	ret = 0;
	for(i=0;i<len;i++)
	{
		if(strHex[i] >='0' && strHex[i] <='9')
		{
			tmp = strHex[i] - '0';
		}
		else if(strHex[i] >='A' && strHex[i] <='F')
		{
			tmp = strHex[i] - 'A' + 10;
		}
		else if(strHex[i] >='a' && strHex[i] <='f')
		{
			tmp = strHex[i] - 'a' + 10;
		}
		else
		{
			return 0;
		}

		ret = ret * 16 + tmp;		
	}

	return ret;
}

void CManageHttpServer::GetIniHtmlEditStr(char *output, char *iniKeyValue, char *Section)
{
	char *p_eq = strstr(iniKeyValue, "=");	
	if(*p_eq !=NULL)
	{
		char key[255];
		char value[255];
		memset(key, 0, 255);
		memset(value, 0, 255);
		strncpy(key, iniKeyValue, p_eq - 1 - iniKeyValue + 1);
		strncpy(value, p_eq+1, strlen(iniKeyValue) - (p_eq - 1 - iniKeyValue + 1) + 1);
		sprintf(output, "%s=<input type=text name='%s$%s' value='%s'>", key, Section, key, value);
	}
	else
	{
		strcpy(output, "");
	}

}

int CManageHttpServer::ParseClientContentLength(char *Content)
{
	// Content-Length: 567
	char *p = strstr(Content, "Content-Length: ");
	if(p)  // p!=NULL
	{
		p += strlen("Content-Length: ");  // move p to start of numeric
		return atoi(p);
	}
	else
		return 0;
}

int CManageHttpServer::GetRealContentLength(char *Content)
{
	char *p = strstr(Content, "\r\n\r\n");
	if(p) // p!=NULL
	{
		p += strlen("\r\n\r\n");
		return strlen(p);
	}
	else
		return 0;
}

int CManageHttpServer::GetRealContent(char *dest, char *Content)
{
	char *p = strstr(Content, "\r\n\r\n");
	int tmpContentLength;
	if(p) // p!=NULL
	{
		p += strlen("\r\n\r\n");
		tmpContentLength = strlen(p);
		if(tmpContentLength) // tmpContentLength != 0
		{
			strncpy(dest, p, tmpContentLength);
			return tmpContentLength;
		}
		else
			return 0;
	}
	else
		return 0;
}

char * CManageHttpServer::GetContentStartPos(char *Content)
{
	char *p = strstr(Content, "\r\n\r\n");
	if(p) // p!=NULL
	{
		p += strlen("\r\n\r\n");
	}

	return p;

}

void CManageHttpServer::SaveToIniFileFromPostData(StrPair *post, char *iniFile)
{
	map<string, string>::iterator mp;	
	char section[255];
	char key[255];
	char *p;
	for(mp=post->begin();mp!=post->end();mp++)
	{
		strcpy(section, mp->first.c_str());
		p = strstr(section, "$");
		*p = 0;
		strcpy(key, p+1);
		WritePrivateProfileString(section, key, mp->second.c_str(), iniFile);
	}
	
	
}

void CManageHttpServer::RestartServiceByName(char *ServiceName)
{
	char param[255];	
	sprintf(param, "stop %s", ServiceName);	
	ExecuteWait("net", param);
	sprintf(param, "start %s", ServiceName);
	ExecuteWait("net", param);	
}

int CManageHttpServer::ExecuteWait(char *exe, char *param)
{
	SHELLEXECUTEINFO si;
	memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
	si.lpFile = exe;
	si.lpParameters = param;
	si.fMask = SEE_MASK_NOCLOSEPROCESS;
	si.nShow = SW_HIDE;
	ShellExecuteEx(&si);
	WaitForSingleObject(si.hProcess, 10000);
	return (int)si.hProcess;
}
