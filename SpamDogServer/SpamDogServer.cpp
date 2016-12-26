// SpamDogServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Swsocket2/CoreClass.h"
#include <stdio.h> 
#include <conio.h> 
#include <crtdbg.h>
#include "AntiSpamServer.h"
#include <stdlib.h>
#include <shellapi.h>
#include "Dbghelp.h"
//#include "ServerDB.h"
#include "../Swsocket2/SMTPClient.h"


LONG __stdcall TheCrashHandlerFunction ( EXCEPTION_POINTERS * pExPtrs )
{
	/*
	 char path_buffer[_MAX_PATH];
	 char drive[_MAX_DRIVE];
	 char dir[_MAX_DIR];
	 char fname[_MAX_FNAME];
	 char ext[_MAX_EXT];

	  
     HINSTANCE hInstance = GetModuleHandle(NULL);
     GetModuleFileName(hInstance, path_buffer, MAX_PATH);

	 _splitpath( path_buffer, drive, dir, fname, ext );

	 char DumpPath[_MAX_PATH];	 
	   

	   strcpy(DumpPath,drive);
	   strcat(DumpPath,dir);	   
	   strcat(DumpPath,"SpamDogCrash.dmp");
	  
	  HANDLE hMiniDumpFile = CreateFile(
		DumpPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL);
	  

	  MINIDUMP_EXCEPTION_INFORMATION eInfo;
      eInfo.ThreadId = GetCurrentThreadId();
      eInfo.ExceptionPointers = pExPtrs;
      eInfo.ClientPointers = TRUE;

	  MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hMiniDumpFile,
            MiniDumpWithFullMemory ,
            &eInfo,
            NULL,
            NULL);


	  CloseHandle(hMiniDumpFile);

	  
		*/
	  FILE *fp = fopen("C:\\CrashMail.eml","w+b");
	  unsigned int MailLen=0;
	  if (fp != NULL) 
	  {
		    char TempStr[50];
			strcpy(TempStr,"NSM Server Crash");
			fseek(fp,0,SEEK_SET);

			fwrite(TempStr,1,strlen(TempStr),fp);
			fseek(fp,0,SEEK_END);
			MailLen = ftell(fp);
	  }
	  fclose(fp);
	

	  _cprintf("SpamDog Server Crash!\n");

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

		/*
		smtphd =  smtpclient->Send("192.168.1.3",
								 "softworking.com",
								 "kenny@microbean.com.tw",
								 "kenny@ezfly.com",
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
		}*/

	  delete smtpclient;

	/*  FILE *fp = fopen("C:\\CrashLog.txt","w+b");	  
	  if (fp != NULL) 
	  {
			fseek(fp,0,SEEK_END);
			
			struct tm *newtime;
			long ltime;

			time( &ltime );

		
		 
		    newtime = localtime ( &ltime );
			char TimeStr[255];

			//char *TimeStr = asctime( newtime );
			//TimeStr[strlen(TimeStr) - 1] = 0; //trim /n

			strftime(TimeStr,255,"%a, %d %b %Y %H:%M:%S\r\n",newtime);

			fwrite(TimeStr,1,strlen(TimeStr),fp);
			fclose(fp);
	  }
	  

      ShellExecute(NULL, "open", "c:\\RestartSpamDogServer.bat", NULL, NULL, SW_SHOW);
*/
      return EXCEPTION_EXECUTE_HANDLER;//EXCEPTION_CONTINUE_SEARCH ;//EXCEPTION_EXECUTE_HANDLER
   
}


SERVICE_STATUS m_ServiceStatus;
SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
BOOL bRunning=true;

#ifdef _CONSOLEDBG
	HANDLE hConsole = NULL;    
#endif

