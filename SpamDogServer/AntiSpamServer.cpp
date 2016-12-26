// AntiSpamServer.cpp: implementation of the CAntiSpamServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AntiSpamServer.h"
#include <process.h>
#include <mbstring.h>
#include <TCHAR.H>

#include "../../SpamDog/Swparser/MailParser.h"



 

//#pragma warning(disable:4146)
//#import "C:\Program Files\common files\system\ado\msado15.dll"  no_namespace rename("EOF", "EndOfFile")

#include "../Swsocket2/SMTPClient.h"
#include "IMAPServer.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CAntiSpamServerDB::CAntiSpamServerDB(CRITICAL_SECTION*  DBSection) 
{
	 mDBSection = DBSection;

	 if (mDBSection == NULL) throw "DB Section NULL";
 
	 IpDB = NULL;
	 char path_buffer[_MAX_PATH];
	 char drive[_MAX_DRIVE];
	 char dir[_MAX_DIR];
	 char fname[_MAX_FNAME];
	 char ext[_MAX_EXT];


	   
       HINSTANCE hInstance = GetModuleHandle(NULL);
       GetModuleFileName(hInstance, path_buffer, MAX_PATH);

	   _splitpath( path_buffer, drive, dir, fname, ext );

	   char IndexDbPath[_MAX_PATH];
	  // char overflowDbPath[_MAX_PATH];
	 //  char datadbPath[_MAX_PATH];
	   

	   strcpy(IndexDbPath,drive);
	   strcat(IndexDbPath,dir); 

	   strcpy(PGPath,IndexDbPath);

	   char Path[MAX_PATH];
	   strcpy(Path,PGPath);
	   strcat(Path,"DB");
	   CreateDirectory(Path,NULL);

	   IpDB = NULL;
	   
	 
}

void CAntiSpamServerDB::RenewDB()
{
 
	if (IpDB == NULL) throw;

		   IpDB->CloseDB();
	       delete IpDB;

		   char IndexDbPath[_MAX_PATH];
		   char overflowDbPath[_MAX_PATH];
		   char datadbPath[_MAX_PATH];	   

		   strcpy(IndexDbPath,PGPath);	   	   
		   strcpy(overflowDbPath,IndexDbPath);
		   strcpy(datadbPath,IndexDbPath);

		   strcat(IndexDbPath,"DB//Ip.db1");
		   strcat(overflowDbPath,"DB//Ip.db2");
		   strcat(datadbPath,"DB//Ip.db3");

		   DeleteFile(IndexDbPath);
		   DeleteFile(overflowDbPath);
		   DeleteFile(datadbPath);

		   IpDB = new CDB(mDBSection,IndexDbPath ,overflowDbPath ,datadbPath ,sizeof(AntiSpamServerData),1024 * 10);

		   IpDB->OpenDB();
	 


}

void CAntiSpamServerDB::OpenIpDB()
{


		

 	   char IndexDbPath[_MAX_PATH];
	   char overflowDbPath[_MAX_PATH];
	   char datadbPath[_MAX_PATH];	   
   
	   strcpy(IndexDbPath,PGPath);
	   strcpy(overflowDbPath,PGPath);
	   strcpy(datadbPath,PGPath);

	   strcat(IndexDbPath,"DB\\Ip.db1");
	   strcat(overflowDbPath,"DB\\Ip.db2");
	   strcat(datadbPath,"DB\\Ip.db3");


	   IpDB = new CDB(mDBSection,IndexDbPath ,overflowDbPath ,datadbPath ,0,1024 * 10);
	   IpDB->OpenDB();

	 


}
void CAntiSpamServerDB::CloseIpDB()
{

	if (IpDB != NULL)
	{
		IpDB->CloseDB();
		delete IpDB ;	
	}
	
}

time_t CAntiSpamServerDB::LastConnectTime(char* IP)
{
 
	if (IpDB == NULL) throw;

	 
		AntiSpamServerData m_AntiSpamServerData;
		memset(&m_AntiSpamServerData,0,sizeof(m_AntiSpamServerData));

		time_t LastUseTime=0;
		SResult sres = IpDB->SelectKey(IP);
		if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
		{
		   //找不到資料則 insert
			time(&m_AntiSpamServerData.LastConnectTime);
			int datapos = IpDB->InsertData((char *) &m_AntiSpamServerData,sizeof(m_AntiSpamServerData));
			IpDB->InsertKey(IP,datapos);
		  
		}
		else
		{   
		   //讀出資料
			IpDB->SelectData(sres.DataFilePos,(char *) &m_AntiSpamServerData , sizeof(m_AntiSpamServerData));
			LastUseTime = m_AntiSpamServerData.LastConnectTime;
			

			//update Connecttime
			time(&m_AntiSpamServerData.LastConnectTime);			
			IpDB->UpdateData((char *) &m_AntiSpamServerData.LastConnectTime , sizeof(m_AntiSpamServerData.LastConnectTime),sres.DataFilePos,FIELD_OFFSET(AntiSpamServerData,LastConnectTime));
			

			

		}  
		

		
		return LastUseTime;
 

}

bool CAntiSpamServerDB::CheckBadIpExists(char* IP)
{
	if (IpDB == NULL) throw;

		AntiSpamServerData m_AntiSpamServerData;
		memset(&m_AntiSpamServerData,0,sizeof(m_AntiSpamServerData));

		 
		SResult sres = IpDB->SelectKey(IP);
		if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
		{
		   return false;
		  
		}
		else
		{   
		   //讀出資料
			IpDB->SelectData(sres.DataFilePos,(char *) &m_AntiSpamServerData , sizeof(m_AntiSpamServerData));
			
			if (m_AntiSpamServerData.BadRate > m_AntiSpamServerData.OkRate)
			{
				return true;
			}
			else
			{
				return false;
			}			

		}    



}
bool CAntiSpamServerDB::CheckOkIpExists(char* IP)
{

	if (IpDB == NULL) throw;
	
		AntiSpamServerData m_AntiSpamServerData;
		memset(&m_AntiSpamServerData,0,sizeof(m_AntiSpamServerData));

		 
		SResult sres = IpDB->SelectKey(IP);
		if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
		{
		   return false;		  
		}
		else
		{   
		   //讀出資料
			IpDB->SelectData(sres.DataFilePos,(char *) &m_AntiSpamServerData , sizeof(m_AntiSpamServerData));
			
			if (m_AntiSpamServerData.BadRate < m_AntiSpamServerData.OkRate)
			{
				return true;
			}
			else
			{
				return false;
			}			

		}    


}
void CAntiSpamServerDB::UpdateIp(char* IP , int OkRate , int BadRate)
{

	if (IpDB == NULL) throw;

		AntiSpamServerData m_AntiSpamServerData;
		memset(&m_AntiSpamServerData,0,sizeof(m_AntiSpamServerData));

		m_AntiSpamServerData.BadRate = BadRate;
		m_AntiSpamServerData.OkRate = OkRate;

		
		SResult sres = IpDB->SelectKey(IP);
		if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
		{
		   //找不到資料則 insert
			//time(&m_AntiSpamServerData.LastConnectTime);
			int datapos = IpDB->InsertData((char *) &m_AntiSpamServerData,sizeof(m_AntiSpamServerData));
			IpDB->InsertKey(IP,datapos);
		  
		}
		else
		{   
		   //讀出資料
			IpDB->SelectData(sres.DataFilePos,(char *) &m_AntiSpamServerData , sizeof(m_AntiSpamServerData));
			//LastUseTime = m_AntiSpamServerData.LastConnectTime;
			

			//update Connecttime
			//time(&m_AntiSpamServerData.LastConnectTime);			
			IpDB->UpdateData((char *) &m_AntiSpamServerData.LastConnectTime , sizeof(m_AntiSpamServerData.LastConnectTime),sres.DataFilePos,FIELD_OFFSET(AntiSpamServerData,LastConnectTime));
			

			

		}    

}

CAntiSpamServerDB::~CAntiSpamServerDB()
{
 //if (IpDB != NULL) 
//	 delete IpDB ;
 
}






CAntiSpamServer::CAntiSpamServer() : CBaseServer(IPPROTO_TCP,25)
{
	SocketCount = 0;

	LogFp = NULL;
	BlockIpList = NULL;

	AllAccount = NULL;
	dns = new CDnsClient();
	
	
	InitializeCriticalSection(&mDBSection);
	InitializeCriticalSection(&mMailDBSection);
	

	ServerDB = new CAntiSpamServerDB(&mDBSection);
	MailServerDB = new CServerDB(&mMailDBSection);

	MailServerDB->OpenDB();
	ServerDB->OpenIpDB();

	
	Running = true;

	
	memset(&Settings,0,sizeof(Settings));

	IniServer();
	
	SocketTimeout = 15; //五分鐘
	RelayMailEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	unsigned threadID = 0;
    RelayMailThreadHandle = (HANDLE) _beginthreadex(NULL, 0, RelayMailThread, (LPVOID) this, 0, &threadID);
	//SetThreadName(threadID,"Spam1");

	ServerProcThreadHandle  = (HANDLE) _beginthreadex(NULL, 0, ServerProcThread, (LPVOID) this, 0, &threadID);
//	SetThreadName(threadID,"Spam2");


	//mHttpServer = new CManageHttpServer(8000);

	//strcpy(mHttpServer->m_PendingFolder,"c:\\");
	//strcpy(mHttpServer->m_MailFolder,"c:\\");
	//strcpy(mHttpServer->m_ServicePath,"c:\\");
	

	//strcpy(mHttpServer->m_username,"kenny");
	//strcpy(mHttpServer->m_password,"1234");


    //GetADAccList("192.168.6.23","mgt@ezfly.com","ezfly\\kenny","",NULL);

	mImapServer = new CIMAPServer(&mMailDBSection);
	mImapServer->SetRelayMailEvent(RelayMailEvent);
	mImapServer->StartServer();

	//mHttpServer->StartServer();

	
}

CAntiSpamServer::~CAntiSpamServer()
{


	if (LogFp != NULL) 
		fclose(LogFp);
	
	DestoryAllAccountList();

	MailServerDB->CloseDB();
	ServerDB->CloseIpDB();
	
	//delete mHttpServer;
	delete mImapServer;
	delete MailServerDB;
	delete ServerDB;

	Running  = false;

	 

	//terminated = true; //因為自己起的 RelayMail Thread 也是由此控制

	SetEvent(RelayMailEvent);
	CloseHandle(RelayMailEvent);

	WaitForSingleObject(RelayMailThreadHandle,INFINITE);
	CloseHandle(RelayMailThreadHandle);

	WaitForSingleObject(ServerProcThreadHandle,INFINITE);
	CloseHandle(ServerProcThreadHandle);

	DeleteCriticalSection (&mDBSection);
	DeleteCriticalSection (&mMailDBSection);
    
	delete dns;
	
}

void CAntiSpamServer::WriteLog(char *LogStr)
{
	if (LogStr == NULL || LogStr[0] == 0)
		return;
	
	IniLogFile();

	char *DupLogStr = _strdup(LogStr);

	if (DupLogStr == NULL) return;
	//char *DupLogStr = LogStr;
	    
	struct tm *newtime;	

	long ltime;

	time( &ltime );

	/* Obtain coordinated universal time: */
	newtime = localtime ( &ltime );	
	
		
	char NowTime[255];
	strftime(NowTime,255,"%Y/%m/%d %H:%M:%S",newtime);

	int len = strlen(DupLogStr);

	if (DupLogStr[len-1] == '\n' && DupLogStr[len-2] == '\r')
		DupLogStr[len-2] = 0;

	fprintf(LogFp,"%s	%s\r\n",NowTime,DupLogStr);
	
	fflush(LogFp);
	//LeaveCriticalSection(&mLogSection);

	delete DupLogStr;

	

}

void CAntiSpamServer::RetriveAccountList(SMTPData *userdata,char* Email)
{
	AllAccountList *ptr = AllAccount;
	while (ptr)
	{	  
	    
		if (ptr->AList != NULL && stricmp(ptr->AList->EMail,Email) == 0)
		{ 

			if (ptr->AccCount == 1)
			{
			   //直接放入
				InsertAccountList(userdata,Email);

			}
			else if (ptr->AccCount > 1)
			{
				//第一因為是 group 所以第一個acount 不放入
				//ex: it@softworking.com (group) , kenny@softworking.com (account)
				    AccountList *aptr = ptr->AList->next;
					while (aptr)
					{	  
						InsertAccountList(userdata,aptr->EMail); 

						AccountList *nextptr = aptr->next;						 
						aptr =  nextptr;
					}

			
			}


		
		}
		
		AllAccountList *nextptr = ptr->next;
		ptr =  nextptr;
	}

}

void CAntiSpamServer::DeleteAccountList(SMTPData *userdata,char* Account)
{

	AccountList *ptr = userdata->AccList;
	while (ptr)
	{	  
	    
		if (strcmp(ptr->Account,Account) == 0)
		{

			 if (ptr->prev)
				ptr->prev->next = ptr->next;
			 if (ptr->next)
				ptr->next->prev = ptr->prev;

			 if (userdata->AccList == ptr)
				userdata->AccList = ptr->next;

				delete ptr;

				return;
		}
		ptr =   ptr->next;
	}

	userdata->AccCount--;

}

void CAntiSpamServer::DestoryAccountList(AccountList *AList)
{

	AccountList *ptr = AList;
	while (ptr)
	{	  
	     
		AccountList *nextptr = ptr->next;

		delete ptr;

		ptr =  nextptr;
	}

}

void  CAntiSpamServer::DestoryAccountList(SMTPData *userdata)
{

	AccountList *ptr = userdata->AccList;
	while (ptr)
	{	  
	     
		AccountList *nextptr = ptr->next;

		delete ptr;

		userdata->AccCount--;

		ptr =  nextptr;
	}
		
	
	 

}



AllAccountList *CAntiSpamServer::InsertAllAccountList(char *Email)
{
	AllAccountList *end=NULL, *ptr=NULL;

	AllAccountList  *obj = new AllAccountList;
	memset(obj,0,sizeof(AllAccountList));


	obj->AccCount++;
	InsertAccountList(&obj->AList,Email);

	ptr = AllAccount;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        AllAccount = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	return obj;

}
void CAntiSpamServer::DestoryAllAccountList()
{
	AllAccountList *ptr = AllAccount;
	while (ptr)
	{	  
	    
		DestoryAccountList(ptr->AList);
		AllAccountList *nextptr = ptr->next;
		delete ptr;	

		ptr =  nextptr;
	}
		
	

}
/*void CAntiSpamServer::InsertAccountListFile(AccountList **AList,char* Email,char *MailBoxAccount,char *PassWord)
{
	AccountList *end=NULL, 
               *ptr=NULL;

	AccountList  *obj = new AccountList;
	memset(obj,0,sizeof(AccountList));

	strcpy(obj->EMail,Email);
	GetAccount(Email);
	strcpy(obj->Account,Email);
	strcpy(obj->MailBoxAccount,MailBoxAccount);
	strcpy(obj->PassWord,PassWord);

	MailServerDB->AddAccount(obj->Account);
	obj->DBAccountId = MailServerDB->GetAccountId(obj->Account);

	
    // Find the end of the list
    ptr = *AList;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        *AList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }



}*/

void CAntiSpamServer::InsertAccountList(AccountList **AList,char* Email)
{
	AccountList *end=NULL, 
               *ptr=NULL;

	AccountList  *obj = new AccountList;
	memset(obj,0,sizeof(AccountList));

	strcpy(obj->EMail,Email);
//	GetAccount(Email);
	strcpy(obj->Account,Email);
	

	MailServerDB->AddAccount(obj->Account);
	obj->DBAccountId = MailServerDB->GetAccountId(obj->Account);

	
    // Find the end of the list
    ptr = *AList;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        *AList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }



}
void CAntiSpamServer::InsertAccountList(SMTPData *userdata,char* Email)
{
	
	

	AccountList *end=NULL, 
               *ptr=NULL;

	AccountList  *obj = new AccountList;
	memset(obj,0,sizeof(AccountList));

	strcpy(obj->EMail,Email);
	//GetAccount(Email);
	strcpy(obj->Account,Email);

	obj->DBAccountId = MailServerDB->GetAccountId(obj->Account);

	
    // Find the end of the list
    ptr = userdata->AccList;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        userdata->AccList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	userdata->AccCount++;

}

/*
void CAntiSpamServer::InsertAccountListFile(SMTPData *userdata,char* Email,char *MailBoxAccount,char *PassWord)
{
	AccountList *end=NULL, 
               *ptr=NULL;

	AccountList  *obj = new AccountList;
	memset(obj,0,sizeof(AccountList));

	strcpy(obj->EMail,Email);
	GetAccount(Email);
	strcpy(obj->Account,Email);
	strcpy(obj->MailBoxAccount,MailBoxAccount);
	strcpy(obj->PassWord,PassWord);


	obj->DBAccountId = MailServerDB->GetAccountId(obj->Account);

	
    // Find the end of the list
    ptr = userdata->AccList;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        userdata->AccList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	userdata->AccCount++;


}*/