BOOL InstallService()
{
  

     char strDir[MAX_PATH];
	    
     HINSTANCE hInstance = GetModuleHandle(NULL);
     GetModuleFileName(hInstance, strDir, MAX_PATH);

  HANDLE schSCManager,schService;
  //GetCurrentDirectory(1024,strDir);
  //strcat(strDir,"\\Srv1.exe"); 

  schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

  if (schSCManager == NULL) 
    return false;
  LPCTSTR lpszBinaryPathName=strDir;

  schService = CreateService(schSCManager,"SpamDog AntiSpam Professional Server", 
        "SpamDog AntiSpam Professional Server", // service name to display
     SERVICE_ALL_ACCESS, // desired access 
     SERVICE_WIN32_OWN_PROCESS, // service type 
     SERVICE_DEMAND_START, // start type 
     SERVICE_ERROR_NORMAL, // error control type 
     lpszBinaryPathName, // service's binary 
     NULL, // no load ordering group 
     NULL, // no tag identifier 
     NULL, // no dependencies
     NULL, // LocalSystem account
     NULL); // no password

  if (schService == NULL)
    return false; 

  CloseServiceHandle(schService);
  return true;
}

BOOL DeleteService()
{
  HANDLE schSCManager;
  SC_HANDLE hService;
  schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

  if (schSCManager == NULL)
    return false;
  hService=OpenService(schSCManager,"SpamDog AntiSpam Professional Server",SERVICE_ALL_ACCESS);
  if (hService == NULL)
    return false;
  if(DeleteService(hService)==0)
    return false;
  if(CloseServiceHandle(hService)==0)
    return false;

return true;
}

void WINAPI ServiceCtrlHandler(DWORD Opcode)
{
  switch(Opcode)
  {
    case SERVICE_CONTROL_PAUSE: 
      m_ServiceStatus.dwCurrentState = SERVICE_PAUSED;
      break;
    case SERVICE_CONTROL_CONTINUE:
      m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
      break;
    case SERVICE_CONTROL_STOP:
      m_ServiceStatus.dwWin32ExitCode = 0;
      m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
      m_ServiceStatus.dwCheckPoint = 0;
      m_ServiceStatus.dwWaitHint = 0;

      SetServiceStatus (m_ServiceStatusHandle,&m_ServiceStatus);
      bRunning=false;
      break;
    case SERVICE_CONTROL_INTERROGATE:
      break; 
  }
  return;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
  SetUnhandledExceptionFilter ( TheCrashHandlerFunction ) ;
	//DWORD status;
  //DWORD specificError;
  m_ServiceStatus.dwServiceType = SERVICE_WIN32;
  m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  m_ServiceStatus.dwWin32ExitCode = 0;
  m_ServiceStatus.dwServiceSpecificExitCode = 0;
  m_ServiceStatus.dwCheckPoint = 0;
  m_ServiceStatus.dwWaitHint = 0;

  m_ServiceStatusHandle = RegisterServiceCtrlHandler("SpamDog AntiSpam Professional Server", 
                                            ServiceCtrlHandler); 
  if (m_ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
  {
    return;
  }
  m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
  m_ServiceStatus.dwCheckPoint = 0;
  m_ServiceStatus.dwWaitHint = 0;
  if (!SetServiceStatus (m_ServiceStatusHandle, &m_ServiceStatus))
  {
  }

  bRunning=true;
  //Start Server
  CAntiSpamServer *smtpsrv = new CAntiSpamServer();
  smtpsrv->StartServer();

  //#ifdef _CONSOLEDBG
     _cprintf("Server Started\n");
  //#endif

  while(bRunning)
  {
    Sleep(3000);
    //Place Your Code for processing here....
  }

  delete smtpsrv;
  
  //#ifdef _CONSOLEDBG
     _cprintf("Server Stop\n");
  //#endif

  #ifdef _CONSOLEDBG
	FreeConsole();
	hConsole = NULL;
  #endif

  return;
}

void  RebuildMail(CServerDB *SrvDB, CServerDB *NewSrvDB , unsigned int AccountId, unsigned int NewAccountId , unsigned int FolderUID, char *TmpData , bool RebuildMailUId)
{

	 DBMailData MailData;
	 memset(&MailData , 0 ,sizeof(MailData));					   
					   
	 if (SrvDB->ListMail(AccountId , FolderUID , 0, &MailData) > 0)
	 {	
						  
			do
			{			   
							   
					FILE *MailTmpFp = NULL;
							   
					MailTmpFp = fopen(TmpData,"r+b");
					if (MailTmpFp == NULL)
					{
						MailTmpFp = fopen(TmpData,"w+b");
						 
					}

					fseek(MailTmpFp,0,SEEK_SET);
							   
					char *buff = new char [MailData.RawMailDataLen + 1];
					memset(buff,0,MailData.RawMailDataLen + 1);

					fseek(SrvDB->DBfp,MailData.RawMailDataPos,SEEK_SET);
					fread(buff,1,MailData.RawMailDataLen,SrvDB->DBfp);

					fwrite(buff,1,MailData.RawMailDataLen,MailTmpFp);



					char *ImapEnvStr = new char [MailData.ImapEnvelopeLen + 1];
					memset(ImapEnvStr,0,MailData.ImapEnvelopeLen + 1);

					fseek(SrvDB->DBfp,MailData.ImapEnvelopePos,SEEK_SET);
					fread(ImapEnvStr,1,MailData.ImapEnvelopeLen,SrvDB->DBfp);
						   
					//MailData
					unsigned int MailUId = 0;
					if (!RebuildMailUId)
					{
						MailUId = MailData.MailUId;
					}


					NewSrvDB->NewImapMail(NewAccountId,
										  FolderUID, //FolderId 暫定新舊 DB 不變 , 因為目前 Folder 不能自定
										  MailData.Status,
										  MailData.ImapInternalDate,
										  ImapEnvStr,
										  MailTmpFp,MailUId);

					delete buff;
					delete ImapEnvStr;
							   
					fclose(MailTmpFp);
						   
							    
			} while (SrvDB->ListNextMail(&MailData) > 0) ;				
					   
	 }				   

}

void  ListRebuildFolder(CServerDB *SrvDB, CServerDB *NewSrvDB ,MailFolderData *FolderData, unsigned int AccountId , unsigned int NewAccountId, char *TmpData,bool RebuildMailUId)
{

	 
	if (FolderData == NULL)
	{	
		//取第一個 FolderID
	   MailFolderData m_FolderData;
	   SrvDB->GetAccountFolderData(AccountId,0,&m_FolderData);
	   
	    
	   RebuildMail(SrvDB,NewSrvDB,AccountId,NewAccountId,m_FolderData.FolderUId,TmpData,RebuildMailUId);

	   ListRebuildFolder(SrvDB,NewSrvDB,&m_FolderData,AccountId,NewAccountId,TmpData,RebuildMailUId);
	}
	else
	{
		//先看右邊
		if (FolderData->SubMailFolderDataPos > 0)
		{
		
			MailFolderData r_FolderData ;
			SrvDB->GetAccountFolderDataPos(FolderData->SubMailFolderDataPos,&r_FolderData);
			//MailFolderData r_FolderData = MailServerDB->GetAccountFolderData(AccountId,FolderData->SubMailFolderDataPos));
			if (r_FolderData.FolderUId != 0)
			{
  			     //
				RebuildMail(SrvDB,NewSrvDB,AccountId,NewAccountId,r_FolderData.FolderUId,TmpData,RebuildMailUId);

				ListRebuildFolder(SrvDB,NewSrvDB,&r_FolderData,AccountId,NewAccountId,TmpData,RebuildMailUId);
			}

		}
		else if (FolderData->NextMailFolderDataPos > 0)
		{
			//MailFolderData n_FolderData = MailServerDB->GetAccountFolderData(AccountId,FolderData->NextMailFolderDataPos);
			MailFolderData n_FolderData ;
			SrvDB->GetAccountFolderDataPos(FolderData->NextMailFolderDataPos,&n_FolderData);

			    
			RebuildMail(SrvDB,NewSrvDB,AccountId,NewAccountId,n_FolderData.FolderUId,TmpData,RebuildMailUId);

	        ListRebuildFolder(SrvDB,NewSrvDB,&n_FolderData,AccountId,NewAccountId,TmpData,RebuildMailUId);
		}
	}
	 

}


int main(int argc, char* argv[])
{


  SetUnhandledExceptionFilter ( TheCrashHandlerFunction ) ;
  /*CHTTPClient *http = new CHTTPClient();

  char TempFile[1024];
  HANDLE hd = http->IPGET("192.168.1.2","www.softworking.com","/index.asp",TempFile);
  WaitForSingleObject(hd,INFINITE);

  delete http;

  printf("%s",TempFile);
  _getch();*/

  
#ifdef WITHSERVICE

  if(argc > 1)
  {
		if(strcmp(argv[1],"-i")==0)
		{
		  if(InstallService())
			printf("\n\nService Installed Sucessfully\n");
		  else
			printf("\n\nError Installing Service\n");
		} else
		if(strcmp(argv[1],"-d")==0)
		{
		  if(DeleteService())
			printf("\n\nService UnInstalled Sucessfully\n");
		  else
			printf("\n\nError UnInstalling Service\n");
		}
		else
		{
		  printf("\n\nUnknown Switch Usage\n\nFor Install use -i\n\nFor UnInstall use -d\n");
		}
  }
  else
  {

		#ifdef _CONSOLEDBG
			 AllocConsole();
			 hConsole = CreateFile("CONOUT$", GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			 SetConsoleMode(hConsole, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT );
			 SetConsoleTitle("SpamDog AntiSpam Professional Server Console");
		#endif

	
    SERVICE_TABLE_ENTRY DispatchTable[]=
                {{"SpamDog AntiSpam Professional Server",ServiceMain},{NULL,NULL}};
    StartServiceCtrlDispatcher(DispatchTable);

  }
#else

  //SetUnhandledExceptionFilter ( TheCrashHandlerFunction ) ;


  if(argc > 1)
  { 
	  if(strcmp(argv[1],"-R")==0 || strcmp(argv[1],"-r")==0)
	  {
		  
		    CRITICAL_SECTION  mDBSection;
			InitializeCriticalSection(&mDBSection);

		    RelayInfo relayinfo;
			memset(&relayinfo,0,sizeof(relayinfo));

			CServerDB *SrvDBCheck = new CServerDB(&mDBSection);
			SrvDBCheck->OpenDB();

			unsigned int infoPos = SrvDBCheck->ListFirstRelayInfo(&relayinfo);

			SrvDBCheck->CloseDB();
			delete SrvDBCheck;

			if (infoPos != 0)
			{
				
				printf("Couldn't Rebuild while still have relay mail in queue");
				DeleteCriticalSection(&mDBSection);
				return -1;			
			}


		  
		  //1. Rename All Old File
		   char path_buffer[_MAX_PATH];
		   char drive[_MAX_DRIVE];
		   char dir[_MAX_DIR];
		   char fname[_MAX_FNAME];
		   char ext[_MAX_EXT];

		   char PGPath[_MAX_PATH];
		   char DBPath[_MAX_PATH];
	       char DBFile[_MAX_PATH];
		   //char AccountDataFile[_MAX_PATH];
		   char TmpFile[_MAX_PATH];
		   char TmpFile2[_MAX_PATH];
		   char TmpFile3[_MAX_PATH];
		   char TmpFile4[_MAX_PATH];
		   char TmpFile5[_MAX_PATH];
		   

		   char TmpDBFile[_MAX_PATH];
		   
		   HINSTANCE hInstance = GetModuleHandle(NULL);
		   GetModuleFileName(hInstance, path_buffer, MAX_PATH);

		   _splitpath( path_buffer, drive, dir, fname, ext );	   

		   strcpy(PGPath,drive);
		   strcat(PGPath,dir);
		   
		   strcpy(DBPath,PGPath);
		   strcat(DBPath,"\\DB");		  

		   strcpy(TmpFile3,DBPath);
		   strcat(TmpFile3,"\\TmpMail.dat");

		   strcpy(DBFile,DBPath);
		   strcat(DBFile,"\\AntiSpamServer.db");
		   

		   strcpy(TmpDBFile,DBPath);
		   strcat(TmpDBFile,"\\AntiSpamServer.db.bak");
		   //刪除舊檔
		   DeleteFile(TmpDBFile);

		   rename(DBFile,TmpDBFile);

		   strcpy(TmpFile,DBPath);
		   strcat(TmpFile,"\\Account.db1"); 

		   strcpy(TmpFile2,DBPath);
		   strcat(TmpFile2,"\\Account.db1.bak"); 
		   //刪除舊檔
		   DeleteFile(TmpFile2);

		   rename(TmpFile,TmpFile2);

		   strcpy(TmpFile,DBPath);
		   strcat(TmpFile,"\\Account.db2"); 

		   strcpy(TmpFile2,DBPath);
		   strcat(TmpFile2,"\\Account.db2.bak"); 
		   //刪除舊檔
		   DeleteFile(TmpFile2);

		   rename(TmpFile,TmpFile2);

		   strcpy(TmpFile,DBPath);
		   strcat(TmpFile,"\\Account.db3"); 

		   strcpy(TmpFile2,DBPath);
		   strcat(TmpFile2,"\\Account.db3.bak"); 
		   //刪除舊檔
		   DeleteFile(TmpFile2);

		   rename(TmpFile,TmpFile2);

		   //strcpy(AccountDataFile,TmpFile2); //新的 db3

		   FILE *fp = NULL;
		   fp = fopen(TmpFile2,"r+b");
		   
		   fseek(fp,sizeof(SystemData),SEEK_SET);

		   
		   CServerDB *SrvDB = new CServerDB(&mDBSection,TmpDBFile,".bak");
		   CServerDB *NewSrvDB = new CServerDB(&mDBSection);

		   SrvDB->OpenDB();
		   NewSrvDB->OpenDB();


		   while (!feof(fp))
		   {
			   AccountData acc_data;
			   if (fread(&acc_data,1,sizeof(acc_data),fp) <= 0)
			   {
			      break;
			   }

			   printf("Trans %s \n",acc_data.ID);
			   //acc_data.AccountId
			   if (!acc_data.bDel && acc_data.ID[0] != 0)
			   {			   
				   unsigned int NewId = NewSrvDB->AddAccount(acc_data.ID);
				   //Rename Old SpamFile to New acc_data.AccountId

				   if (NewId != acc_data.AccountId)
				   {
					   char NewDBNum[50],OldDBNum[50];
					   itoa(NewId,NewDBNum,10);
					   itoa(acc_data.AccountId,OldDBNum,10);

					   strcpy(TmpFile4,DBPath);
					   strcat(TmpFile4,"\\");
					   strcat(TmpFile4,OldDBNum);
					   strcat(TmpFile4,"1.db");
					
					   strcpy(TmpFile5,DBPath);
					   strcat(TmpFile5,"\\");
					   strcat(TmpFile5,NewDBNum);
					   strcat(TmpFile5,"1.db");

					   rename(TmpFile4,TmpFile5);

					   strcpy(TmpFile4,DBPath);
					   strcat(TmpFile4,"\\");
					   strcat(TmpFile4,OldDBNum);
					   strcat(TmpFile4,"2.tmpdb");
					
					   strcpy(TmpFile5,DBPath);
					   strcat(TmpFile5,"\\");
					   strcat(TmpFile5,NewDBNum);
					   strcat(TmpFile5,"2.db");

					   rename(TmpFile4,TmpFile5);

					   strcpy(TmpFile4,DBPath);
					   strcat(TmpFile4,"\\");
					   strcat(TmpFile4,OldDBNum);
					   strcat(TmpFile4,"3.db");
					
					   strcpy(TmpFile5,DBPath);
					   strcat(TmpFile5,"\\");
					   strcat(TmpFile5,NewDBNum);
					   strcat(TmpFile5,"3.db");

					   rename(TmpFile4,TmpFile5);
				   }


				   //MailFolderData FData;
				   //memset(&FData,0,sizeof(FData));	 

				   //SrvDB->GetAccountFolderData(acc_data.AccountId,0,&FData);
				   bool MailUIdRebuild = false;
				   if (strcmp(argv[1],"-R")==0)
				   {
					   MailUIdRebuild = true;
				   }

				   ListRebuildFolder(SrvDB,NewSrvDB,NULL,acc_data.AccountId,NewId,TmpFile3,MailUIdRebuild);

				 			 
			   }
			   else
			   {

				   //刪除的 user 要刪除相關檔案
			   
			   }
		   }

		   SrvDB->CloseDB();
		   NewSrvDB->CloseDB();

		   delete SrvDB;
		   delete NewSrvDB;

		   DeleteFile(TmpFile3);


		   DeleteCriticalSection(&mDBSection);
		  
	  
	  }
  }

  CAntiSpamServer *smtpsrv = NULL;
  while(1)
  {
	  try
	  {
        smtpsrv = new CAntiSpamServer();
		smtpsrv->StartServer();
		_cprintf("Server Started\n");
		 _getch();
	  }
	  catch(...)
	  {
		  try
		  {
		   delete smtpsrv;        
		  } catch(...)
		  {}

		   _cprintf("Server Error\n");

          Sleep(1000 * 30);    
	  }
  }

  //#ifdef _CONSOLEDBG
   

   
   
    

	_cprintf("Exit Program...\r\n");
   delete smtpsrv;

  

#endif
  
	//InstallService();

	/*CSMTPServer *smtpsrv = new CSMTPServer();
	smtpsrv->StartServer();

	printf("<Press Any Key to Quit and Check Mem>\n");

	int ch = 1;
	while (ch != 'q')
	{
	  ch = _getch();

	  if (ch == 'c') system("cls");
	}



	
	delete smtpsrv;*/



	
	/*
	CIMAPServer *imapsrv = new CIMAPServer();
	imapsrv->StartServer();

	printf("<Press Any Key to Quit and Check Mem>\n");

	int ch = 1;
	while (ch != 'q')
	{
	  ch = _getch();

	  if (ch == 'c') system("cls");
	}



	
	delete imapsrv;*/

	/*
	CBaseClient *client = new CBaseClient();
	client->iniClient();
	//WSAEVENT waitevent = client->Connect(IPPROTO_TCP,"202.43.195.13",80,5000);
	//WSAEVENT waitevent = client->Connect(IPPROTO_UDP,"168.95.1.1",53,5000);
	HANDLE waitevent[2]; 
	waitevent[0] = client->Connect(IPPROTO_TCP,"192.168.2.31",80,5000);
	//waitevent[1] = client->Connect(IPPROTO_TCP,"168.95.4.10",25,5000);

	
		DWORD dwWaitResult = WaitForMultipleObjects(1,waitevent,false,INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0)
		{
			printf("Connect finish 1\r\n");
			 
		}
	

	
	delete client;
	*/

	/*CDnsClient *dns = new CDnsClient();
	char Result[255];
	char Result2[255];
	char Result3[255];
	HANDLE handle[3];

	memset(Result,0,255);
	memset(Result2,0,255);
	memset(Result3,0,255);

	handle[0] = dns->Resolve("168.95.1.1","61.62.70.233",qtPTR,Result);
	handle[1] = dns->Resolve("168.95.1.1","microbean.com.tw",qtMX,Result2);
	handle[2] = dns->Resolve("168.95.1.1","www.ezfly.com",qtA,Result3);
	//HANDLE handle = dns->Connect(IPPROTO_TCP,"192.168.2.31",80,5000);

	DWORD dwWaitResult = WaitForMultipleObjects(3,handle,true,INFINITE);
	if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_OBJECT_0 + 1 || dwWaitResult == WAIT_OBJECT_0 + 2)
	{
			//printf("Connect finish 1\r\n");

			printf("res1: %s \nres2: %s \nres3: %s \n",Result,Result2,Result3);
			 
	}
	//if (handle != NULL)
	//WaitForSingleObject(handle,INFINITE);

	

	delete dns;*/

	/*
	CBaseClientXP *client = new CBaseClientXP(IPPROTO_TCP,"202.43.195.13",80);	
	WSAEVENT waitevent[1];
	waitevent[0] = client->Connect();	
	
		DWORD dwWaitResult = WaitForMultipleObjects(1,waitevent,false,INFINITE);
		if (dwWaitResult == WAIT_OBJECT_0)
		{
			printf("Connect finish 1\r\n");
			 
		}
	

	
	delete client;*/


	/*
	CBaseClient  *base = new CBaseClient();
	base->iniClient();

	HANDLE handle = base->Connect(IPPROTO_UDP,"127.0.0.1",1670,1500,NULL);

	WaitForSingleObject(handle,INFINITE);
	delete base;*/


	//printf("<Press Any Key to Quit>\n");
	//CCoreSocket::Cleanup();
	//_CrtDumpMemoryLeaks();
	//_getch();
	return 0;
}