void CAntiSpamServer::GetIPZone(char* Ip , IPZone *AIpZone)
{

		memset(AIpZone,0,sizeof(IPZone));

	    int len = strlen(Ip);
	
	    int ZoneIdx = 0;
		int SavePos = 0;

		for(int i= 0 ; i < len ; i++)
		{
			if (Ip[i] == '.')
			{
			
				if (ZoneIdx == 0)
				{					
					strncpy(AIpZone->Zone1,Ip + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 1)
				{
					strncpy(AIpZone->Zone2,Ip + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 2)
				{
					strncpy(AIpZone->Zone3,Ip + SavePos, i - SavePos);
					strncpy(AIpZone->Zone4,Ip + i + 1, len-i);
					 
					break;
				}
			 

				ZoneIdx ++;
				SavePos = i + 1;

			}
		}

}

void CAntiSpamServer::GetDomainIdent(char* URL , char* DomainIdent)
{
 static char *DomainList[] =
  {".com.tw", //twnic zone
   ".net.tw",
   ".org.tw",
   ".gov.tw",
   ".idv.tw",
   ".game.tw",
   ".ebiz.tw",
   ".club.tw",
   ".edu.tw",
   ".tw",
   ".com.nz", //networksolution
   ".co.uk",
   ".com",
   ".net",
   ".org",
   ".info",
   ".biz",
   ".tv",
   ".us",
   ".cc",
   ".name",
   ".biz",
   ".de",
   ".be",
   ".at",
   ".co.jp", //jpnic
   ".ad.jp",
   ".ac.jp",
   ".go.jp",
   ".or.jp",
   ".ne.jp",
   ".gr.jp",
   ".ed.jp",
   ".lg.jp",
   ".jp",
   ".com.cn",//cnnic
   ".ac.cn",
   ".org.cn",
   ".edu.cn",
   ".net.cn",
   ".adm.cn",
   ".gov.cn",
   ".cn",
   ".com.hk",//hknic
   ".net.hk",
   ".org.hk",
   ".edu.hk",
   ".gov.hk",
   ".idv.hk",
   ".hk",
   ".co.kr",//krnic
   ".ac.kr",
   ".cc.kr",
   ".go.kr",
   ".ne.kr",
   ".cr.kr",
   ".re.kr",
   ".pe.kr",
   ".mil.kr",
   ".kr"};

   if (URL  == NULL || DomainIdent == NULL) return ;
   

   int DomainSize = sizeof(DomainList) / sizeof(DomainList[0]);

   int len = strlen(URL);
   
   char *revstr = _strrev(_strdup(URL));

   for (int i=0 ; i < DomainSize ; i ++)
   {
	   	char *DomainRevStr = _strrev(_strdup(DomainList[i]));
		int DomainLen = strlen(DomainList[i]);
	    
		if (strnicmp(DomainRevStr,revstr,DomainLen) == 0)
		{
			free(DomainRevStr);
			free(revstr);

			//求第二個 . 
			for (int j = (len - DomainLen -1) ; j >= 0 ; j --)
			{
				if (URL[j] == '.' )
				{
					int ResLen = len - j - DomainLen ;
					if (ResLen > 0)
					{
						strncpy(DomainIdent,URL+j+1, ResLen);
						DomainIdent[ResLen-1] = 0;
					}

					break;
				} 
				else if (j == 0)
				{
				

					int ResLen = len - DomainLen ;
					if (ResLen > 0)
					{
						strncpy(DomainIdent,URL, ResLen);
						DomainIdent[ResLen] = 0;
					}

				}
			}


		
			return;
		}

		free(DomainRevStr);
   }

   free(revstr);


}

bool  CAntiSpamServer::CheckFromIp(_TCHAR* EmailDomain , char* RemoteIp,char* PTR)
{

	    IPZone AIpZone;

		char CmpIpStr[16]={0};		
		char CmpDomainIpStr[16]={0};
		char MXDomainIpStr[16]={0};
		char ADomainIpStr[16]={0};
		char DomainIpStr[16]={0};


		GetIPZone(RemoteIp,&AIpZone);

		strcpy(CmpIpStr,AIpZone.Zone1);
		strcat(CmpIpStr,".");
		strcat(CmpIpStr,AIpZone.Zone2);
		strcat(CmpIpStr,".");
		strcat(CmpIpStr,AIpZone.Zone3);


		int rc = NO_ERROR;
	
		HANDLE handle;
		handle  = dns->Resolve(Settings.DnsIp,EmailDomain,qtMX,MXDomainIpStr,&rc);	 	
		WaitForSingleObject(handle,2 * 1000);	
		CloseHandle(handle);

		_cprintf("MX DomainIpStr : %s => %s\n",EmailDomain,MXDomainIpStr);


		if (strlen(MXDomainIpStr) > 0)
		{
			IPZone DnsIpZone;
			GetIPZone(MXDomainIpStr,&DnsIpZone);

			strcpy(CmpDomainIpStr,DnsIpZone.Zone1);
			strcat(CmpDomainIpStr,".");
			strcat(CmpDomainIpStr,DnsIpZone.Zone2);
			strcat(CmpDomainIpStr,".");
			strcat(CmpDomainIpStr,DnsIpZone.Zone3);


			_cprintf("cmp: %s <-> %s\n",CmpDomainIpStr,CmpIpStr);

			if (strcmp(CmpDomainIpStr , CmpIpStr) == 0)
			{
				return true;			
			}
		
		
		}
		
	
		//找 qtA
		handle  = dns->Resolve(Settings.DnsIp,EmailDomain,qtA,ADomainIpStr,&rc);	 	
		WaitForSingleObject(handle,2 * 1000);	
		CloseHandle(handle);

		_cprintf("A DomainIpStr : %s => %s\n",EmailDomain,ADomainIpStr);


		if (strlen(ADomainIpStr) > 0)
		{
			IPZone DnsIpZone;
			GetIPZone(ADomainIpStr,&DnsIpZone);

			strcpy(CmpDomainIpStr,DnsIpZone.Zone1);
			strcat(CmpDomainIpStr,".");
			strcat(CmpDomainIpStr,DnsIpZone.Zone2);
			strcat(CmpDomainIpStr,".");
			strcat(CmpDomainIpStr,DnsIpZone.Zone3);


			_cprintf("cmp: %s <-> %s\n",CmpDomainIpStr,CmpIpStr);

			if (strcmp(CmpDomainIpStr , CmpIpStr) == 0)
			{
				return true;			
			}
		
		
		}

		if (PTR != NULL && (ADomainIpStr[0] != 0 || MXDomainIpStr[0] != 0))
		{
			char MXPTRStr[255]={0};
			
			if (MXDomainIpStr[0] != 0)
			{
				strcpy(DomainIpStr,MXDomainIpStr);
			}
			else
			{
			    strcpy(DomainIpStr,ADomainIpStr);
			}
				
			   //找 qtMx or atA 的 PTR
				handle  = dns->Resolve(Settings.DnsIp,DomainIpStr,qtPTR,MXPTRStr,&rc);	 	
				WaitForSingleObject(handle,2 * 1000);	
				CloseHandle(handle);

				_cprintf("MX A DomainIpStr : %s => %s\n",DomainIpStr,MXPTRStr);


				if (strlen(MXPTRStr) > 0)
				{
					char PTRDomainIdent[255]={0};
					char EmailDomainIdent[255]={0};

					GetDomainIdent(PTR , PTRDomainIdent);
					GetDomainIdent(MXPTRStr , EmailDomainIdent);

						if (PTRDomainIdent[0] != 0 && EmailDomainIdent[0] != 0)
						{
							if (stricmp(PTRDomainIdent,EmailDomainIdent) == 0)
							{
								return true;
							}
						
						}	
				}
		}





	  
	
		
return false;		


}

int CAntiSpamServer::CheckPersonalIP(unsigned int AccountId ,char *IP)
{
	MailServerDB->CheckPersonalIp(AccountId,IP);
	return Personal_None;
}


int CAntiSpamServer::CheckRBL(char *IP)
{

	if (Settings.RBL1[0] != 0)
	{
			HANDLE handle;
			char Result[255];
			char Rip[16];
			char DnsQuery[255];

			strcpy(Rip,IP); 
			CCoreSocket::IpReverse(Rip);

			strcpy(DnsQuery,Rip);
			strcat(DnsQuery,".");
			strcat(DnsQuery,Settings.RBL1);

			
			memset(Result,0,255);
 
			int rc = NO_ERROR;
			//try
			//{
			    handle  = dns->Resolve(Settings.DnsIp,DnsQuery,qtA,Result,&rc);	 
			//}
			//catch(...)
			//{
				//RSet Componet
			//	delete dns;
				//dns = new CDnsClient();

				//handle  = dns->Resolve(Settings.DnsIp,DnsQuery,qtA,Result,&rc);	 
				
			//}
			DWORD dwWaitResult = WaitForSingleObject(handle,5 * 1000);
			CloseHandle(handle);

			if (dwWaitResult == WAIT_OBJECT_0 )
			{
				_cprintf("RBL1: %s: %s \n",Settings.RBL1,Result);

				
				if (Result[0] == 0)
				{
					char DnsQuery2[255];
					//smtpdata->SpamConnection = false;

					//再對 spamhaus 查詢
					strcpy(DnsQuery2,Rip);
					strcat(DnsQuery2,".");
					strcat(DnsQuery2,Settings.RBL2);
					Result[0] = 0;
					
					int rc = NO_ERROR;
					//try
					//{
					handle  = dns->Resolve(Settings.DnsIp,DnsQuery2,qtA,Result,&rc);
					//}
					//catch(...)
					//{
						//RSet Componet
						//delete dns;
						//dns = new CDnsClient();

						//handle  = dns->Resolve(Settings.DnsIp,DnsQuery2,qtA,Result,&rc);
				
					
					//}
                    
					if(WaitForSingleObject(handle,5 * 1000) == WAIT_OBJECT_0)
					{
						CloseHandle(handle);
						_cprintf("RBL2: %s: %s \n",Settings.RBL2,Result);
						if (Result[0] != 0)
						{
								//Report To Server
								char TempFile[_MAX_PATH];
								char SendStr[255];

								TempFile[0] = 0;

								strcpy(SendStr,"//ServerReportSpam.asp?IP=");
								strcat(SendStr,IP);

								int rc = NO_ERROR;
								HANDLE hd = http.IPGET(Settings.SpamDogCenterIp,"www.softworking.com",SendStr,TempFile,&rc);
								WaitForSingleObject(hd,10 * 1000);
								CloseHandle(hd);

								if (TempFile[0] != 0)
									DeleteFile(TempFile);
							
							return SOCKET_ERROR;
						}
					}
					else
					{
						CloseHandle(handle);
					
					}

					

					//}
					//else
					//{
						//CloseHandle(handle);
					//}



					return NO_ERROR;
				}
				else
				{
				
					//CloseHandle(handle);
					//smtpdata->SpamConnection = true;
					return SOCKET_ERROR;
				}

			}

		 
	}

	return NO_ERROR;
}

void CAntiSpamServer::ProcessMailList(SOCKET_OBJ* sock)
{

	SMTPData *userdata = (SMTPData *) sock->UserData;		
	
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
		
	_splitpath(userdata->TempFileName, drive, dir, fname, ext );
	dir[strlen(dir)-strlen("Incoming")-1] = 0; //"Incoming" size

	char MailListDataPath[MAX_PATH];

	//mail box MailList Data File
	strcpy(MailListDataPath,drive);
	strcat(MailListDataPath,dir);
	strcat(MailListDataPath,"Mail\\");
	strcat(MailListDataPath,userdata->RcptTo);
	strcat(MailListDataPath,"\\MailList.dat");	
	



	//check if file exist
	FILE *fp = NULL;
	MailListHeader mHeader;

	memset(&mHeader,0,sizeof(mHeader));
	

	/*WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(MailListDataPath, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
		fp = fopen(MailListDataPath,"w+b");	
		time(&mHeader.LastUsedTime);
		fwrite(&mHeader,1,sizeof(mHeader),fp);
		
	}
	else
	{
		FindClose(hFind);
		fp = fopen(MailListDataPath,"r+b");
		fread(&mHeader,1,sizeof(mHeader),fp);
	}		*/

	fp = fopen(MailListDataPath,"r+b");
	if (fp == NULL)
	{
	
		fp = fopen(MailListDataPath,"w+b");	
		time(&mHeader.LastUsedTime);
		fwrite(&mHeader,1,sizeof(mHeader),fp);
		

	
	}
	else
	{	 
	   //fp = fopen(MailListDataPath,"r+b");
	   fread(&mHeader,1,sizeof(mHeader),fp);	
	 }
	
	 

	//fseek(fp,0,SEEK_END);
	//fwrite(&userdata->MailList,1,sizeof(userdata->MailList),fp);
	


	fseek(fp,0,SEEK_SET);
	time(&mHeader.LastUsedTime);
	mHeader.MailCount++;
	fwrite(&mHeader,1,sizeof(mHeader),fp);

	
	if(mHeader.MailCount >= Settings.MailNotifyNum)
	{
	
		char RelayMailPath[MAX_PATH];
		char TempFileName[MAX_PATH];

		strcpy(RelayMailPath,drive);
		strcat(RelayMailPath,dir);
		strcat(RelayMailPath,"\\Relay\\");

		//Send Notify Mail
		GetTempFileName(RelayMailPath, // directory for temp files 
				"Mail",                    // temp file name prefix 
				0,                        // create unique name 
				TempFileName);              // buffer for name 		
			
		FILE *fp2 = fopen(TempFileName,"r+b");

		  //list all mail
		fseek(fp,sizeof(mHeader),SEEK_SET);
		

		MailListData listdata;
				
		bool NeedSend = false;


		while (fread(&listdata,1,sizeof(listdata),fp) == sizeof(listdata))	
		{
		
		
				if (!listdata.SendNotify)
				{
					struct tm *newtime;
					newtime = localtime( &listdata.ReceiveTime );   

					if (!NeedSend)
					{
						//寫入mail header
						fprintf(fp2,"From: admin<%s>\r\nTo: <%s>\r\nSubject: SpamDog AntiSpam Server Blocking Confirm\r\n\r\n",Settings.AdminEmail,userdata->RcptTo);
					}

					fprintf(fp2,"Status: %d , Mail From:  <%s> , Ip: %s , Time: %s\r\n",listdata.Status , listdata.MailFrom,listdata.Ip,asctime(newtime));
				
					//更新 status				 
					listdata.SendNotify =true;

					fseek(fp,(int) sizeof(listdata) * -1,SEEK_CUR); //移回更新
					fwrite(&listdata,1,sizeof(listdata),fp);
					

					fflush(fp);
					//fseek(fp,sizeof(listdata),SEEK_CUR); //移回
				

					NeedSend = true;				


				}
		
		
		}

		fclose(fp2);

		if (NeedSend)
		{
			//更新 mail count
			fseek(fp,0,SEEK_SET);
			time(&mHeader.LastUsedTime);
			mHeader.MailCount = 0;
			fwrite(&mHeader,1,sizeof(mHeader),fp);

			//create realy dat file
			char RelayDataPath[MAX_PATH];
				
			strcpy(RelayDataPath,TempFileName);
			strcat(RelayDataPath,".dat");

			FILE *fp3 = fopen(RelayDataPath,"w+b");

			  fprintf(fp3,"<%s>\r\n",Settings.AdminEmail);
			  fprintf(fp3,"<%s>\r\n",userdata->RcptTo);

			fclose(fp3);

			//rename TempFile for send
			char RenameToMail[MAX_PATH];
			strcpy(RenameToMail,TempFileName);
			strcat(RenameToMail,".eml");

			MoveFile(TempFileName,RenameToMail);

			SetEvent(RelayMailEvent);

		
		}
		else
		{
			DeleteFile(TempFileName);
		}


	}


	fclose(fp);




}

bool CAntiSpamServer::VerifyAccount(char* AD_IP, char* Account,char* Password)
{

		   

			wchar_t AdQuery[255 * 2];
			wcscpy(AdQuery,L"LDAP://");

			wchar_t IPW[16*2];
			MultiByteToWideChar(CP_ACP,0,AD_IP,strlen(AD_IP)+1,IPW,strlen(AD_IP)+1);
			wcscat(AdQuery,IPW);

			HRESULT hr;

		
			wchar_t AccountW[255 * 2];
			wchar_t PasswordW[255 * 2];

		
			MultiByteToWideChar(CP_ACP,0,Account,strlen(Account)+1,AccountW,strlen(Account)+1);
			MultiByteToWideChar(CP_ACP,0,Password,strlen(Password)+1,PasswordW,strlen(Password)+1);
			

			IDirectorySearch*   pIDs;

			hr = ADsOpenObject(AdQuery,AccountW,PasswordW,NULL,
					   IID_IDirectorySearch, (void**)&pIDs);

			if (FAILED(hr))  
			{
			 
				return false;
			}
			else
			{
				return true;
			}

			/*

			try
			{
	
					_ConnectionPtr mConnection ; 
					_CommandPtr mCommand ;

					char AdoConnStr[255];
					//strcpy(AdoConnStr,"Provider=MSDASQL.1;Password=dog1234;Persist Security Info=True;User ID=dog;Data Source=SpamDog2;Initial Catalog=SpamDog2");
					strcpy(AdoConnStr,"Provider=ADsDSOObject");


					HRESULT hr;
					hr = mConnection.CreateInstance(__uuidof(Connection));	
					if(SUCCEEDED(hr))
					{			
						hr = mConnection->Open(AdoConnStr,Account,Password,adModeUnknown);			
						if(SUCCEEDED(hr))
						{
							mCommand.CreateInstance(__uuidof(Command));
							mCommand->ActiveConnection = mConnection;
							
							char ADStr[255];

							strcpy(ADStr,"<LDAP://");
							strcat(ADStr,AD_IP);
							strcat(ADStr,">;(&(objectCategory=User)(sAMAccountName=administrator));samAccountName");				 

							mCommand->CommandText  = ADStr;
							mCommand->CommandType = adCmdText;

							
								_RecordsetPtr mRecordset = mCommand->Execute(NULL,NULL,adCmdText);

								if(!mRecordset->EndOfFile)
								{						
									Result =  true;
								}
								else
								{
									Result = false;
								}
						
							
						}
					}

				}
				catch(...)
				{
					
						Result = false;
				}*/

			


}

/*
void CAntiSpamServer::GetAccount(_TCHAR* Email)
{
	_TCHAR *findpos = _tcsstr(Email,"@");

	if (findpos != NULL)
	{
	
		*findpos = 0;
	
	}


}*/

void CAntiSpamServer::GetCN(char* AdsPath)
{


	char *findpos = strstr(AdsPath,"CN=");
	if (findpos != NULL)
	{
		char* findpos2 = strstr(findpos+3,",");

		if(findpos2 != NULL)
		{
		
			memmove(AdsPath,findpos+3,findpos2 - findpos - 3);
			AdsPath[findpos2 - findpos - 3] =  0;
		}
		else
		{
			memmove(AdsPath,findpos+3,strlen(findpos) - 3);
			AdsPath[strlen(findpos) - 3] =  0;
		}

	
	}

}


void CAntiSpamServer::GetAccountFromLogin(char* AD_IP,char* LoginId,char* UserId,char* Password,char* Account,int BufferSize)
{
	HRESULT hr;

	wchar_t AdQuery[255 * 2];
	wcscpy(AdQuery,L"LDAP://");

	wchar_t IPW[16*2];
	MultiByteToWideChar(CP_ACP,0,AD_IP,strlen(AD_IP)+1,IPW,strlen(AD_IP)+1);
	wcscat(AdQuery,IPW);
		

	wchar_t LoginIdW[255 * 2];
	wchar_t UserIdW[255 * 2];
	wchar_t PasswordW[255 * 2];

	MultiByteToWideChar(CP_ACP,0,LoginId,strlen(LoginId)+1,LoginIdW,strlen(LoginId)+1);
	MultiByteToWideChar(CP_ACP,0,UserId,strlen(UserId)+1,UserIdW,strlen(UserId)+1);
	MultiByteToWideChar(CP_ACP,0,Password,strlen(Password)+1,PasswordW,strlen(Password)+1);

 
 
	IDirectorySearch *pSearch;

	///////////////////////////////////////////////
	// Bind to Object, it serves as a base search
	///////////////////////////////////////////////

	hr = ADsOpenObject(AdQuery,UserIdW,PasswordW,NULL,
					   IID_IDirectorySearch, (void**)&pSearch);

	if ( !SUCCEEDED(hr) )
	{
		return ;
	}




	LPWSTR pszAttr[] = { L"mail"   };
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount= sizeof(pszAttr)/sizeof(LPWSTR);

 
	ADS_SEARCH_COLUMN col;
 

 

	//(objectCategory=person)
	wchar_t strFilter[255 * 2];	
	wcscpy(strFilter,L"(&(objectCategory=person)(sAMAccountName=");
	wcscat(strFilter,LoginIdW);
	wcscat(strFilter,L"))");


	hr = pSearch->ExecuteSearch(strFilter, pszAttr, dwCount, &hSearch );

	if ( !SUCCEEDED(hr) )
	{	
		return ;
	}

	if (pSearch->GetNextRow(hSearch)  != S_ADS_NOMORE_ROWS)
	{
		hr = pSearch->GetColumn( hSearch, pszAttr[0], &col );
		if ( SUCCEEDED(hr) )
		{
		 
			for(DWORD x = 0; x < col.dwNumValues; x++)
            {		   
						WideCharToMultiByte(CP_ACP,0,col.pADsValues[x].CaseIgnoreString,wcslen(col.pADsValues[x].CaseIgnoreString)+1,Account,BufferSize,NULL,NULL);
						
						
						//_TCHAR* findpos = _tcsstr(Account,"@");
						//if (findpos != NULL)
						//	*findpos = 0;

						//GetCN(Account);
						break;
						//printf("%s\n", Mail);
			
            }


			pSearch->FreeColumn( &col ); // You need to FreeColum after use.
		}

	
	}

	pSearch->CloseSearchHandle(hSearch);
}

void CAntiSpamServer::GetMailFromCN(IDirectorySearch *pSearch,char *CN, char* Mail, int BufferSize)
{

	GetCN(CN);

	HRESULT hr;
	LPWSTR pszAttr[] = { L"mail"   };
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount= sizeof(pszAttr)/sizeof(LPWSTR);

	wchar_t CNW[255 * 2];
	ADS_SEARCH_COLUMN col;
 

	MultiByteToWideChar(CP_ACP,0,CN,strlen(CN)+1,CNW,255 * 2);

	//(objectCategory=person)
	wchar_t strFilter[255 * 2];	
	wcscpy(strFilter,L"(&(cn=");
	wcscat(strFilter,CNW);
	wcscat(strFilter,L")(sAMAccountName=*))");


	hr = pSearch->ExecuteSearch(strFilter, pszAttr, dwCount, &hSearch );

	if ( !SUCCEEDED(hr) )
	{	
		return ;
	}

	if (pSearch->GetNextRow(hSearch)  != S_ADS_NOMORE_ROWS)
	{
		hr = pSearch->GetColumn( hSearch, pszAttr[0], &col );
		if ( SUCCEEDED(hr) )
		{
		 
			for(DWORD x = 0; x < col.dwNumValues; x++)
            {		   
						WideCharToMultiByte(CP_ACP,0,col.pADsValues[x].CaseIgnoreString,wcslen(col.pADsValues[x].CaseIgnoreString)+1,Mail,BufferSize,NULL,NULL);
						//printf("%s\n", Mail);
			
            }


			pSearch->FreeColumn( &col ); // You need to FreeColum after use.
		}

	
	}

	pSearch->CloseSearchHandle(hSearch);
}


void CAntiSpamServer::GetADAccList(char* AD_IP,char* Email,char* UserId,char* Password,SMTPData *userdata)
{
	//CoInitialize(NULL);		
	
	wchar_t AdQuery[255 * 2];
	wcscpy(AdQuery,L"LDAP://");

	wchar_t IPW[16*2];
	MultiByteToWideChar(CP_ACP,0,AD_IP,strlen(AD_IP)+1,IPW,strlen(AD_IP)+1);
	wcscat(AdQuery,IPW);
		

	wchar_t EMailW[255 * 2];
	wchar_t UserIdW[255 * 2];
	wchar_t PasswordW[255 * 2];

	MultiByteToWideChar(CP_ACP,0,Email,strlen(Email)+1,EMailW,strlen(Email)+1);
	MultiByteToWideChar(CP_ACP,0,UserId,strlen(UserId)+1,UserIdW,strlen(UserId)+1);
	MultiByteToWideChar(CP_ACP,0,Password,strlen(Password)+1,PasswordW,strlen(Password)+1);

//	char QueryDomain[255];
	//char *DomainIdx = strstr(Email,"@");
	///strcpy(QueryDomain,DomainIdx+1);


	HRESULT hr;
	IDirectorySearch *pSearch;

	///////////////////////////////////////////////
	// Bind to Object, it serves as a base search
	///////////////////////////////////////////////

	hr = ADsOpenObject(AdQuery,UserIdW,PasswordW,NULL,
					   IID_IDirectorySearch, (void**)&pSearch);

	if ( !SUCCEEDED(hr) )
	{
		return ;
	}






	////////////////////////////////////
	// Prepared for attributed returned
	////////////////////////////////////
	//LPWSTR pszAttr[] = { L"altRecipient"};
	//LPWSTR pszAttr[] = { L"member" };
	LPWSTR pszAttr[] = { L"member"  };
	//LPWSTR pszAttr[] = { L"deliverAndRedirect"};
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount= sizeof(pszAttr)/sizeof(LPWSTR);

	wchar_t strFilter[255 * 2];	
	wcscpy(strFilter,L"(&(objectCategory=Group)(mail=");
	wcscat(strFilter,EMailW);
	wcscat(strFilter,L"))");


	//////////////////////////////////////////
	// Search for all groups in a domain
	/////////////////////////////////////////////
	//hr = pSearch->ExecuteSearch(L"(objectCategory=Group)(mail=it@ezfly.com)", pszAttr, dwCount, &hSearch );

	
	hr = pSearch->ExecuteSearch(strFilter, pszAttr, dwCount, &hSearch );

	//hr = pSearch->ExecuteSearch(L"(&(objectCategory=person)(memberOf=it))", pszAttr, dwCount, &hSearch );


	if ( !SUCCEEDED(hr) )
	{
		pSearch->Release();
		return ;
	}

	//////////////////////////////////////////
	// Now enumerate the result
	/////////////////////////////////////////////
	ADS_SEARCH_COLUMN col;
	bool bGroup = false;
	bool AccFound = false;

    while(  pSearch->GetNextRow(hSearch)  != S_ADS_NOMORE_ROWS )
    {
		bGroup = true;
		AccFound = true;
		// Get 'Name' attribute
		hr = pSearch->GetColumn( hSearch, pszAttr[0], &col );
		if ( SUCCEEDED(hr) )
		{
			//printf("%S\n", col.pADsValues->CaseIgnoreString);

			for(DWORD x = 0; x < col.dwNumValues; x++)
            {
                       //wprintf(col.pADsValues[x].CaseIgnoreString); 
                       //printf("%S\n", col.pADsValues[x].CaseIgnoreString);

					    char CN[255];
						WideCharToMultiByte(CP_ACP,0,col.pADsValues[x].CaseIgnoreString,wcslen(col.pADsValues[x].CaseIgnoreString)+1,CN,sizeof(CN),NULL,NULL);
						
						
						//取 mail
						char mEMail[255] = {0};
						GetMailFromCN(pSearch,CN,mEMail,255);

						//查 fw
						GetADAccList(AD_IP,mEMail,UserId,Password,userdata);
					
						
            }


			pSearch->FreeColumn( &col ); // You need to FreeColum after use.
		}

      
    }

	////////////////////
	// Clean-up
	////////////////////////
    pSearch->CloseSearchHandle(hSearch);


	if (!bGroup)
	{
		//query User sAMAccountName adspath
		//LPWSTR pszAttr[] = { L"DistinguishedName" , L"altRecipient" , L"deliverAndRedirect"};
		LPWSTR pszAttr[] = {   L"altRecipient" , L"deliverAndRedirect" , L"sAMAccountName" };
		
		ADS_SEARCH_HANDLE hSearch;
		DWORD dwCount= sizeof(pszAttr)/sizeof(LPWSTR);

		wcscpy(strFilter,L"(&(objectCategory=person)(mail=");
		wcscat(strFilter,EMailW);
		wcscat(strFilter,L"))");


		hr = pSearch->ExecuteSearch(strFilter, pszAttr, dwCount, &hSearch );

		if ( !SUCCEEDED(hr) )
		{
			pSearch->Release();
			return ;
		}

		//////////////////////////////////////////
		// Now enumerate the result
		/////////////////////////////////////////////
		ADS_SEARCH_COLUMN col;
		bool NeedToSelf = false;
		bool HasFW = false;
		
		

		while(  pSearch->GetNextRow(hSearch)  != S_ADS_NOMORE_ROWS )
		{		
		
			hr = pSearch->GetColumn( hSearch, pszAttr[2], &col );
			if ( SUCCEEDED(hr) )
			{
				//HasFW = true;
				//printf("%S\n", col.pADsValues->CaseIgnoreString);
 
					if (col.dwNumValues > 0 )
					AccFound = true;
				 
			}

			// Get 'Name' attribute
			hr = pSearch->GetColumn( hSearch, pszAttr[1], &col );
			if ( SUCCEEDED(hr) )
			{
				//HasFW = true;
				//printf("%S\n", col.pADsValues->CaseIgnoreString);

				for(DWORD x = 0; x < col.dwNumValues; x++)
				{
					
					
					//wprintf(col.pADsValues[x].CaseIgnoreString); 
					if (col.dwADsType == ADSTYPE_BOOLEAN)
					{
						if( col.pADsValues[x].Boolean)
							NeedToSelf = true;
					}

					AccFound = true;
					
				 
				}
			}

			

				hr = pSearch->GetColumn( hSearch, pszAttr[0], &col );
				if ( SUCCEEDED(hr) )
				{
					//printf("%S\n", col.pADsValues->CaseIgnoreString);
					HasFW = true;

					for(DWORD x = 0; x < col.dwNumValues; x++)
					{
					 AccFound = true;
						
						if (col.dwADsType == ADSTYPE_DN_STRING ) 
						{
							//printf("%S\n", col.pADsValues[x].CaseIgnoreString);
							//Convert to ansi
							char CN[255];
							WideCharToMultiByte(CP_ACP,0,col.pADsValues[x].CaseIgnoreString,wcslen(col.pADsValues[x].CaseIgnoreString)+1,CN,sizeof(CN),NULL,NULL);
							//printf("%s\n", CN);

							char mEMail[255] = {0};
							GetMailFromCN(pSearch,CN,mEMail,255);

							//查 fw
							GetADAccList(AD_IP,mEMail,UserId,Password,userdata);
							


						}
					}


					pSearch->FreeColumn( &col ); // You need to FreeColum after use.

			
				}
			 

      
		}

		if (AccFound == true && ((HasFW == true && NeedToSelf ==  true) || (HasFW == false)))
		{
			//AddToList
			char EmailAcc[255];
			strcpy(EmailAcc,Email);
			
			//GetAccount(EmailAcc);
			//printf("%s\n",EmailAcc);
			//自動建立帳號
			MailServerDB->AddAccount(EmailAcc);

			InsertAccountList(userdata,Email);
			
		} 
		 

		////////////////////
		// Clean-up
		////////////////////////
		pSearch->CloseSearchHandle(hSearch);



	
	
	}



	pSearch->Release();


	
}

void CAntiSpamServer::ReadAccountFile()
{

       char FilePath[MAX_PATH];	 	  
	   char TmpStr[1024];
	  

	   strcpy(FilePath,ServerDB->PGPath);	      
	   strcat(FilePath,"Account.ini");

	   FILE *fp = NULL;	   
	   fp = fopen(FilePath,"r+b");
	   if (fp == NULL)
	   {
			fp = fopen(FilePath,"w+b");
	   }

	   while (!feof(fp))
	   {
	   
		   unsigned AccountCount  = 0;
		   char EMailData[255]={0};
		   unsigned int FindIdx = 0;
		   AllAccountList *AList = NULL;


		   int ReadByte = fscanf(fp,"%s\r\n",TmpStr);
		   if (ReadByte <= 0 ) break;

		   int len = strlen(TmpStr);

		   for (int i = 0 ; i < len ; i ++)
		   {
			   if(TmpStr[i] == ':' || i == len -1)
			   {
				   AccountCount ++ ;
				   memset(EMailData,0,sizeof(EMailData));

				   if (i == len -1)
				   {
					strncpy(EMailData, TmpStr + FindIdx , i - FindIdx + 1);
				   }
				   else
				   {
				    strncpy(EMailData, TmpStr + FindIdx , i - FindIdx );
				   }

				   printf("%s\r\n",EMailData);

				   //解開 BoxName , PWD
				   char MailBoxAccount[120]={0};
				   char PassWord[50]={0};
				   char Account[50]={0};
				   
				   char *Pos1 = strstr(EMailData,",");
				   if (Pos1 != NULL)
				   {
					   strncpy(Account,EMailData, Pos1 - EMailData);
					   
					   char* Pos2 = strstr(Pos1+1,",");
					   if (Pos2 != NULL)
					   {
						   strncpy(MailBoxAccount,Pos1+1, Pos2 - Pos1 -1);
						   strcpy(PassWord,Pos2+1);
					   }
					   else
					   {
							break;
					   }
				   }
				   else
				   {
					   break;				   
				   }

				   
				   if (AccountCount == 1)
				   {
						AList = InsertAllAccountList(Account);
				   }
				   else
				   {
						InsertAccountList(&AList->AList,Account);
						AList->AccCount ++;						
				   }

				   

				   FindIdx = i + 1;
			   }
			   
		   }	   
	   }

	   fseek(fp,0,SEEK_SET);
	   fclose(fp);


}
 
void CAntiSpamServer::ToLowCase(char *AStr)
{
	char TmpStr[255] = {0};

	int templen = 0;
	int len = strlen(AStr);
	for (int i = 0 ; i < len ; i++)
	{
		
		  TmpStr[templen] =	tolower(AStr[i]);
		  templen++;
		

	}

	strcpy(AStr,TmpStr);

}

void CAntiSpamServer::SendMSG(char *Msg)
{
			struct tm *newtime;
			long ltime;

			time( &ltime );

			/* Obtain coordinated universal time: */
			newtime = gmtime( &ltime );
		 
			//char *TimeStr = asctime( newtime );
			//TimeStr[strlen(TimeStr) - 1] = 0; //trim /n
			char TimeStr[255];
			strftime(TimeStr,255,"%Y%m%d%H%M%S",newtime);

			char MsgPath[MAX_PATH];
			char OKPath[MAX_PATH];

			strcpy(MsgPath,"D:\\MSNMSG\\");
			strcat(MsgPath,TimeStr);
			strcpy(OKPath,MsgPath);

			strcat(MsgPath,".TXT");
			strcat(OKPath,".OK");			


			FILE *fp = fopen(MsgPath,"w+b");
			fwrite(Msg,1,strlen(Msg),fp);
			fclose(fp);

			fp = fopen(OKPath,"w+b");		 
			fclose(fp);

}

void CAntiSpamServer::ReadBlockIpList()
{

	   IpList *end=NULL, 
               *ptr=NULL;

	   char BlockIpPath[MAX_PATH];	 	  

	   strcpy(BlockIpPath,ServerDB->PGPath);	      
	   strcat(BlockIpPath,"BlockIp.ini");

	   FILE *fp = fopen(BlockIpPath,"r+b");	   
	   if (fp == NULL) return;

	   while( !feof(fp))
	   {
		   IpList *obj = new IpList;
		   memset(obj,0,sizeof(IpList));		  
		   
		   fscanf(fp,"%s\r\n",obj->Ip);
		   obj->Status = 1; //bad

		   //Insert Into Block Ip list
		    IpList *ptr = BlockIpList;
			if (ptr)
			{
				while (ptr->next)
					ptr = ptr->next;
				end = ptr;
			}

			obj->next = NULL;
			obj->prev = end;

			if (end == NULL)
			{
				// List is empty
				BlockIpList = obj;
			}
			else
			{
				// Put new object at the end 
				end->next = obj;
				obj->prev = end;
			}

		   

		   
	   
	   }

	   fclose(fp);
}


bool CAntiSpamServer::CheckBlockIp(char *Ip)
{
    
	
	IpList *ptr = BlockIpList;
	while (ptr)
	{	  
	  if (strncmp(Ip,ptr->Ip,strlen(ptr->Ip)) == 0 )
		   return true;


		ptr =  ptr->next;
	}

	return false;


}


bool CAntiSpamServer::CheckADExist(char* AD_IP,char* Email,char* UserId,char* Password)
{			
			
		    //bool Result = false;
	       
			
			wchar_t AdQuery[255 * 2];
			wcscpy(AdQuery,L"LDAP://");

			wchar_t IPW[16*2];
			MultiByteToWideChar(CP_ACP,0,AD_IP,strlen(AD_IP)+1,IPW,strlen(AD_IP)+1);
			wcscat(AdQuery,IPW);

			HRESULT hr;

			wchar_t EMailW[255 * 2];
			wchar_t UserIdW[255 * 2];
			wchar_t PasswordW[255 * 2];

			MultiByteToWideChar(CP_ACP,0,Email,strlen(Email)+1,EMailW,strlen(Email)+1);
			MultiByteToWideChar(CP_ACP,0,UserId,strlen(UserId)+1,UserIdW,strlen(UserId)+1);
			MultiByteToWideChar(CP_ACP,0,Password,strlen(Password)+1,PasswordW,strlen(Password)+1);
			

			IDirectorySearch*   pIDs;

			hr = ADsOpenObject(AdQuery,UserIdW,PasswordW,NULL,
					   IID_IDirectorySearch, (void**)&pIDs);

			if (FAILED(hr))  
			{
				_cprintf("AD Error\n");
				return false;
			}

			
			
			wchar_t strFilter[255 * 2];
			wcscpy(strFilter,L"(&(objectCategory=user)(mail=");
			wcscat(strFilter,EMailW);
			wcscat(strFilter,L"))");

			LPWSTR pszAttr[] = { L"samAccountName" };
			ADS_SEARCH_HANDLE hSearch;
			DWORD dwAttrNameSize = sizeof(pszAttr)/sizeof(LPWSTR);

			hr = pIDs->ExecuteSearch(strFilter, pszAttr ,dwAttrNameSize, &hSearch );
			if(!SUCCEEDED(hr))
			{
				_cprintf ("AD ExecuteSearch FAILED!\n");
				pIDs->CloseSearchHandle(hSearch);
				pIDs->Release();				
				return false;
			}

			hr = pIDs->GetNextRow(hSearch);
			if(!SUCCEEDED(hr) || hr == S_ADS_NOMORE_ROWS)
			{
				_cprintf ("AD S_ADS_NOMORE_ROWS!\n");
				pIDs->CloseSearchHandle(hSearch);
				pIDs->Release();
				return false;
			}
			else
			{
				pIDs->CloseSearchHandle(hSearch);
				pIDs->Release();
				return true;
			}
			

			







			/*try
			{
	
					_ConnectionPtr mConnection ; 
					_CommandPtr mCommand ;

					char AdoConnStr[255];
					//strcpy(AdoConnStr,"Provider=MSDASQL.1;Password=dog1234;Persist Security Info=True;User ID=dog;Data Source=SpamDog2;Initial Catalog=SpamDog2");
					strcpy(AdoConnStr,"Provider=ADsDSOObject");


					HRESULT hr;
					hr = mConnection.CreateInstance(__uuidof(Connection));	
					if(SUCCEEDED(hr))
					{			
						hr = mConnection->Open(AdoConnStr,UserId,Password,adModeUnknown);			
						if(SUCCEEDED(hr))
						{
							mCommand.CreateInstance(__uuidof(Command));
							mCommand->ActiveConnection = mConnection;
							
							char ADStr[255];

							strcpy(ADStr,"<LDAP://");
							strcat(ADStr,AD_IP);
							strcat(ADStr,">;(&(objectCategory=User)(mail=");
							strcat(ADStr,Email);
							strcat(ADStr,"));samAccountName,mail;subtree");

							//_cprintf("Ad: %s\r\n",ADStr);

							mCommand->CommandText  = ADStr;
							mCommand->CommandType = adCmdText;
							_RecordsetPtr mRecordset = mCommand->Execute(NULL,NULL,adCmdText);

							if(!mRecordset->EndOfFile)
							{
								//MsgBox objRecordset.Fields("samAccountName")
								Result =  true;
							}
							else
							{
								Result = false;
							}

							
							
							mConnection->Close();
							
						}					
				 
					}

			} catch(...)
			{
			
				_cprintf("Ad Check Error \r\n");
				Result = false;
			
			}*/
			

	
		//	return Result;

}

void CAntiSpamServer::IniLogFile()
{
	   long ltime;
	   time( &ltime );
	   struct tm *Filetime;
	   Filetime = localtime ( &ltime );		
	   
	   char TmpLogName[255];
	   strftime(TmpLogName,255,"%Y%m%d.log",Filetime);	

	   if (strcmp(TmpLogName ,LogName) == 0) return;

	   strcpy(LogName,TmpLogName);

       char LogPath[MAX_PATH];
	   strcpy(LogPath,ServerDB->PGPath);
	   strcat(LogPath,"Log");


	   strcat(LogPath,"\\");  
	   strcat(LogPath,LogName);

	   if (LogFp != NULL) fclose(LogFp);

	   LogFp = fopen(LogPath,"r+b");
	   if (LogFp == NULL)
	   {
				LogFp = fopen(LogPath,"w+b");
	   }

		

	   fseek(LogFp,0,SEEK_END);

}

void CAntiSpamServer::IniServer()
{
	   

	   char LogPath[MAX_PATH];
	   strcpy(LogPath,ServerDB->PGPath);
	   strcat(LogPath,"Log");
	   CreateDirectory(LogPath,NULL);
	   
	   IniLogFile();


	   char RelayPath[MAX_PATH];
	   strcpy(RelayPath,ServerDB->PGPath);
	   strcat(RelayPath,"Relay");
	   CreateDirectory(RelayPath,NULL);

	   char IncomingPath[MAX_PATH];
	   strcpy(IncomingPath,ServerDB->PGPath);
	   strcat(IncomingPath,"Incoming");
	   CreateDirectory(IncomingPath,NULL);

	   char MailPath[MAX_PATH];
	   strcpy(MailPath,ServerDB->PGPath);
	   strcat(MailPath,"Mail");
	   CreateDirectory(MailPath,NULL);

	
	
	   char IniPath[MAX_PATH];	 	  

	   strcpy(IniPath,ServerDB->PGPath);	      
	   strcat(IniPath,"SpamDogServer.ini");

	   
	   Settings.OkDelayResponse = GetPrivateProfileInt("ServerSetup","OkDelayResponse",0,IniPath); 
	   Settings.SpamDelayResponse = GetPrivateProfileInt("ServerSetup","SpamDelayResponse",0,IniPath);
	   Settings.LimitRetryTime = GetPrivateProfileInt("ServerSetup","LimitRetryTime",0,IniPath); 
	   Settings.LastRebuildDBTime = GetPrivateProfileInt("ServerSetup","LastRebuildDBTime",0,IniPath); 
	   Settings.MailNotifyNum = GetPrivateProfileInt("ServerSetup","MailNotifyNum",20,IniPath); 
	   Settings.MaxNoSpamSize = GetPrivateProfileInt("ServerSetup","MaxNoSpamSize",1024*10,IniPath); 
	   Settings.AccountCheckMode = GetPrivateProfileInt("ServerSetup","AccountCheck",1,IniPath);  //FILE mode be default
	   Settings.RelayPopMode = GetPrivateProfileInt("ServerSetup","RelayPopMode",0,IniPath);  
	  

	  

	   //處理 db renew
	   if (Settings.LastRebuildDBTime > 0)
	   {
	   
		   time_t LastTime(Settings.LastRebuildDBTime);
		   
		   time_t Now;
		   time(&Now);

		   if (difftime(Now,LastTime) > 60 * 60 * 24 * 7) // 1 week
		   {
				//ServerDB->OpenIpDB();
			    ServerDB->RenewDB();
				//ServerDB->CloseIpDB();

				char TimeStr[255];
				itoa(Now,TimeStr,10);

				WritePrivateProfileString("ServerSetup","LastRebuildDBTime",TimeStr,IniPath);
		   }
	   
	   
	   }
	   else
	   {
		   time_t Now;
		   time(&Now);

		   char TimeStr[255];
		   itoa(Now,TimeStr,10);

		   WritePrivateProfileString("ServerSetup","LastRebuildDBTime",TimeStr,IniPath);
				   
	   
	   }

	   GetPrivateProfileString("ServerSetup","ServerMode","S",Settings.ServerMode,sizeof(Settings.ServerMode),IniPath);
	   GetPrivateProfileString("ServerSetup","RBL1","",Settings.RBL1,sizeof(Settings.RBL1),IniPath);
	   GetPrivateProfileString("ServerSetup","RBL2","",Settings.RBL2,sizeof(Settings.RBL2),IniPath);
	   GetPrivateProfileString("ServerSetup","ServerName","",Settings.ServerName,sizeof(Settings.ServerName),IniPath);
	   GetPrivateProfileString("ServerSetup","RelayToServerIp","",Settings.RelayToServerIp,sizeof(Settings.RelayToServerIp),IniPath);
	   GetPrivateProfileString("ServerSetup","DnsIp","168.95.1.1",Settings.DnsIp,sizeof(Settings.DnsIp),IniPath);
	   GetPrivateProfileString("ServerSetup","ADServerIp","",Settings.ADServerIp,sizeof(Settings.ADServerIp),IniPath);
	   GetPrivateProfileString("ServerSetup","ADLoginId","",Settings.ADLoginId,sizeof(Settings.ADLoginId),IniPath);
	   GetPrivateProfileString("ServerSetup","ADPassWord","",Settings.ADPassWord,sizeof(Settings.ADPassWord),IniPath);
	   GetPrivateProfileString("ServerSetup","AdminEmail","",Settings.AdminEmail,sizeof(Settings.AdminEmail),IniPath);
	   

	   


	   GetPrivateProfileString("SysSetup","SpamDogCenterIp","114.32.69.18",Settings.SpamDogCenterIp,sizeof(Settings.SpamDogCenterIp),IniPath);
	   
	   
	   if (Settings.AccountCheckMode == 1 || Settings.ADServerIp[0] == 0)
	   {
			ReadAccountFile();
	   }
	   
	   ReadBlockIpList();

	  /* //登入 msn
	    LoginClient = new CMsnClient();

		HANDLE hand = LoginClient->MSNLogin("207.46.96.153",1863);
		WaitForSingleObject(hand , INFINITE);

		char NewIP[15];
		int Newport;

		if (LoginClient->LastStatus == MSN_NEED_RELOGIN)
		{
			strcpy(NewIP,LoginClient->XFRIP);
			Newport = LoginClient->XFRport;
			CloseHandle(hand);

			delete LoginClient;

			LoginClient = new CMsnClient();

			HANDLE hand = LoginClient->MSNLogin(NewIP,Newport);
			//WaitForSingleObject(hand , INFINITE);


		
		} */
	 


}

void CAntiSpamServer::HandleSocketMgr(SOCKET_OBJ *sock)
{
	//return;
	//sock->LastCmd = SM_QUIT;
	//SendLn(sock,"550 IDLE");



	if (sock->s != INVALID_SOCKET)
	{

		shutdown(sock->s,SD_BOTH);

		//LINGER       lingerStruct;   
		//lingerStruct.l_onoff  = 1;   
		//lingerStruct.l_linger =  0;   
		//setsockopt(sock->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		//BOOL   bDontLinger=FALSE;         
		//setsockopt(sock->s,SOL_SOCKET,SO_DONTLINGER,(LPCTSTR)&bDontLinger,sizeof(BOOL));   
  
		closesocket(sock->s);
		sock->s = INVALID_SOCKET;

	}
	else if (sock->OutstandingOps == 0)
	{

		_cprintf("Free Sock in Mgr \r\n");
		FreeSocketProc(sock);

			

		//PostQueuedCompletionStatus (CompletionPort,0,(u_long ) sock,0);
	}


	
	//CResourceMgr::DoDisconnect(sock);
	//closesocket(sock->s);
	//sock->s = INVALID_SOCKET;

//	OnBeforeDisconnect(sock);




}

int CAntiSpamServer::OnAfterAccept(SOCKET_OBJ* sock) //連線後可以送的第一個 command
{
	HANDLE ProcessHeap = GetProcessHeap();
	sock->UserData =  (char *) CHeap::GetHeap(sizeof(SMTPData),&ProcessHeap);
	SMTPData *smtpdata = (SMTPData *) sock->UserData;

 	if (Settings.ServerMode[0] != 'H')
	{
		_cprintf("Goto to Check RBL: %s \n",sock->RemoteIp);
		if ( CheckRBL(sock->RemoteIp) != NO_ERROR)
		{
			smtpdata->SpamConnection = true;
		}

		_cprintf("RBL Finish: %s \n",sock->RemoteIp);
	}

	int rc = NO_ERROR;
	char PTRChar[255]={0};
	HANDLE handle;
	//try
	//{
	   handle  = dns->Resolve(Settings.DnsIp,sock->RemoteIp,qtPTR,PTRChar,&rc);	 	
	//}
	//catch(...)
	//{
	
		//RSet Componet
		//delete dns;
		//dns = new CDnsClient();

		//handle  = dns->Resolve(Settings.DnsIp,sock->RemoteIp,qtPTR,PTRChar,&rc);	 	
				
					
	//}
	


	WaitForSingleObject(handle,2 * 1000);	

	CloseHandle(handle);

	strcpy(smtpdata->PTR,PTRChar);

	char TmpStr[60];
	sprintf(TmpStr,"RemoteIP: %s [%s]\r\n",sock->RemoteIp,smtpdata->PTR);

	_cprintf(TmpStr);

	WriteLog(TmpStr);


 	
	return 	SendLn(sock,"220 Want Email No Spam !");
}

void CAntiSpamServer::SendErrMail(char* ErrorStr)
{
	 FILE *fp = fopen("C:\\CrashMail.eml","W+b");
	  unsigned int MailLen=0;
	  if (fp != NULL) 
	  {
			fseek(fp,0,SEEK_SET);
			fwrite(ErrorStr,1,strlen(ErrorStr),fp);
			fseek(fp,0,SEEK_END);
			MailLen = ftell(fp);
	  }
	  fclose(fp);
	

	 // _cprintf("SpamDog Server Crash!\n");

	  //Send Mail
      char SendResult[255]={0};				
	  int rc = NO_ERROR;

	  CSMTPClient *smtpclient = new CSMTPClient;
	  HANDLE smtphd =  smtpclient->Send("192.168.1.3",
								 "softworking.com",
								 "kenny@microbean.com.tw",
								 "kenny@microbean.com.tw",
								 "C:\\CrashMail.eml",
								 0,
								 MailLen,
								 SendResult,
								 &rc);

	  	if (WaitForSingleObject(smtphd,5 * 60 * 1000) == WAIT_OBJECT_0)
		{
					
						CloseHandle(smtphd);
						if (rc == NO_ERROR && SendResult[0] == 0)
						{										 

							_cprintf("Send OK \r\n");

						}
						else
						{
							_cprintf("Send ERROR %s\r\n",SendResult);
						}

		}
		else
		{
					CloseHandle(smtphd);
		}

	

	  delete smtpclient;


}

int CAntiSpamServer::OnConnect(sockaddr* RemoteIp) //在 配至資源 之前 , 可拒絕 client
{
	//if (SocketCount >= 1)
		 //return SOCKET_ERROR;


	char ip[50];
	sprintf(ip,"%s",inet_ntoa(((struct sockaddr_in *) RemoteIp)->sin_addr));	

    if (CheckBlockIp(ip) == true)
		 return SOCKET_ERROR;

	 
	


/*
	if (strncmp(ip,"121.35.7",8) == 0 ||
		strncmp(ip,"116.24.131",10) == 0 ||
		strncmp(ip,"116.7.",6) == 0 ||
		strncmp(ip,"116.25",6) == 0 ||
		strncmp(ip,"116.30",6) == 0 ||
		strncmp(ip,"118.102.25.",11) == 0 ||
		strncmp(ip,"119.122.",8) == 0 ||
		strncmp(ip,"119.123.",8) == 0 ||
		strncmp(ip,"119.136.",8) == 0 ||
		strncmp(ip,"121.15.",7) == 0 ||
		strncmp(ip,"121.34.",7) == 0 ||
		strncmp(ip,"121.35.",7) == 0 ||
		strncmp(ip,"58.8.171.251",12) == 0 ||
		strncmp(ip,"58.61.",6) == 0 ||
		strncmp(ip,"220.101.214.",12) == 0 ||
		strncmp(ip,"63.174.244.250",14) == 0 ||
		strncmp(ip,"67.221.174.100",14) == 0 ||
        strncmp(ip,"202.109.80.151",14) == 0 ||
		strncmp(ip,"190.115.199.176",15) == 0 ||		
		strncmp(ip,"192.203.222.29",14) == 0 ||
		strncmp(ip,"208.84.68.188",14) == 0
		
*/


	  

		

	    /*char TempFile[_MAX_PATH];
		char SendStr[1025];

		TempFile[0] = 0;

		strcpy(SendStr,"//GetSpamIp.asp?IP=");
		strcat(SendStr,ip);	

		int rc = NO_ERROR;
		HANDLE hd = http.IPGET(Settings.SpamDogCenterIp,"www.softworking.com",SendStr,TempFile,&rc);
		WaitForSingleObject(hd,10 * 1000);
		CloseHandle(hd);

		if (TempFile[0] != 0)
		{
			FILE *fp = NULL;
			fp = fopen(TempFile , "rb");

			int CheckRes=-1;
			fscanf(fp,"%d\n",&CheckRes);
	 
			fclose(fp);			

			DeleteFile(TempFile);

			if (CheckRes >= 2) 
				 return SOCKET_ERROR;

		}*/

	//char PTRResult[255];
	//PTRResult[0] = 0;

	//_cprintf("Goto to Check DNS: %s \n",ip);

	


	//ServerDB->OpenIpDB();
	time_t LastConnect;
	try
	{	
		 LastConnect = ServerDB->LastConnectTime(ip);
	} 
	catch(...)
	{
					printf("LastConnectTime Error \r\n");
					SendErrMail("LastConnectTime Error \r\n");			
	}
	//ServerDB->CloseIpDB();

	
	//check retry time	

	if (Settings.LimitRetryTime > 0 && LastConnect != 0 && strcmp(ip,"192.168.1.4") != 0 )
	{
	
		time_t now;
		time(&now);

		double diff = difftime(now,LastConnect);		

		if (diff < Settings.LimitRetryTime)
		{
			_cprintf("Time Limit %s [%f] \n",ip,diff);
			return SOCKET_ERROR;			
		}

	
	}

	InterlockedIncrement(&SocketCount);

	char sc[50] ;
	char scs[50];

	itoa(SocketCount , sc,10);
	strcpy(scs,"Socket Count :");
	strcat(scs , sc);
	strcat(scs," ->IP:");
	strcat(scs,ip);
	strcat(scs, "\r\n");

	//OutputDebugString(scs);
	_cprintf("\r\n%s\r\n\r\n",scs);

	if (Settings.ServerMode[0] == 'H')
		return CheckRBL(ip);

	 

	return NO_ERROR;
}
int CAntiSpamServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	
	int rc = NO_ERROR;

	char MailFrom[1024]={0};
	char Subject[1024]={0};

	SMTPData *userdata = (SMTPData *) sock->UserData;

	CBaseServer::OnDataRead(sock,buf,BytesRead);

	WriteLog(buf->buf);
	
	//_cprintf("%s",buf->buf);

	if (sock->LastCmd == 0)
	{
		//第一個指令, 確認 helo
		if (strnicmp(buf->buf,"HELO",4) == 0 || strnicmp(buf->buf,"EHLO",4) == 0)
		{		
			sock->LastCmd = SM_HELO;
			rc = SendLn(sock,"250 OK");
		}
		else
		{
			rc = SOCKET_ERROR;		
		}
	}
	else if (sock->LastCmd == SM_HELO)
	{
	
		if (strnicmp(buf->buf,"MAIL",4) == 0) 
		{
			
			_TCHAR *start, *end;

			start = _tcsstr(buf->buf,"<");			
			end = _tcsstr(buf->buf, ">");	 

			if (end > start + 1) 
			{
				int len = end - 1 - (start + 1) + 1;
				//_tcsncpy(userdata->MailList.MailFrom, start + 1, len);		
				//userdata->MailList.MailFrom[len]=0;	
				_tcsncpy(userdata->MailFrom, start + 1, len);		
				userdata->MailFrom[len]=0;				
				
				//_cprintf("MailFrom : %s \n",userdata->MailFrom);

				_TCHAR *mailpos =  _tcsstr (userdata->MailFrom,"@");

				if (_tcslen(userdata->MailFrom) > 0 && mailpos != NULL && (mailpos - userdata->MailFrom) > 1 && userdata->MailFrom[len-1] != '@')
				{
				
					_TCHAR EmailDomain[255];
					_tcscpy(EmailDomain,mailpos+1);

					bool HostNameCheck = false;

					//先比對 RemoteIp Hostname
					if (userdata->PTR[0] != 0) 
					{
						char PTRDomainIdent[255]={0};
						char EmailDomainIdent[255]={0};

						GetDomainIdent(userdata->PTR , PTRDomainIdent);
						GetDomainIdent(EmailDomain , EmailDomainIdent);

						if (PTRDomainIdent[0] != 0 && EmailDomainIdent[0] != 0)
						{
							if (stricmp(PTRDomainIdent,EmailDomainIdent) == 0)
							{
								HostNameCheck = true;
								_cprintf("HostNameCheck OK \n");
							}
						
						}
					
					}
					
					if (!HostNameCheck)
					{
						if (!CheckFromIp(EmailDomain,sock->RemoteIp,userdata->PTR))
						{
							userdata->SpamConnection = true;
							_cprintf("SpamConnection ! \n");
						}
						else
						{
						    _cprintf("RemoteIP Check OK \n");
						}
					}			
					
					//產生 Incoming Mail
					char InComingPath[_MAX_PATH];
					strcpy(InComingPath,ServerDB->PGPath);
					strcat(InComingPath,"Incoming");

					GetTempFileName(InComingPath, // directory for temp files 
						"Mail",                    // temp file name prefix 
						0,                        // create unique name 
						userdata->TempFileName);              // buffer for name 		
					
					FILE *fp = fopen(userdata->TempFileName,"r+b");
					//OutputDebugString("FP Inc \r\n");
					userdata->fp = fp;				
					
					//strcpy(userdata->Ip , sock->RemoteIp);
					time(&userdata->ReceiveTime);
					
					fprintf(userdata->fp,"SpamDog-MailFrom: %s\r\n",userdata->MailFrom);

					sock->LastCmd = SM_MAIL;
					rc = SendLn(sock,"250 OK");

				}
				else
				{
						rc = SOCKET_ERROR;
				}

				

			}
			else
			{
				rc = SOCKET_ERROR;
				
			}
		}
		else if (strnicmp(buf->buf,"RSET",4) == 0) 
		{
			sock->LastCmd = SM_HELO;
			rc = SendLn(sock,"250 OK");
		}
		else
		{
			rc = SOCKET_ERROR;		
		}

	
	}
	else if (sock->LastCmd == SM_MAIL)
	{
		if (strnicmp(buf->buf,"RCPT",4) == 0) 
		{		
			
				
				_TCHAR *pos1 = _tcsstr(buf->buf,"<");
				if (pos1 != NULL)
				{
					_TCHAR *pos2 = _tcsstr(pos1,">");

					if (pos2 != NULL)
					{
						_TCHAR ToMail[255];
						memset(ToMail,0,255);
						_tcsncpy(ToMail,pos1+1, pos2 - pos1 -1);
						int CkPIPstatus = 0 ;

						ToLowCase(ToMail);

						_tcscpy(userdata->RcptTo,ToMail);

						_TCHAR * findchar = _tcsstr(ToMail,"@");
						if (findchar == NULL)
							return SOCKET_ERROR;					

						//strncpy(userdata->Account,ToMail,findchar - ToMail);
						_tcscpy(userdata->Domain,findchar+1);

						//userdata->Account[findchar - ToMail] = 0;


						bool PassMail = false;
					

						if (Settings.AccountCheckMode == 1 || Settings.ADServerIp[0] == 0)
						{
							//
							//PassMail = CheckDBExist();
							//_TCHAR AccountName[255];
							//_tcsncpy(AccountName,ToMail,findchar-ToMail); //取得 rcpt account


						     //Retrive Account List
							RetriveAccountList(userdata,ToMail);

							if (userdata->AccCount > 0)
								PassMail = true;

							//PassMail = MailServerDB->CheckAccountExists(AccountName);
						
						}
						else
						{
							_cprintf("Check AD ...\r\n");	
							//PassMail = CheckADExist(Settings.ADServerIp,ToMail,Settings.ADLoginId,Settings.ADPassWord);
							GetADAccList(Settings.ADServerIp,ToMail,Settings.ADLoginId,Settings.ADPassWord,userdata);

						    if (userdata->AccCount > 0)
								PassMail = true;
							 
						}

						
						//PassMail = true;

						int AcceptCount = 0;
						int NomailCount = 0;
						int DenyCount = 0;
						
						if (PassMail)
						{
													 
							_cprintf("Check Fine\r\n");

							//travel acc list
							

							AccountList *ptr = userdata->AccList;
							while (ptr)
							{							

								ptr->PersonalAcceptType = CheckPersonalIP(ptr->DBAccountId,sock->RemoteIp);
							
								if (CkPIPstatus == Personal_Accept)
								{
									AcceptCount++;
									//userdata->PersonalPass = true;	
								}
								else if (CkPIPstatus == Personal_Deny)
								{								
									//移出接收清單
									DeleteAccountList(userdata,ptr->Account);

									//PassMail = false;	
									DenyCount++;
								} 
								else
								{
								
									NomailCount++;
								}

								ptr = ptr->next; 	
							  
							}


							//RCPT cmd - Start to check personal info - IP
						
						 
						
						
						}
					 
						//if(Settings.ServerMode == 'S'  && AcceptCount == 0 && userdata->SpamConnection)
						//{


						//		  rc = SOCKET_ERROR;
						//		  return rc;
							 
						//} else
						if (userdata->AccCount > 0 && DenyCount < userdata->AccCount)//全部都拒決則不收信
						{						 
							
							//寫入 rcpt
											 

							//ProcessMailList(sock);

							AccountList *ptr = userdata->AccList;
							while (ptr)
							{	

								if (Settings.ServerMode[0] == 'S' && userdata->SpamConnection && ptr->PersonalAcceptType != Personal_Accept)
								{
									   //char Desc[255];
									   //strcpy(Desc,"SpamMail in Standard Mode : Reject Connection ");
									   //strcat(Desc,	userdata->MailList.MailFrom);

									   //mImapServer->AddImapHistoryIpMail(ptr->Account,userdata->Domain,sock->RemoteIp,userdata->MailList.MailFrom,Desc);

								}
								else
								{								
								
									
								
								}

								fprintf(userdata->fp,"SpamDog-RcptTo: %s\r\n",ptr->EMail);

									  //寫到 imap ip history
								   ptr = ptr->next; 	
							}
							
						

							//fprintf(userdata->DataFp,"%s\r\n",userdata->RcptTo);


							//Personal Accept Not Delay Send
							// Low Mode not Delay Send
							// Not Spam Connection No Delay Send
							_cprintf("AD Process Fine\r\n");

							if ( Settings.ServerMode[0] != 'L' && !userdata->SpamConnection && Settings.OkDelayResponse == 0 && AcceptCount == 0)
							{
								sock->LastCmd = SM_RCPT;
								rc = SendLn(sock,"250 OK");
							}
							else if ( Settings.ServerMode[0] != 'L' && (Settings.SpamDelayResponse > 0  || Settings.OkDelayResponse > 0) && AcceptCount == 0) //只要有人收 就不進 delay send
							{
								strcpy(userdata->DelayCmdTxt,"250 OK");
								userdata->DelayCmd = SM_RCPT;

								time_t now;
								time(&now);	

								if (userdata->SpamConnection)
								{
								   userdata->DelayResponseTime = now  + (Settings.SpamDelayResponse);
								}
								else
								{
								   userdata->DelayResponseTime = now  + (Settings.OkDelayResponse);
								}

								sock->SocketType = ST_FOR_DELAY; //ST_FOR_DELAY

								rc = NO_ERROR;
							
							}
							else
							{
								sock->LastCmd = SM_RCPT;
								rc = SendLn(sock,"250 OK");
							}
						
						}
						else
						{
							_cprintf("Check AD Not Exist or Personal Deny\r\n");	

							rc = SOCKET_ERROR;	
						}
					}
					else
					{
						rc = SOCKET_ERROR;
					}
				
				
				}
				else
				{
					rc = SOCKET_ERROR;
				}

	
		
		}
		else if (strnicmp(buf->buf,"RSET",4) == 0) 
		{
			sock->LastCmd = SM_HELO;
			rc = SendLn(sock,"250 OK");
		}
		else
		{
			rc = SOCKET_ERROR;		
		}
	}
	else if (sock->LastCmd == SM_RCPT)
	{
	
		if (strnicmp(buf->buf,"DATA",4) == 0) 
		{		
			
			sock->LastCmd = SM_DATA;

			//InitializeCriticalSection(&userdata->SMTPCritSec);
			
			//寫入 receieve
			//fwrite("Received: from ([",1,17,userdata->fp);
			
			//fwrite(sock->RemoteIp,1,strlen(sock->RemoteIp),userdata->fp);
			struct tm *newtime;
			long ltime;

			time( &ltime );

			/* Obtain coordinated universal time: */
			newtime = gmtime( &ltime );
		 
			//char *TimeStr = asctime( newtime );
			//TimeStr[strlen(TimeStr) - 1] = 0; //trim /n
			char TimeStr[255];
			strftime(TimeStr,255,"%a, %d %b %Y %H:%M:%S +0000",newtime);

			fprintf(userdata->fp,"Received: from ([%s]) by %s with SpamDog AntiSpam Server ; %s \r\n",sock->RemoteIp,Settings.ServerName,TimeStr);
			fprintf(userdata->fp,"SpamDog-RemoteIp: %s\r\n",sock->RemoteIp);
			fprintf(userdata->fp,"X-Mailer: SpamDog AntiSpam (%s)\r\n",sock->RemoteIp);
			//寫入 rcpt , mail from 
			//fwrite("]) by softworking.com with SpamDog AntiSpam Server\r\n",1,52,userdata->fp);

			rc = SendLn(sock,"354 OK");
		}
		else if (strnicmp(buf->buf,"RSET",4) == 0) 
		{
			sock->LastCmd = SM_HELO;
			rc = SendLn(sock,"250 OK");
		}
		else
		{
			rc = SOCKET_ERROR;		
		}
	
	}
	else if (sock->LastCmd == SM_DATA)
	{
		//bool IsMailEnd = false;
		char EndStr[6];
		strcpy(EndStr,"\r\n.\r\n");	

		/*

		if (BytesRead == 5)
		{
			if (strncmp(buf->buf,LastEndStr,5) == 0 )
				IsMailEnd = true;
		} 
		else if (BytesRead > 5)
		{
			//檢查最後5bytes
			if (strncmp(buf->buf + (BytesRead - 5),EndStr,5) == 0)
			{
				IsMailEnd = true;
			}
			else
			{	//取最後5bytes
				strncpy(LastEndStr,buf->buf + (BytesRead - 5),5);
			
			}
		}
		else
		{

		
		}*/



			
				

		if (strncmp(buf->buf + (BytesRead - 5),EndStr,5) == 0  ||
			((BytesRead <5 && BytesRead >=3)  && strncmp(buf->buf ,EndStr + 5- BytesRead,BytesRead) == 0) )
		{
			
			sock->LastCmd =  SM_QUIT;

			//DeleteCriticalSection(&userdata->SMTPCritSec);

			

			if (BytesRead > 5)
			{
			  fwrite(buf->buf,1,BytesRead-5,userdata->fp);
			}
		
			//放入 testing string
			//char tmpchar[50];
			//strcpy(tmpchar,"\r\n");
			//strcat(tmpchar,"SpamDog Server URL Tesing : softworking.com");
			//fwrite(tmpchar,1,strlen(tmpchar),userdata->fp);

			//Check Personal B-Rate 
			fseek(userdata->fp,0,SEEK_END);
			int filesize = ftell(userdata->fp);

			userdata->MailSize = filesize;
			 
			

			//if ()
			//{
				char *MailBuffer = new char[filesize + 1];
				MailBuffer[filesize] = 0;
				fseek(userdata->fp,0,SEEK_SET);
				fread(MailBuffer,1,filesize,userdata->fp);

				AccountList *ptr = userdata->AccList;
				while (ptr)
				{

					//在 rbl 中 , 不再檢查 rate	
					if (ptr->PersonalAcceptType == Personal_None )
						{
							double SpamRate = 0.5;

							char *MailText = new char[strlen(MailBuffer)+1];
							char *MailContent = new char[strlen(MailBuffer)+1];

							MailText[0] = 0;
							MailContent[0] = 0;

							
							
							CEmail cem;						
							
							MailData *mb = cem.GetMailData(MailBuffer);
							if (mb == NULL)
							{
								delete MailText;
								delete MailContent;
								delete MailBuffer;

								fclose(userdata->fp);
								userdata->fp = NULL; 

								//OutputDebugString("FP Dec \r\n");
								
								return rc;							
							}

							MailHeader *mh = NULL;

						//	try
						//	{

							 mh = cem.GetMailHeader(mb->mailHeader);

							 

							 if (strlen(mh->From) >= 1024)
							 {
								 strncpy(MailFrom,mh->From,1024);
							 }
							 else
							 {
							     strcpy(MailFrom , mh->From); 
							 }

							//strcpy(MailFrom , mh->From);
							//strcpy(Subject , mh->Subject);

							 if (strlen(mh->Subject) >= 1024)
							 {
								 strncpy(Subject,mh->Subject,1024);
							 }
							 else
							 {
							     strcpy(Subject , mh->Subject); 
							 }

						//	} 
						//	catch(...)
						//	{
						//			printf("Parser Error \r\n");
						//			SendErrMail("Parser Error \r\n");			
						//	}

							if  (userdata->SpamConnection)
							{
								cem.FreeMailHeader(mh);
								cem.FreeMailData(mb);
								delete MailText;
								delete MailContent;

								break;

							
							}

							

							if (filesize < Settings.MaxNoSpamSize )
							{
									CChecker cc(&mMailDBSection,"Null","SpamDogServer.ini"); //0 for func use
									cc.MainTainRBL(Settings.SpamDogCenterIp ,false,sock->RemoteIp);	
							
									cem.FreeMailHeader(mh);
									cem.FreeMailData(mb);
									delete MailText;
									delete MailContent;

								break;
							}


						    if (mh->Content_Type->boundary[0] != 0)
							{
							//代表有 boundary

							   MailBoundary* m_MailBoundary = 
							   cem.GetBoundary(mb->mailBody,mh->Content_Type->boundary);
							   
							    MailBoundary* ptr =  m_MailBoundary;

								//char tmpc[500];
								//int BoundCount = 0;
								while (ptr)
								{
									if (ptr->IsRealData)
									{
										//BoundCount ++ ;	
									
										//strncpy(tmpc,ptr->BoundContent,499);

										if (stricmp(ptr->BoundHeader->Content_Transfer_Encoding,"base64") == 0)
										{
											
										
											CMailCodec cm;
											char *out = NULL;
											unsigned int size=0;
											
											cm.Base64Decode(ptr->BoundMail->mailBody,&out,&size);

											//printf("%s\n",out);
											strcat(MailContent,out);

											if (strlen(out) > 0 )
											{

												CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(out) + 1];

												ccd.TrimHTML(out,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;
											}

											cm.FreeBase64Decode(out);
										
										}
										else if (stricmp(ptr->BoundHeader->Content_Transfer_Encoding,"quoted-printable") == 0)
										{
										
												CMailCodec cm;
												char *out = cm.QPDecode(ptr->BoundMail->mailBody);

												//printf("%s\n",out);
												strcat(MailContent,out);

												CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(out) + 1];

												ccd.TrimHTML(out,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;

												cm.FreeQPDecode(out);
										
										}
										else
										{
												
												strcat(MailContent,ptr->BoundMail->mailBody);

												CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(ptr->BoundMail->mailBody) + 1];

												ccd.TrimHTML(ptr->BoundMail->mailBody,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;
										
										}

										//tmpc[499] = 0;
										//printf("%s\n",tmpc);					
										//printf("level : %d %s=>%s+++++++++++++++++++++++++++++++++++++++++\n\n\n",ptr->level,ptr->BoundHeader->Content_Type->text,ptr->BoundHeader->Content_Transfer_Encoding);


									}
 
									ptr = ptr->next;	
								}

								cem.FreeBoundary(m_MailBoundary);


						   
							}
						    else
							{
						   
							   if (stricmp(mh->Content_Transfer_Encoding,"base64") == 0)
							   {
					   						CMailCodec cm;
											char *out = NULL;
											unsigned int size=0;
											
											cm.Base64Decode(mb->mailBody,&out,&size);

											//printf("%s\n",out);
											strcat(MailContent,out);

												CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(out) + 1];

												ccd.TrimHTML(out,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;



											cm.FreeBase64Decode(out);
							   }
							   else if (stricmp(mh->Content_Transfer_Encoding,"quoted-printable") == 0)
							   {
											CMailCodec cm;
											char *out = cm.QPDecode(mb->mailBody);

											//printf("%s\n",out);
											strcat(MailContent,out);

												CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(out) + 1];

												ccd.TrimHTML(out,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;

											cm.FreeQPDecode(out);
							   
							   }
							   else
							   {
								   strcat(MailContent,mb->mailBody);

								   				CMailCodec ccd;	
												char *TmpBuffer = new char[strlen(mb->mailBody) + 1];

												ccd.TrimHTML(mb->mailBody,TmpBuffer);
												strcat(MailText,TmpBuffer);

												delete TmpBuffer;
								   
							   
							   }
								   
						   
							}


						


							
							char DBNum[50];
							itoa(ptr->DBAccountId,DBNum,10);

						

							CChecker cc(&mMailDBSection,DBNum,"SpamDogServer.ini");
							SpamRate = MailServerDB->CountSpamRate(&cc,
																   MailContent,
																   MailText,
																   mh->From,
																   mh->Subject,
																   sock->RemoteIp);

							if (SpamRate > 0.5)
							{
								//This is a spam mail
								//userdata->SpamConnection = true;
								MailServerDB->UpdateSpamRate(&cc,0,1,true);
								ptr->SpamMail = true;
								//OutputDebugString("SpamRate > 0.5 \r\n");

								cc.MainTainRBL(Settings.SpamDogCenterIp ,true,sock->RemoteIp);
							}
							else
							{						
								MailServerDB->UpdateSpamRate(&cc,1,0,true);
								cc.MainTainRBL(Settings.SpamDogCenterIp ,false,sock->RemoteIp);
							}

							cem.FreeMailHeader(mh);
							cem.FreeMailData(mb);
							delete MailText;
							delete MailContent;
						}

					
					ptr = ptr->next;
				}
				
				delete [] MailBuffer;

			

			//}
			//else
			//{
				//CChecker cc(&mMailDBSection,"Null","SpamDogServer.ini"); //0 for func use
				//cc.MainTainRBL(Settings.SpamDogCenterIp ,false,sock->RemoteIp);			
			//}
			 

			//寫入 mail from
			//fseek(userdata->fp,0,SEEK_END);
			//_ftprintf(userdata->fp,"%s\r\n",userdata->MailList.MailFrom); 			

			//fclose(userdata->DataFp);
			//userdata->DataFp = NULL; //此時尚未放入 rcpt to

			
			

			try
			{
		
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_EXT];
		
			_splitpath(userdata->TempFileName, drive, dir, fname, ext );
			dir[strlen(dir)-strlen("Incoming")-1] = 0; //"Incoming" size
			
			if (Settings.ServerMode[0] == 'H' && userdata->SpamConnection)
			{
				//
				rc = SOCKET_ERROR;
			
			} 
			else 
			{
				bool NeedRelay = false;
				
				AccountList *ptr = userdata->AccList;
				while (ptr)
				{
					
					//if ((userdata->SpamConnection && ptr->PersonalAcceptType != Personal_Accept) || ptr->SpamMail)
					if (1 != 1) //all mail not to imap
					{
						
						//MailServerDB->NewMail(ptr->Account,Folder_SpamMail,MailBuffer);
						char * EnvStr = mImapServer->GetEnvelopeStr(userdata->fp);
						if (EnvStr != NULL)
						{
							MailServerDB->NewImapMail(ptr->DBAccountId,
													  Folder_SpamMail,
													  NULL,
													  0,
													  EnvStr,
													  userdata->fp);

							delete EnvStr;

							mImapServer->SendStatus(ptr->DBAccountId,Folder_SpamMail);
						}
						//將資料放入 ServerDB
						//通知 imap send status	

						
					/*	int LenLim = 1023;						
						char MSNStr[1024] = {0};
						strcpy(MSNStr , "您有一封垃圾信 \r\n");
						strcat(MSNStr ,MailFrom);						
						strcat(MSNStr , "\r\n");
						strcat(MSNStr , userdata->RcptTo);
						strcat(MSNStr , "\r\n");

						LenLim = LenLim - strlen(MSNStr);

						if (strlen(Subject) > LenLim)
						{
							if (LenLim > 0)
							strncat(MSNStr,Subject , LenLim);
						}
						else
						{						 
							strcat(MSNStr, Subject);
						}*/

						/*WCHAR*  strA; 
			
						int  i=  MultiByteToWideChar(CP_ACP,0,(char*)  MSNStr,-1,NULL,0);  						
						strA  =  new  WCHAR[i];  
						MultiByteToWideChar  (CP_ACP,0,(char *) MSNStr,-1,strA,i);   
						i=  WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);  
						char  *strB=new  char[i];  
						WideCharToMultiByte  (CP_UTF8,0,strA,-1,strB,i,NULL,NULL);  

						*/

						//SendMSG(MSNStr);

						/*char TempFile[_MAX_PATH];
						char SendStr[1025];

						TempFile[0] = 0;

						strcpy(SendStr,"//SpamIp.asp?IP=");
						strcat(SendStr,sock->RemoteIp);
						strcat(SendStr,"&sub=");
						strcat(SendStr,Subject);

						int rc = NO_ERROR;
						HANDLE hd = http.IPGET(Settings.SpamDogCenterIp,"www.softworking.com",SendStr,TempFile,&rc);
						WaitForSingleObject(hd,10 * 1000);
						CloseHandle(hd);

						if (TempFile[0] != 0)
							DeleteFile(TempFile);*/
						

						//MSN
						//LoginClient->SendMsg("service@softworking.com",strB); 
						//LoginClient->SendMsg("kenny@ezfly.com",strB); 
						

						//delete [] strA ;
						//delete [] strB ;*/
					 
					} else
					{

					
						//正常信放入 Incoming Box

						//char * EnvStr = mImapServer->GetEnvelopeStr(userdata->fp);
						if (Settings.RelayPopMode == 0)
						{
							MailServerDB->RelayMail(userdata->fp);
						}
						else
						{
							//POP

							char * EnvStr = mImapServer->GetEnvelopeStr(userdata->fp);
							if (EnvStr != NULL)
							{
								MailServerDB->NewImapMail(ptr->DBAccountId,
														  Folder_InBox,
														  NULL,
														  0,
														  EnvStr,
														  userdata->fp);

								delete EnvStr;

								mImapServer->SendStatus(ptr->DBAccountId,Folder_InBox);
							}

							
						}


						//lete EnvStr;

						//mImapServer->SendStatus(ptr->DBAccountId,Folder_InBox);

						//relay mail						
						NeedRelay = true;


						 
					 
					/*	int LenLim = 1023;
						char MSNStr[1024] = {0};
						strcpy(MSNStr , "您有一封信 \r\n");
						strcat(MSNStr ,MailFrom);
						strcat(MSNStr , "\r\n");
						strcat(MSNStr , userdata->RcptTo);
						strcat(MSNStr , "\r\n");

						LenLim = LenLim - strlen(MSNStr);

						if (strlen(Subject) > LenLim)
						{
							if (LenLim > 0)
							strncat(MSNStr,Subject , LenLim);
						}
						else
						{						 
							strcat(MSNStr, Subject);
						}

						SendMSG(MSNStr);*/

					 /*
						WCHAR*  strA; 
			
						int  i=  MultiByteToWideChar(CP_ACP,0,(char*)  MSNStr,-1,NULL,0);  						
						strA  =  new  WCHAR[i];  
						MultiByteToWideChar  (CP_ACP,0,(char *) MSNStr,-1,strA,i);   
						i=  WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);  
						char  *strB=new  char[i];  
						WideCharToMultiByte  (CP_UTF8,0,strA,-1,strB,i,NULL,NULL);  


						//MSN
					
						//LoginClient->SendMsg("service@softworking.com",strB); 
						LoginClient->SendMsg("kenny@ezfly.com",strB); 
					
						

						delete [] strA ;
						delete [] strB ;*/
					
					}

				
					ptr = ptr->next; 
				}
				
				
			
				fclose(userdata->fp);
				userdata->fp = NULL; 

				
			

				//OutputDebugString("FP Dec \r\n");

				if (NeedRelay)
				{
							//觸動 relay
						SetEvent(RelayMailEvent);

				}

				DeleteFile(userdata->TempFileName);
				//DeleteFile(FromData);
				



			
				
				rc = SendLn(sock,"250 OK");

			
								
			} 
		 
			} 
			catch(...)
			{
					printf("Parser Error 2\r\n");
					SendErrMail("Parser Error 2 \r\n");			
			}

		

			
		  
		}
		else if (strnicmp(buf->buf,"RSET",4) == 0) 
		{
			if (userdata->fp != NULL)
			{
			   fclose(userdata->fp);
			   userdata->fp = NULL;

			   //OutputDebugString("FP Dec \r\n");
			   
			   DeleteFile(userdata->TempFileName);

			}

			/*if (userdata->DataFp != NULL)
			{
			   fclose(userdata->DataFp);
			   userdata->DataFp = NULL;

			   char DataPath[_MAX_PATH];
			   strcpy(DataPath,userdata->TempFileName);
			   strcat(DataPath,".dat");
			   
			   DeleteFile(DataPath);

			}*/
			
			sock->LastCmd = SM_HELO;
			rc = SendLn(sock,"250 OK");
		}
		else if (userdata->fp != NULL)
		{
			
			//EnterCriticalSection(&userdata->SMTPCritSec);
			fwrite(buf->buf,1,BytesRead,userdata->fp);
			//LeaveCriticalSection(&userdata->SMTPCritSec);
			
			
			rc = NO_ERROR;		
		} 
		else
		{
		    rc = SOCKET_ERROR;
		}

	}
	else if (sock->LastCmd == SM_QUIT)
	{
		if (strnicmp(buf->buf,"QUIT",4) == 0) 
		{
			rc = SendLn(sock,"250 BYE");
			 //rc = SOCKET_ERROR;
		}
		else
		{
			rc = SOCKET_ERROR;		
		}
	}

	//char mstr[50];
	//wsprintf(mstr,"Got Data:%d",BytesRead);
	//return SendLn(sock,mstr);

	//SendLn(sock,"go");

	
	 
	return rc;
}

void CAntiSpamServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{
 	InterlockedDecrement(&SocketCount);


	try
	{


	SMTPData *userdata = (SMTPData *) sock->UserData;

	_cprintf("Disconnect : %s\n",sock->RemoteIp);


	char sc[50] ;
	char scs[50];

	itoa(SocketCount , sc,10);
	strcpy(scs,"Socket Count :");
	strcat(scs , sc);
	strcat(scs," <-IP:");
	strcat(scs,sock->RemoteIp);
	strcat(scs, "\r\n");
//	OutputDebugString(scs);
	_cprintf("\r\n%s\r\n\r\n",scs);

	//Delay Send Check
/*
	if (userdata->DelayResponseTime >0 )
	{
		
		AccountList *ptr = userdata->AccList;
		while (ptr)
		{

			char tmpstr[2048];
			char tmpstr2[255];

			//if (userdata->PTR[0] != 0) 
			//{

			if (userdata->SpamConnection)
			{
			  //userdata->MailList.Status = 2;
				strcpy(tmpstr,"此 IP 已在 RBL 中 ");
				strcat(tmpstr,sock->RemoteIp);
				strcat(tmpstr,"\r\n");	
				if (userdata->PTR[0] != 0)
				strcat(tmpstr,userdata->PTR);

				strcpy(tmpstr2,"BAD IP ");
				strcat(tmpstr2,sock->RemoteIp);

				mImapServer->AddImapHistoryIpMail(ptr->Account,userdata->Domain,sock->RemoteIp,userdata->MailList.MailFrom,tmpstr2,tmpstr);
			}
			else
			{
				
				strcpy(tmpstr,"此 IP 正常 ");
				strcat(tmpstr,sock->RemoteIp);
				strcat(tmpstr,"\r\n");
				if (userdata->PTR[0] != 0)
				strcat(tmpstr,userdata->PTR);

				strcpy(tmpstr2,"OK IP ");
				strcat(tmpstr2,sock->RemoteIp);

				mImapServer->AddImapHistoryIpMail(ptr->Account,userdata->Domain,sock->RemoteIp,userdata->MailList.MailFrom,tmpstr2,tmpstr);
			 // userdata->MailList.Status = 3;
			}

			//}
			//ProcessMailList(sock);
			ptr = ptr->next;
		}
	
	}
*/
	if (userdata->fp != NULL)
	{
			   fclose(userdata->fp);
			   userdata->fp = NULL;

			   //OutputDebugString("FP Dec \r\n");
			   
			   DeleteFile(userdata->TempFileName);

	}

	/*if (userdata->DataFp != NULL)
	{
			   fclose(userdata->DataFp);
			   userdata->DataFp = NULL;

			   char DataPath[_MAX_PATH];
			   strcpy(DataPath,userdata->TempFileName);
			   strcat(DataPath,".dat");
			   
			   DeleteFile(DataPath);

	}*/

	if (userdata->AccList != NULL)
		DestoryAccountList(userdata);
	}
	catch(...)
	{}
}

// RelayMailThread
////////////////////////////////////////////////////////////////////// 
unsigned __stdcall ServerProcThread(LPVOID lpParam)
{

	SetThreadName(GetCurrentThreadId(),"SThread2");
	CAntiSpamServer *smtpsrv = (CAntiSpamServer *) lpParam; 
	while (smtpsrv->Running)
    {
		

		Sleep(1000);

		time_t   now;
		time(&now);

	

		//處理 DelaySend
		//_cprintf("Delay Send Check 1 \n");
			SOCKET_OBJ *ptrs = smtpsrv->m_SocketList;

			EnterCriticalSection(&smtpsrv->SocketListSection);
			while (ptrs)
			{	  
				//_cprintf("Delay Send Check 2 \n");
				//EnterCriticalSection(&ptrs->SockCritSec);
				//_cprintf("Delay Send Check 3 \n");
				SMTPData *userdata = NULL;
				userdata = (SMTPData *) ptrs->UserData;
				//_cprintf("Delay Send Check 4 \n");
				
				

				if (userdata != NULL && userdata->DelayResponseTime >0 )
				{
					//_cprintf("Delay Send Check 5 \n");
					double diff = difftime(now,userdata->DelayResponseTime) ;
					//_cprintf("Delay Send Check 6 \n");

					if (diff >= 0)
					{
					

						if (ptrs->s == INVALID_SOCKET)
						{
							//_cprintf("Delay Send INVALID_SOCKET\n");
							ptrs = ptrs->next;
							//LeaveCriticalSection(&ptrs->SockCritSec);
							continue;
						}



						//_cprintf("Delay Send Check 7 \n");
						ptrs->LastCmd = userdata->DelayCmd;
							_cprintf("Delay Send \n");

						ptrs->SocketType = ST_FOR_NORMAL;//change for normal
						smtpsrv->SendLn(ptrs,userdata->DelayCmdTxt);

						//clear delay time
						userdata->DelayResponseTime = 0;
						userdata->DelayCmd = 0;
						userdata->DelayCmdTxt[0] = 0;
					}
				}
		   
				//_cprintf("Delay Send Check 8 \n");
			//	LeaveCriticalSection(&ptrs->SockCritSec);
				ptrs = ptrs->next;
			}
 
			LeaveCriticalSection(&smtpsrv->SocketListSection);

	}

	

	_endthreadex(0);
    return 0;
}
// SetThreadName
//////////////////////////////////////////////////////////////////////
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = szThreadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
   }
   __except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
// RelayMailThread
////////////////////////////////////////////////////////////////////// 
unsigned __stdcall RelayMailThread(LPVOID lpParam)
{

	SetThreadName(GetCurrentThreadId(),"SThread1");
	

	//try
	//{

		//smtp 的資料要以 structer or File fp 傳入

	CAntiSpamServer *smtpsrv = (CAntiSpamServer *) lpParam;

    CServerDB  MailServerDB(&smtpsrv->mMailDBSection);

		    /*char RelayPath[MAX_PATH];

			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_EXT];
		
			_splitpath(smtpsrv->ServerDB->PGPath, drive, dir, fname, ext );
		
		
			strcpy(RelayPath,drive);
			strcat(RelayPath,dir);
			strcat(RelayPath,"\\Relay\\");*/

	if (smtpsrv->Settings.RelayToServerIp[0] != 0)
	{

		CSMTPClient *smtpclient = new CSMTPClient;
		while (smtpsrv->Running)
		{
			WaitForSingleObject(smtpsrv->RelayMailEvent,30 * 60 * 1000); //每半小時一次

			MailServerDB.OpenDB();

			RelayInfo relayinfo;
			memset(&relayinfo,0,sizeof(relayinfo));

			unsigned int infoPos = MailServerDB.ListFirstRelayInfo(&relayinfo);

		 
			while (infoPos > 0)
			{
				char SendResult[255]={0};				

				int rc = NO_ERROR;

				//先讀出 50 K 取 MailFrom , RcptTo
				char buf[1024 * 50];
				unsigned int FpPos = relayinfo.RawMailDataPos;
				MailServerDB.DumpMail(&relayinfo,buf,1024 * 50,FpPos);

				//轉 cmail parser 去取相關 header
				CEmail cm;
				MailHeader *mh = cm.GetMailHeader(buf);					
				
				HANDLE smtphd =  smtpclient->Send(smtpsrv->Settings.RelayToServerIp,
								 smtpsrv->Settings.ServerName,
								 mh->SpamDogMailFrom,
								 mh->SpamDogRcptTo,
								 MailServerDB.GetDBFile(),
								 relayinfo.RawMailDataPos,
								 relayinfo.RawMailDataLen,
								 SendResult,
								 &rc);




				if (WaitForSingleObject(smtphd,5 * 60 * 1000) == WAIT_OBJECT_0)
				{
					
						CloseHandle(smtphd);
						if (rc == NO_ERROR && SendResult[0] == 0)
						{
							//成功
							MailServerDB.SetRelayedInfo(relayinfo);						 

							_cprintf("Send OK \r\n");

						}
						else
						{
							_cprintf("Send ERROR %s\r\n",SendResult);
						}

				}
				else
				{
					CloseHandle(smtphd);
				}




	            cm.FreeMailHeader(mh);					
			
				infoPos = MailServerDB.ListNextRelayInfo(&relayinfo);
			
			}

			MailServerDB.CloseDB();



		}

		delete smtpclient;
	}

	
		//WaitForSingleObject(smtpsrv->RelayMailEvent,3 * 1000); //每半小時一次

		//Start Relay Mail
		//WIN32_FIND_DATA FindFileData;
		//HANDLE hFind = INVALID_HANDLE_VALUE;
		//char DirSpec[MAX_PATH + 1];  // directory specification

		//strcpy(DirSpec,RelayPath);
		//strcat(DirSpec,"*.eml");

		//strcpy(DirSpec,smtpsrv->ServerDB.PGPath);
		//strcat(DirSpec,"\\Relay\\*.eml");

		//strcpy(DirSpec,"c://Relay/*.eml");

		//_cprintf("RelayThread Go.. %s\r\n",DirSpec);

		//MailServerDB.OpenDB();

/*

		hFind = FindFirstFile(DirSpec, &FindFileData);

		if (hFind != INVALID_HANDLE_VALUE) 
		{
			do 
			{				
				//確認 retry //只 retry 5 天

				//FindFileData.ftCreationTime

				char DataFile[_MAX_PATH];	
				char EmailFile[_MAX_PATH];	
			
				strcpy(DataFile,RelayPath);				 
							
				strncat(DataFile,FindFileData.cFileName,strlen(FindFileData.cFileName)-3);			
				strcat(DataFile,"dat");

				strcpy(EmailFile,RelayPath);			 		
				strcat(EmailFile,FindFileData.cFileName);
				
				_cprintf("SendMail..Email:%s , Data:%s\r\n",EmailFile,DataFile);

				if (smtpsrv->Settings.RelayToServerIp[0] != 0)
				{
				
					char SendResult[255];
					SendResult[0] = 0;

					int rc = NO_ERROR;
					HANDLE smtphd = smtpclient->Send(smtpsrv->Settings.RelayToServerIp,smtpsrv->Settings.ServerName,EmailFile,DataFile,SendResult,&rc);
					if (WaitForSingleObject(smtphd,5 * 60 * 1000) == WAIT_OBJECT_0)
					{
					
						CloseHandle(smtphd);
						if (rc == NO_ERROR && SendResult[0] == 0)
						{
							//成功
							DeleteFile(EmailFile);
							DeleteFile(DataFile);

							_cprintf("Send OK %s\r\n",EmailFile);

						}
						else
						{
							_cprintf("Send ERROR %s\r\n",SendResult);
						}

					}
					else
					{
						CloseHandle(smtphd);
					}
				}


					//DeleteFile(EmailFile);
					//DeleteFile(DataFile);



			} while (FindNextFile(hFind, &FindFileData) != 0) ;
    
      
			FindClose(hFind);		
		
		}



	}

	

	delete smtpclient;

	}
	catch(...)
	{
		_cprintf("RelayMailThread ERROR........................%s\r\n");
	
	
	}
*/

	_endthreadex(0);
    return 0;
}