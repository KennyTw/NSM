// IMAPServer.cpp: implementation of the CIMAPServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IMAPServer.h"
#include <stdlib.h>
#include <mbstring.h>
#include "AntiSpamserver.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIMAPServer::CIMAPServer(CRITICAL_SECTION*  mDBSection) : CBaseServer(IPPROTO_TCP,143)
{

	InitializeCriticalSection(&MailUIDSection);
	InitializeCriticalSection(&MailListSection);

	mRelayMailEvent = NULL;
	DBSection = mDBSection;

	MailServerDB = new CServerDB(mDBSection); 
	MailServerDB->OpenDB();

	 char path_buffer[_MAX_PATH];
	 char drive[_MAX_DRIVE];
	 char dir[_MAX_DIR];
	 char fname[_MAX_FNAME];
	 char ext[_MAX_EXT];


	   
       HINSTANCE hInstance = GetModuleHandle(NULL);
       GetModuleFileName(hInstance, path_buffer, MAX_PATH);

	   _splitpath( path_buffer, drive, dir, fname, ext );

	   
	   strcpy(PGPath,drive);
	   strcat(PGPath,dir); 

	  
	 m_ImapUserList = NULL; 
	 m_AccountList = NULL;


	 char IniPath[MAX_PATH];	 	  

	 strcpy(IniPath,PGPath);	      
	 strcat(IniPath,"SpamDogServer.ini");
	   
	 GetPrivateProfileString("ServerSetup","ADServerIp","",AD_IP,sizeof(AD_IP),IniPath);
	 GetPrivateProfileString("ServerSetup","ADLoginId","",ADLoginId,sizeof(ADLoginId),IniPath);
	 GetPrivateProfileString("ServerSetup","ADPassWord","",ADPassWord,sizeof(ADPassWord),IniPath);
	 GetPrivateProfileString("SysSetup","SpamDogCenterIp","114.32.69.18",SpamDogCenterIp,sizeof(SpamDogCenterIp),IniPath);

	 MaxNoSpamSize = GetPrivateProfileInt("ServerSetup","MaxNoSpamSize",1024*1024,IniPath); 
	 AccountCheckMode = GetPrivateProfileInt("ServerSetup","AccountCheck",1,IniPath);  //FILE mode be default  

	 ReadAccountFile();
}

CIMAPServer::~CIMAPServer()
{
	MailServerDB->CloseDB();

	EnterCriticalSection(&MailListSection);

	ImapUserList *ptr = m_ImapUserList;
	while (ptr)
	{	  
	     
		ImapUserList *nextptr = ptr->next;

		delete ptr;

		ptr = nextptr;	
	  
	}

	if (m_AccountList != NULL)
		DestoryAccountList();

	LeaveCriticalSection(&MailListSection);

	DeleteCriticalSection(&MailUIDSection);
	DeleteCriticalSection(&MailListSection);

	delete MailServerDB;
}

void CIMAPServer::ToImapString(char* InputStr)
{
	int len = strlen(InputStr);

	char *tmpStr = _strdup(InputStr);
	
	char tmpstrlen[255];
	itoa(len,tmpstrlen,10);

	strcpy(InputStr,"{");
	strcat(InputStr,tmpstrlen);
	strcat(InputStr,"}\r\n");
	strcat(InputStr,tmpStr);

	delete tmpStr;

}

/*
void CIMAPServer::ConvertToImapStr(char* InputStr)
{
	//可能會有寫入過大的問題

	unsigned int len = strlen(InputStr);
	
	char tmpstrlen[255];
	itoa(len,tmpstrlen,10);

	unsigned int intlen = strlen(tmpstrlen);
    


	char *NewBuf = new char[len + intlen + 1];

	strcpy(NewBuf,"{");
	strcat(NewBuf,tmpstrlen);
	strcat(NewBuf,"}\r\n");
	strcat(NewBuf,InputStr);

	strcpy(InputStr,NewBuf);

	delete NewBuf;
	

	




}*/

void CIMAPServer::HandleSocketMgr(SOCKET_OBJ *sock)
{
	closesocket(sock->s);
	sock->s = INVALID_SOCKET;		
}

void CIMAPServer::MailStatusToStr(unsigned int MailStatus , ImapStr *Astr)
{

	int ChangeCount = 0 ;

	if ((MailStatus & Flag_Recent)  == Flag_Recent)
	{		
		
		AddVarString("\\Recent",Astr);
		ChangeCount ++;
	}

	if ((MailStatus & Flag_Seen)  == Flag_Seen)
	{		
		

		if (ChangeCount > 0)
		{
		   AddVarString(" \\Seen",Astr);
		}
		else
		{
		   AddVarString("\\Seen",Astr);
		}

		ChangeCount ++;
	}
		
	if ((MailStatus & Flag_Deleted)  == Flag_Deleted)
	{		
		if (ChangeCount > 0)
		{
		   AddVarString(" \\Deleted",Astr);
		}
		else
		{
		   AddVarString("\\Deleted",Astr);
		}

		ChangeCount ++;
	}

	if ((MailStatus & Flag_Flagged)  == Flag_Flagged)
	{		
		if (ChangeCount > 0)
		{
		   AddVarString(" \\Flagged",Astr);
		}
		else
		{
		   AddVarString("\\Flagged",Astr);
		}

		ChangeCount ++;
	}

	if ((MailStatus & Flag_Answered)  == Flag_Answered)
	{		
		if (ChangeCount > 0)
		{
		   AddVarString(" \\Answered",Astr);
		}
		else
		{
		   AddVarString("\\Answered",Astr);
		}

		ChangeCount ++;
	}

	if ((MailStatus & Flag_Draft)  == Flag_Draft)
	{		
		if (ChangeCount > 0)
		{
		   AddVarString(" \\Draft",Astr);
		}
		else
		{
		   AddVarString("\\Draft",Astr);
		}

		ChangeCount ++;
	}


	if (ChangeCount == 0)
		AddVarString("\\Flagged",Astr);

}
void CIMAPServer::AddVarImapString(char* AppendStr , ImapStr* OldStr)
{

	int len = strlen(AppendStr) ;

	char tmpstrlen[255];
	itoa(len,tmpstrlen,10);
	
	int ImapLen = strlen(tmpstrlen) + len + 4 ; //4 => {} /r/n

	if (OldStr->size == 0)
	{
		OldStr->str = new char[255];
		memset(OldStr->str,0,255);
		OldStr->size = 255;
	}
	
	if (ImapLen + strlen(OldStr->str) < OldStr->size)
	{
		strcat(OldStr->str,"{");
		strcat(OldStr->str,tmpstrlen);
		strcat(OldStr->str,"}\r\n");
		strcat(OldStr->str,AppendStr);	
	}
	else
	{
		//space not enough
		int newsize = OldStr->size + ImapLen + 255;
		char *newchar = new char[newsize];
		strcpy(newchar,OldStr->str);

		strcat(newchar,"{");
		strcat(newchar,tmpstrlen);
		strcat(newchar,"}\r\n");
		strcat(newchar,AppendStr);	

		delete OldStr->str;
		OldStr->str = newchar;
		OldStr->size = newsize;
	
	}


}


void CIMAPServer::AddVarString(char* AppendStr , ImapStr* OldStr)
{
	if (OldStr->size == 0)
	{
		OldStr->str = new char[255];
		memset(OldStr->str,0,255);
		OldStr->size = 255;
	}
	
	if (strlen(AppendStr) + strlen(OldStr->str) < OldStr->size)
	{
		strcat(OldStr->str,AppendStr);	
	}
	else
	{
		//space not enough
		int newsize = OldStr->size + strlen(AppendStr) + 255;
		char *newchar = new char[newsize];
		strcpy(newchar,OldStr->str);
		strcat(newchar,AppendStr);

		delete OldStr->str;
		OldStr->str = newchar;
		OldStr->size = newsize;
	
	}



}

void CIMAPServer::AddString(char* StrBuffer , char* InputStr)
{

	int len = strlen(InputStr);
	
	char tmpstrlen[255];
	itoa(len,tmpstrlen,10);

	strcpy(StrBuffer,"{");
	strcat(StrBuffer,tmpstrlen);
	strcat(StrBuffer,"}\r\n");
	strcat(StrBuffer,InputStr);


}




void CIMAPServer::SelectMailDataFile(ImapMailData* md , char* MailFilePath ,char* User , char* MailBoxName)
{

 

	FILE *fp = NULL;

	fp = fopen(MailFilePath,"rb");
	if (fp != NULL)
	{
	
		fread(md,1,sizeof(ImapMailData),fp);
		fclose(fp);
	}

	

}

ImapMailData CIMAPServer::SelectMailData(unsigned int MailUID ,char* User , char* MailBoxName)
{

	char ImapFileName[255];
	char UID[255];

	itoa(MailUID,UID,10);
	
	ImapMailData md;
	memset(&md,0,sizeof(md));
	//NilMailData(&md);

	strcpy(ImapFileName,PGPath);
	strcat(ImapFileName,"Mail\\");
	strcat(ImapFileName,User);
	strcat(ImapFileName,"\\");
	strcat(ImapFileName,MailBoxName);
	strcat(ImapFileName,"\\");
	strcat(ImapFileName,UID);
	strcat(ImapFileName,".imp");


	SelectMailDataFile(&md,ImapFileName,User,MailBoxName);





	return md;

}

void CIMAPServer::ReAssignBoxSeq(unsigned int UserId , unsigned int FolderId)
{

	int SeqIdx = 0;

	DBMailData MailData;
	unsigned int MailDataPos = MailServerDB->ListMail(UserId,FolderId,0,&MailData);

	while (MailData.MailUId > 0)
	{		
		
		SeqIdx ++;

	//	if (SeqIdx >= FromSeq)
		//{
			//MailData.ImapSeqNo = SeqIdx;		
			MailServerDB->SetImapSeqNo(MailDataPos,SeqIdx);
		
		//}



	
		MailDataPos = MailServerDB->ListNextMail(&MailData);
	}
/*
	char FindPath[MAX_PATH];
	strcpy(FindPath,PGPath);
	strcat(FindPath,"Mail\\");
	strcat(FindPath,User);
	strcat(FindPath,"\\");
	strcat(FindPath,MailBoxName);
	strcat(FindPath,"\\*.imp");

	 WIN32_FIND_DATA FindFileData;
     HANDLE hFind;
	 

	 unsigned int NewSeq = 0;

	 hFind = FindFirstFile(FindPath,&FindFileData);
 	 if (hFind != INVALID_HANDLE_VALUE)
	 {
		 do
		 {			
			if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
			{
				NewSeq ++;

				if (NewSeq >= FromSeq)
				{

					char FindPathTmp[MAX_PATH];
					strcpy(FindPathTmp,PGPath);
					strcat(FindPathTmp,"Mail\\");
					strcat(FindPathTmp,User);
					strcat(FindPathTmp,"\\");
					strcat(FindPathTmp,MailBoxName);
					strcat(FindPathTmp,"\\");
					strcat(FindPathTmp,FindFileData.cFileName);				

					ImapMailData im;
					memset(&im,0,sizeof(im));
					//NilMailData(&im);
				 
					SelectMailDataFile(&im,FindPathTmp,User,MailBoxName);				

					if (im.MailUID != 0)
					{
						im.SeqNo = NewSeq;
						UpdateMailData(&im,User,MailBoxName);				
					}
				}


				
			}		
		 
		 }while(FindNextFile(hFind, &FindFileData) != 0);

		 
		 FindClose(hFind);
		
	 }
*/

}

BoxInfo CIMAPServer::CacuBoxInfo(char* User , char* MailBoxName)
{
//找出所有的信,並統計及付與 SEQNO

	BoxInfo bi = SelectBoxInfo(User,MailBoxName);
	//memset(&bi,0,sizeof(bi));
	bi.Exists = 0;
	bi.FirstUnseen = 0;
	bi.Recent = 0;
	bi.Seen = 0;
	bi.FirstMsgUID = 0;
	bi.LastMsgUID = 0;
	
	

	char FindPath[MAX_PATH];
	strcpy(FindPath,PGPath);
	strcat(FindPath,"Mail\\");
	strcat(FindPath,User);
	strcat(FindPath,"\\");
	strcat(FindPath,MailBoxName);
	strcat(FindPath,"\\*.imp");

	 WIN32_FIND_DATA FindFileData;
     HANDLE hFind;
	 

	 unsigned int NewSeq = 0;

	 hFind = FindFirstFile(FindPath,&FindFileData);
 	 if (hFind != INVALID_HANDLE_VALUE)
	 {
		 do
		 {			
			if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
			{
				NewSeq ++;

				char FindPathTmp[MAX_PATH];
				strcpy(FindPathTmp,PGPath);
				strcat(FindPathTmp,"Mail\\");
				strcat(FindPathTmp,User);
				strcat(FindPathTmp,"\\");
				strcat(FindPathTmp,MailBoxName);
				strcat(FindPathTmp,"\\");
				strcat(FindPathTmp,FindFileData.cFileName);				

				ImapMailData im;
				memset(&im,0,sizeof(im));
				//NilMailData(&im);
			 
				SelectMailDataFile(&im,FindPathTmp,User,MailBoxName);				

				if (im.MailUID != 0)
				{
					if (bi.FirstMsgUID == 0 || bi.FirstMsgUID > im.MailUID)
					{
						bi.FirstMsgUID = im.MailUID;
					}

					im.SeqNo = NewSeq;
					bi.Exists ++;
					
					if ((im.Status & Flag_Recent)  == Flag_Recent)
					{
						bi.Recent ++;

					///	if (bi.FirstUnseen == 0)
						//{
						//	bi.FirstUnseen = NewSeq;
					//	}
					} else if ((im.Status & Flag_Seen)  == Flag_Seen)
					{
							bi.Seen ++;

					///	if (bi.FirstUnseen == 0)
						//{
							bi.FirstUnseen = NewSeq;
					//	}
					}


					

					UpdateMailData(&im,User,MailBoxName);
					
					if (bi.LastMsgUID == 0 || bi.LastMsgUID < im.MailUID)
					bi.LastMsgUID = im.MailUID;
				
				}
				


				
			}		
		 
		 }while(FindNextFile(hFind, &FindFileData) != 0);

		 
		 FindClose(hFind);
		
	 }
	 

	 UpdateBoxInfo(&bi,User,MailBoxName);
	 return bi;

}
BoxInfo CIMAPServer::SelectBoxInfo(char* User , char* MailBoxName)
{

	BoxInfo bi;
	memset(&bi,0,sizeof(bi));

	char MailBoxPath[MAX_PATH];
	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\MailBox.dat");

	FILE  *fp=NULL;

	fp = fopen(MailBoxPath,"rb");
	if (fp != NULL)
	{
		fread(&bi,1,sizeof(bi),fp);
		fclose(fp);
	}

	
	

	return bi;



}

void CIMAPServer::UpdateBoxInfo(BoxInfo *bi,char* User , char* MailBoxName)
{
	char MailBoxPath[MAX_PATH];
	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\MailBox.dat");

	FILE  *fp=NULL;

	fp = fopen(MailBoxPath,"wb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_SET);
		fwrite(bi,1,sizeof(BoxInfo),fp);
		fclose(fp);
	}

	

}

void CIMAPServer::UpdateMailData(ImapMailData *im,char* User , char* MailBoxName)
{
	char ImapFileName[255];
	char UID[255];

	itoa(im->MailUID,UID,10);
	
	strcpy(ImapFileName,PGPath);
	strcat(ImapFileName,"Mail\\");
	strcat(ImapFileName,User);
	strcat(ImapFileName,"\\");
	strcat(ImapFileName,MailBoxName);
	strcat(ImapFileName,"\\");
	strcat(ImapFileName,UID);
	strcat(ImapFileName,".imp");

	FILE *fp = NULL;

	fp = fopen(ImapFileName,"wb");
	if (fp != NULL)
	{
	
		fwrite(im,1,sizeof(ImapMailData),fp);
		fclose(fp);
	}



}

void CIMAPServer::CreateBoxInfo(char* BoxPath)
{

	BoxInfo bi;
	memset(&bi,0,sizeof(bi));

	bi.UID = GetBoxUID();

	char MailBoxPath[MAX_PATH];
	strcpy(MailBoxPath,BoxPath);	 
	strcat(MailBoxPath,"\\"); 
	strcat(MailBoxPath,"\\MailBox.dat");

	FILE  *fp=NULL;

	fp = fopen(MailBoxPath,"wb");
	if (fp != NULL)
	{
		fwrite(&bi,1,sizeof(bi),fp);
		fclose(fp);
	}


}

 
void CIMAPServer::EXPUNGEMail(SOCKET_OBJ* sock)
{
	IMAPData *userdata = (IMAPData *) sock->UserData;	
	DBMailData MailData;
	MailServerDB->ListMail(userdata->DBAccountId,userdata->DBSelectedFolderId,0,&MailData);

	unsigned int DeleteSeq = 0;
	char UID[50]={0};
	char SEQ[50]={0};
	char TmpSend[255];


	while (MailData.MailUId > 0 )
	{	            				 
				
			//MailData.Status = MailData.Status & Flag_Deleted;		

			//MailServerDB->SetMailStatus(userdata->DBAccountId,userdata->DBSelectedFolderId,MailData.MailUId,MailData.Status);			
			if ((MailData.Status & Flag_Deleted) == Flag_Deleted)
			{
				itoa(MailData.ImapSeqNo-DeleteSeq,SEQ,10);
				DeleteSeq ++; 
									
				
				strcpy(TmpSend,"* ");
				strcat(TmpSend,SEQ);
				strcat(TmpSend," EXPUNGE");

										

				SendLn(sock,TmpSend);

				MailServerDB->DelMail(userdata->DBAccountId,userdata->DBSelectedFolderId,MailData.MailUId);			

				
			}

			MailServerDB->ListNextMail(&MailData);

	}

	if (DeleteSeq > 0)
	{
		ReAssignBoxSeq(userdata->DBAccountId,userdata->DBSelectedFolderId);
	
	}

	//MailServerDB->CleanDelMail(userdata->DBAccountId);
	//CleanDelMail(userdata);

}
void CIMAPServer::CleanDelMail(IMAPData *userdata)
{
	//return;
	
	DBMailData MailData;
	//MailServerDB->ListMail(userdata->DBAccountId,userdata->DBSelectedFolderId,0,&MailData);
	MailServerDB->ListMail(userdata->DBAccountId,Folder_SpamMail,0,&MailData);

	
	while (MailData.MailUId > 0 )
	{	     
		   	if ((MailData.Status & Flag_Deleted) != Flag_Deleted)
			{
				MailServerDB->SetMailStatus(userdata->DBAccountId,Folder_SpamMail,MailData.MailUId,Flag_Deleted);			
			}

			MailServerDB->DelMail(userdata->DBAccountId,Folder_SpamMail,MailData.MailUId);				

		   /* if (userdata->DelSpamFolder && userdata->DBSelectedFolderId == Folder_SpamMail)
			{			
				MailServerDB->SetMailStatus(userdata->DBAccountId,userdata->DBSelectedFolderId,MailData.MailUId,Flag_Deleted);
				MailServerDB->DelMail(userdata->DBAccountId,userdata->DBSelectedFolderId,MailData.MailUId);							
			}
			else				
			if ((MailData.Status & Flag_Deleted) == Flag_Deleted)
			{
				MailServerDB->DelMail(userdata->DBAccountId,userdata->DBSelectedFolderId,MailData.MailUId);				
			}*/

			MailServerDB->ListNextMail(&MailData);
	}

			  

	ReAssignBoxSeq(userdata->DBAccountId,userdata->DBSelectedFolderId);
/*  char FindPath[MAX_PATH];
				strcpy(FindPath,PGPath);
				strcat(FindPath,"Mail\\");
				strcat(FindPath,userdata->USER);
				strcat(FindPath,"\\");
				strcat(FindPath,userdata->SelectedBoxName);
				strcat(FindPath,"\\*.imp");

				 WIN32_FIND_DATA FindFileData;
				 HANDLE hFind;

				 hFind = FindFirstFile(FindPath,&FindFileData);
 				 if (hFind != INVALID_HANDLE_VALUE)
				 {
					 do
					 {			
						if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
						{
						

							char FindPathTmp[MAX_PATH];
							strcpy(FindPathTmp,PGPath);
							strcat(FindPathTmp,"Mail\\");
							strcat(FindPathTmp,userdata->USER);
							strcat(FindPathTmp,"\\");
							strcat(FindPathTmp,userdata->SelectedBoxName);
							strcat(FindPathTmp,"\\");
							strcat(FindPathTmp,FindFileData.cFileName);				

							ImapMailData im;
							memset(&im,0,sizeof(im));
							//NilMailData(&im);
						 
							SelectMailDataFile(&im,FindPathTmp,userdata->USER,userdata->SelectedBoxName);				

							if (im.MailUID != 0)
							{
								
								if ((im.Status & Flag_Deleted)  == Flag_Deleted)
								{
									char PathTmp[MAX_PATH];
									char UID[50];

									itoa(im.MailUID,UID,10);

									strcpy(PathTmp,PGPath);
									strcat(PathTmp,"Mail\\");
									strcat(PathTmp,userdata->USER);
									strcat(PathTmp,"\\");
									strcat(PathTmp,userdata->SelectedBoxName);
									strcat(PathTmp,"\\");
									strcat(PathTmp,UID);
									strcat(PathTmp,".eml");

									DeleteFile(PathTmp);

									strcpy(PathTmp,PGPath);
									strcat(PathTmp,"Mail\\");
									strcat(PathTmp,userdata->USER);
									strcat(PathTmp,"\\");
									strcat(PathTmp,userdata->SelectedBoxName);
									strcat(PathTmp,"\\");
									strcat(PathTmp,UID);
									strcat(PathTmp,".dat");

									DeleteFile(PathTmp);

									strcpy(PathTmp,PGPath);
									strcat(PathTmp,"Mail\\");
									strcat(PathTmp,userdata->USER);
									strcat(PathTmp,"\\");
									strcat(PathTmp,userdata->SelectedBoxName);
									strcat(PathTmp,"\\");
									strcat(PathTmp,UID);
									strcat(PathTmp,".ehd");

									DeleteFile(PathTmp);
									DeleteFile(FindPathTmp);
									
								}

															
							}
							


							
						}		
					 
					 }while(FindNextFile(hFind, &FindFileData) != 0);

					 
					 FindClose(hFind);
					
				 }*/

}

char *CIMAPServer::GetEnvelopeStr(FILE *MailFp)
{

	ImapStr istr;
	memset(&istr,0,sizeof(istr));

	fseek(MailFp,0,SEEK_END);
	int size = ftell(MailFp);
	fseek(MailFp,0,SEEK_SET);

	char * StrBuf = new char [size + 1];
	fread(StrBuf,1,size,MailFp);
	StrBuf[size] = 0;

	CEmail email;
	MailHeader *mh = email.GetMailHeader(StrBuf);

	if (mh->Subject[0] == 0 &&
		mh->From[0] == 0 &&
		mh->To[0] == 0)
	{
	
		//parser 無法 ident
		email.FreeMailHeader(mh);
		delete StrBuf;

		return NULL;
	
	}



						
	AddVarString("ENVELOPE (",&istr);

	//Date rewrite now
	struct tm *newtime;
	long ltime;

	time( &ltime );

	/* Obtain coordinated universal time: */
	newtime = gmtime( &ltime );
		 
	char TimeStr[255];
	strftime(TimeStr,255,"%a, %d %b %Y %H:%M:%S +0000",newtime);
						
	//if (mh->Date[0] == 0)
	//{	
	//	AddVarString("NIL ",&istr);
	//}
	//else
	//{
		AddVarString("\"",&istr);
	    AddVarString(TimeStr,&istr);
		AddVarString("\" ",&istr);	
	//}

						
	if (mh->Subject[0] == 0)
	{
	
		AddVarString("NIL ",&istr);
	}
	else
	{
				
		//Transfer To Imap String
		AddVarImapString(mh->Subject,&istr);		
		AddVarString(" ",&istr);

	 
	}

		RFCToEnvStruct(mh->From,&istr);
		RFCToEnvStruct(mh->Sender,&istr);
		RFCToEnvStruct(mh->Reply_To,&istr);
		RFCToEnvStruct(mh->To,&istr);
		RFCToEnvStruct(mh->Cc,&istr);
		RFCToEnvStruct(mh->Bcc,&istr);

						
		
		AddVarString("NIL ",&istr);


		if (mh->Message_Id[0] != 0)
		{		
				AddVarString("\"",&istr);
				AddVarString(mh->Message_Id,&istr);
				AddVarString("\") ",&istr);
		}
		else
		{		
		        AddVarString("NIL) ",&istr);
		}

				

		email.FreeMailHeader(mh);
		delete StrBuf;
							
		
		

	return istr.str; //由外部釋放

}

void CIMAPServer::SendIDLEResult(SOCKET_OBJ* sock)
{
	IMAPData *userdata = (IMAPData *) sock->UserData;

					MailFolderData m_FolderData;
					MailServerDB->GetAccountFolderData(userdata->DBAccountId,userdata->DBSelectedFolderId,&m_FolderData);
					
					

					char tmpcmd[255];
					tmpcmd[0] = 0;

					char charnum[20];

					ImapStr istr;
					memset(&istr,0,sizeof(istr));
					
					//AddVarString("* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft) \r\n",&istr);

					strcpy(tmpcmd,"* ");
					itoa(m_FolderData.Exists,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," EXISTS");
					strcat(tmpcmd,"\r\n");
					AddVarString(tmpcmd,&istr);
					//SendLn(sock,tmpcmd);
					

					
					strcpy(tmpcmd,"* ");
					itoa(m_FolderData.Recent,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," RECENT");
					//strcat(tmpcmd,"\r\n");
					AddVarString(tmpcmd,&istr);
					//SendLn(sock,tmpcmd);

					SendLn(sock,istr.str);
					

					delete istr.str;

					//再送 idel end
					//strcpy(tmpcmd,tag);
					//strcat(tmpcmd," OK IDLE terminated");

					//userdata->IDLEcmd[0] = 0;

					SendLn(sock,tmpcmd);



}

bool CIMAPServer::VerifyAccountFile(char* BoxAccount,char* Password,char* Account)
{

	IMAPAccountList *ptr = m_AccountList;
	while (ptr)
	{	  
	    
		if (strcmp(ptr->MailBoxAccount,BoxAccount) == 0 && strcmp(ptr->PassWord,Password) == 0)
		{ 
			
			strcpy(Account,ptr->Account);
			return true;
		}

		IMAPAccountList *nextptr = ptr->next;
		ptr =  nextptr;
	}

	return false;
}
/*
void CIMAPServer::GetAccount(_TCHAR* Email)
{
	_TCHAR *findpos = _tcsstr(Email,"@");

	if (findpos != NULL)
	{
	
		*findpos = 0;
	
	}


}*/

void CIMAPServer::ReadAccountFile()
{

       char FilePath[MAX_PATH];	 	  
	   char TmpStr[1024];
	  

	   strcpy(FilePath,PGPath);	      
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
					strncpy(EMailData, TmpStr + FindIdx , i - FindIdx + 1 );
				   }
				   else
				   {
				    strncpy(EMailData, TmpStr + FindIdx , i - FindIdx);
				   }

				   //printf("%s\r\n",EMailData);

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

				   
				   
				   InsertAccountList(Account,MailBoxAccount,PassWord);
				  

				   

				   FindIdx = i + 1;
			   }
			   
		   }	   
	   }

	   fseek(fp,0,SEEK_SET);
	   fclose(fp);


}

void CIMAPServer::InsertAccountList(char* Email,char *MailBoxAccount,char *PassWord)
{
	
	

	IMAPAccountList *end=NULL, 
               *ptr=NULL;

	IMAPAccountList  *obj = new IMAPAccountList;
	memset(obj,0,sizeof(IMAPAccountList));

	strcpy(obj->EMail,Email);
	//GetAccount(Email);
	strcpy(obj->Account,Email);	
	strcpy(obj->MailBoxAccount,MailBoxAccount);
	strcpy(obj->PassWord,PassWord);
	
    // Find the end of the list
    ptr = m_AccountList;
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
        m_AccountList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

 

}

void CIMAPServer::DestoryAccountList()
{

	IMAPAccountList *ptr = m_AccountList;
	while (ptr)
	{	  
	     
		IMAPAccountList *nextptr = ptr->next;

		delete ptr;
 

		ptr =  nextptr;
	}
}


void CIMAPServer::AddImapAppendMail(IMAPData *imapdata,char* Flag ,char* InternalDate , char* MailBoxName )
{
	
	char InComingPath[_MAX_PATH];
	char TempFileName[_MAX_PATH];

	strcpy(InComingPath,PGPath);
	strcat(InComingPath,"Incoming");

	GetTempFileName(InComingPath, // directory for temp files 
						"IMAP",                    // temp file name prefix 
						0,                        // create unique name 
						TempFileName);              // buffer for name 		
	
		
//	strcpy(imapdata->RecvDataFilePath,InComingPath);
//	strcat(imapdata->RecvDataFilePath,"\\");
	strcpy(imapdata->RecvDataFilePath,TempFileName);

	imapdata->Recvfp = fopen(TempFileName,"r+b");

	if (InternalDate != NULL)
		strcpy(imapdata->InternalDate,InternalDate);

	//Select Box
	
	strcpy(imapdata->SelectedBoxName,MailBoxName);
			//從 Mail DB 取出
	MailFolderData m_FolderData;
	MailServerDB->GetAccountFolderNameData(imapdata->DBAccountId,imapdata->SelectedBoxName,&m_FolderData);
	imapdata->DBSelectedFolderId = m_FolderData.FolderUId;



	
	imapdata->Status = Flag_Recent;

	if (Flag != NULL)
	{
		ParseData pData;
		memset(&pData,0,sizeof(pData));
		ToParseData(Flag,&pData);

		for (int i = 0 ; i < pData.DataCount ; i ++)
		{
			if (stricmp(pData.Data[i],"\\Recent") == 0)
			{			
				imapdata->Status  |= Flag_Recent;
				
			}
			else if (stricmp(pData.Data[i],"\\Seen") == 0)
			{			
				imapdata->Status  |= Flag_Seen;
				
			}
			else if (stricmp(pData.Data[i],"\\Deleted") == 0)
			{
			
				imapdata->Status  |= Flag_Deleted;
				
			}
			else if (stricmp(pData.Data[i],"\\Answered") == 0)
			{
			
				imapdata->Status  |= Flag_Answered;
				
			}
			else if (stricmp(pData.Data[i],"\\Flagged") == 0)
			{
			
				imapdata->Status  |= Flag_Flagged;
				
			}
			else if (stricmp(pData.Data[i],"\\Draft") == 0)
			{
			
				imapdata->Status  |= Flag_Draft;
				
			}
		
		}
	
	
	}


	/*
	
	unsigned int MUid =  GetMailUID();

	//Create Mail File
	char numchar[25];
	ltoa(MUid,numchar,10);

	ImapMailFp fp;
	memset(&fp,0,sizeof(fp));	

	char MailBoxPath[MAX_PATH];
	
	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,numchar);
	strcat(MailBoxPath,".eml.tmp");
	
	
	fp.EmlFp = fopen(MailBoxPath,"w+b");
 
	
	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,numchar);
	strcat(MailBoxPath,".ehd.tmp");

	
	fp.EhdFp = fopen(MailBoxPath,"w+b");	
 

	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,numchar);
	strcat(MailBoxPath,".dat.tmp");


	fp.DatFp = fopen(MailBoxPath,"w+b");


	strcpy(MailBoxPath,PGPath);
	strcat(MailBoxPath,"Mail\\");
	strcat(MailBoxPath,User);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,MailBoxName);
	strcat(MailBoxPath,"\\");
	strcat(MailBoxPath,numchar);
	strcat(MailBoxPath,".imp.tmp");

	ImapMailData impdata;
	memset(&impdata,0,sizeof(impdata));

	if (InternalDate != NULL)
		strcpy(impdata.InternalDate,InternalDate);
	//NilMailData(&impdata);
	
	impdata.MailUID = MUid;
//	strcpy(impdata.RemoteIp,RemoteIp);
	impdata.Status = Flag_Recent;


	if (Flag != NULL)
	{
		ParseData pData;
		memset(&pData,0,sizeof(pData));
		ToParseData(Flag,&pData);

		for (int i = 0 ; i < pData.DataCount ; i ++)
		{
			if (stricmp(pData.Data[i],"\\Recent") == 0)
			{			
				impdata.Status |= Flag_Recent;
				
			}
			else if (stricmp(pData.Data[i],"\\Seen") == 0)
			{			
				impdata.Status |= Flag_Seen;
				
			}
			else if (stricmp(pData.Data[i],"\\Deleted") == 0)
			{
			
				impdata.Status |= Flag_Deleted;
				
			}
			else if (stricmp(pData.Data[i],"\\Answered") == 0)
			{
			
				impdata.Status |= Flag_Answered;
				
			}
			else if (stricmp(pData.Data[i],"\\Flagged") == 0)
			{
			
				impdata.Status |= Flag_Flagged;
				
			}
			else if (stricmp(pData.Data[i],"\\Draft") == 0)
			{
			
				impdata.Status |= Flag_Draft;
				
			}
		
		}
	
	
	}
	


 
	fp.ImpFp = fopen(MailBoxPath,"w+b");
	if (fp.ImpFp != NULL)
	{
		fwrite(&impdata,1,sizeof(impdata),fp.ImpFp);
		fclose(fp.ImpFp);
		fp.ImpFp = NULL;
	}
	


	return fp;*/

}

void CIMAPServer::SetRelayMailEvent(HANDLE RelayMailEvent)
{
	mRelayMailEvent = RelayMailEvent;
}

bool CIMAPServer::SendStatus(unsigned int AccountId , unsigned int FolderId)
{

	bool BeSend = false;
	EnterCriticalSection(&MailListSection);
	ImapUserList *obj = SearchAccountIdList(AccountId);

		if (obj != NULL)
		{
			//if (obj->sock->s != INVALID_SOCKET)
			//{
				IMAPData *userdata = (IMAPData *) obj->sock->UserData;

				if (userdata->DBSelectedFolderId == FolderId)
				{
					
					MailFolderData m_FolderData;
					MailServerDB->GetAccountFolderData(AccountId,FolderId,&m_FolderData);
					
					//BoxInfo bi = CacuBoxInfo(User,MailBoxName);
  
					/*char tmpcmd[255];
					tmpcmd[0] = 0;

					char charnum[20]; 

					strcpy(tmpcmd,"* ");
					itoa(FolderData.Exists,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," EXISTS");
					SendLn(obj->sock,tmpcmd);
					

					strcpy(tmpcmd,"* ");
					itoa(FolderData.Recent,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," RECENT");
					SendLn(obj->sock,tmpcmd);*/

					char tmpcmd[255];
					tmpcmd[0] = 0;

					char charnum[20];

					ImapStr istr;
					memset(&istr,0,sizeof(istr));
					
					//AddVarString("* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft) \r\n",&istr);

					strcpy(tmpcmd,"* ");
					itoa(m_FolderData.Exists,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," EXISTS");
					strcat(tmpcmd,"\r\n");
					AddVarString(tmpcmd,&istr);
					//SendLn(sock,tmpcmd);
					

					
					strcpy(tmpcmd,"* ");
					itoa(m_FolderData.Recent,charnum,10);
					strcat(tmpcmd,charnum);
					strcat(tmpcmd," RECENT");
					//strcat(tmpcmd,"\r\n");
					AddVarString(tmpcmd,&istr);
					//SendLn(sock,tmpcmd);

					SendLn(obj->sock,istr.str);

					delete istr.str;

					BeSend = true;

				

				 
				}
		//	}

		}

			LeaveCriticalSection(&MailListSection);
 

			return BeSend;

}

void CIMAPServer::InsertQueSend(char* AStr , ImapQueSend **qSend)
{
	ImapQueSend *end=NULL, 
               *ptr=NULL;

	ImapQueSend  *obj = new ImapQueSend;
	obj->cmd = AStr;
	
    // Find the end of the list
    ptr = *qSend;
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
        *qSend = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

}
void CIMAPServer::RemoveQueSend(ImapQueSend *obj , ImapQueSend **qSend)
{
	if (*qSend != NULL)
    {
        // Fix up the next and prev pointers
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;

        if (*qSend == obj)
            (*qSend) = obj->next;

		delete obj->cmd;
		delete obj;
    }

}

char* CIMAPServer::GetInternalDate()
{

	struct tm *newtime;
	long ltime;
	time( &ltime );

	/* Obtain coordinated universal time: */
	newtime = gmtime( &ltime );		 

	char *TimeStr = new char[255];
	strftime(TimeStr,255,"%a, %d %b %Y %H:%M:%S +0000",newtime);

	return TimeStr; //需由外部釋放
}




void CIMAPServer::TrimQuote(char *AStr)
{
	int len = strlen(AStr);

	for (int i = 0 ; i < len ; i ++)
	{
	
		if (AStr[i] == '"')
		{
			if ( i == len - 1)
			{
			  AStr[len-1] = 0;
			}
			else
			{
				memmove(AStr+i,AStr+i+1,len-i-1);
				AStr[len-1] = 0;
			}
				
			len--;
			i --;
		   
		}

	}

}

void CIMAPServer::TrimParenthesis(char *AStr)
{
	int len = strlen(AStr);
	
	if (AStr[0] == '(')
	{
		memmove(AStr,AStr+1,len-2);
		AStr[len-2] = 0;
	   
	}

}

bool CIMAPServer::ExecuteCmd(char* Cmd ,char* MailList, char* ImapQuery , unsigned int UserId , unsigned int FolderId, SOCKET_OBJ* sock )
{
					//zxmm UID STORE 50:53,57:61,65:70 +FLAGS.SILENT (\Seen)
	
			char* StoreType = NULL;

			if (strnicmp(Cmd , "STORE",5) == 0)
			{

				StoreType = Cmd + 6;

			}
			/*else if (strnicmp(Cmd,"COPY",4) == 0)
			{
			
				StoreType = Cmd + 5;
			}*/


			int OkCount = 0;

	
			MailFolderData FolderData ;
			MailServerDB->GetAccountFolderData(UserId,FolderId,&FolderData);
			//BoxInfo bi = SelectBoxInfo(User,MailBoxName);

			
			if (FolderData.Exists <= 0)
			{
				return false;
			}

			ParseData pData;
			memset(&pData,0,sizeof(pData));

			ToParseSeqData(MailList,&pData);

			for (int i = 0 ; i < pData.DataCount ; i++)
			{

	
					char* pos = strstr(pData.Data[i],":");
					char FromUID[50]={0};
					char ToUID[50]={0};
					unsigned int FromiUID = 0;
					unsigned int ToiUID = 0;

						
					if (pos == NULL)
					{
						FromiUID = atoi(pData.Data[i]);						
						ToiUID =  FromiUID;

						//OutLook 特例
						/*if (FromiUID == 0)
						{
							FromiUID = FolderData.FirstMailUID;
							ToiUID =  FromiUID;						
						}*/
					}
					else
					{
												
						strncpy(FromUID,pData.Data[i],(pos - pData.Data[i]));
						strncpy(ToUID,pos+1,strlen(pData.Data[i]) - (pos - pData.Data[i]));

						if (ToUID[0] == '*')
						{
							
							ToiUID = FolderData.LastMailUID;
						}
						else
						{
							ToiUID = atoi(ToUID);	
						}
						
						FromiUID =  atoi(FromUID);
						

					}				

					bool Ok = false;

					if (FromiUID < FolderData.FirstMailUID || FromiUID > ToiUID)
					{
						FromiUID = FolderData.FirstMailUID;

						if (FromiUID > ToiUID)
							ToiUID = FolderData.LastMailUID;
					}

				 
 
					

					if (stricmp(Cmd , "FETCH") == 0)
					{
								Ok = FetchMail(FromiUID,ToiUID,ImapQuery,UserId,FolderId,sock);
					}
					else if (strnicmp(Cmd , "STORE",5) == 0)
					{
								
								Ok = StoreMail(FromiUID,ToiUID,StoreType,ImapQuery,UserId,FolderId,sock);						
					} 
					else if (strnicmp(Cmd , "COPY",4) == 0)
					{
						        //由 imapquery (to box name 轉 To FolderId
								MailFolderData FolderData;
								memset(&FolderData,0,sizeof(FolderData));								

								MailServerDB->GetAccountFolderNameData(UserId,ImapQuery,&FolderData);
						        unsigned int ToFolderId = FolderData.FolderUId;
								Ok = CopyMail(FromiUID,ToiUID,UserId,FolderId,ToFolderId,sock);
							
					}
					else if (strnicmp(Cmd , "SEARCH",5) == 0)
					{
					   Ok =	SearchMail(FromiUID,ToiUID,ImapQuery,UserId,FolderId,sock);

					}

					
					if (Ok)
					{						  
							   OkCount++;
					}



				/*
						for (int j = FromiUID ; j <= ToiUID ; j++)
						{
						
							

							if (stricmp(Cmd , "FETCH") == 0)
							{
								Ok = FetchMail(j,ImapQuery,User,MailBoxName,sock);
							}
							else if (strnicmp(Cmd , "STORE",5) == 0)
							{
								
								Ok = StoreMail(StoreType,j,ImapQuery,User,MailBoxName,sock);						
							} 
							else if (strnicmp(Cmd , "COPY",4) == 0)
							{
								Ok = CopyMail(j,User,MailBoxName,ImapQuery,sock);
							
							}
							if (Ok)
							{						  
							   OkCount++;
							}
						}
				 */

			}
			

					if (OkCount > 0)
					{
						return true;
					}
					else
					{
						return false;
					}


	
					

}

bool CIMAPServer::CopyMail(unsigned int FromMailUId,unsigned int ToMailUId  ,unsigned int UserId, unsigned int FromFolderId , unsigned int ToFolderId , SOCKET_OBJ* sock )
{
	
	IMAPData *userdata = (IMAPData *) sock->UserData;	
	DBMailData dbMailData;
	MailServerDB->ListMail(UserId,FromFolderId,FromMailUId,&dbMailData);

	while (dbMailData.MailUId > 0 && dbMailData.MailUId <= ToMailUId)
	{

		if (ToFolderId == Folder_LearnOk)
		{		
		
			//異動 Spam Data
			if (dbMailData.RawMailDataLen <= MaxNoSpamSize)
			{
				char *MailBuffer = MailServerDB->RetriveMail(&dbMailData);
			
				char *MailText = new char[strlen(MailBuffer)+1];
				char *MailContent = new char[strlen(MailBuffer)+1];

				MailText[0] = 0;
				MailContent[0] = 0;

							
				CEmail cem;						
							
				MailData *mb = cem.GetMailData(MailBuffer);
				MailHeader *mh = cem.GetMailHeader(mb->mailHeader);


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

											CMailCodec ccd;	
											char *TmpBuffer = new char[strlen(out) + 1];

											ccd.TrimHTML(out,TmpBuffer);
											strcat(MailText,TmpBuffer);

											delete TmpBuffer;

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
				itoa(userdata->DBAccountId,DBNum,10);

				CChecker cc(DBSection,DBNum,"SpamDogServer.ini");

				double SpamRate = MailServerDB->CountSpamRate(&cc,
			 												  MailContent,
															  MailText,
															  mh->From,
															  mh->Subject,
															  mh->SpamDogXMailer);
				
						
				//直接 update spam db 不放入 ServerDB
				MailServerDB->UpdateSpamRate(&cc,1,-1,false);

				if (mh->SpamDogRemoteIp[0] != 0)
					  cc.MainTainRBL(SpamDogCenterIp ,false,mh->SpamDogRemoteIp);
			

				
				delete MailText;
				delete MailContent;

				cem.FreeMailHeader(mh);
				cem.FreeMailData(mb);



				MailServerDB->FreeMail(MailBuffer);
			}

			
			
			//不搬移 , 但要處理 relay ....
		
			MailServerDB->RelayDBMail(UserId,FromFolderId,dbMailData.MailUId);
			MailServerDB->SetMailStatus(UserId,FromFolderId,dbMailData.MailUId,Flag_Deleted);
			
			
			
			//MailServerDB->DelMail(UserId,FromFolderId,MailData.MailUId);
			
			if (mRelayMailEvent != NULL)
			  SetEvent(mRelayMailEvent);
		}
		else
		{		
			MailServerDB->MoveMail(UserId,FromFolderId,dbMailData.MailUId,ToFolderId);
		}
		
		MailServerDB->ListNextMail(&dbMailData);		
	}



	return true;
}

bool CIMAPServer::StoreMail(unsigned int FromMailUId,unsigned int ToMailUId , char* StoreType,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock )
{
	IMAPData *userdata = (IMAPData *) sock->UserData;	
	DBMailData MailData ;
	MailServerDB->ListMail(UserId,FolderId,FromMailUId,&MailData);

	ParseData pData;
	memset(&pData , 0 , sizeof(pData));

	ToParseData(ImapQuery,&pData);

	while (MailData.MailUId > 0 && MailData.MailUId <= ToMailUId)
	{

		//BoxInfo bi = SelectBoxInfo(User,MailBoxName);
		//ImapMailData im = SelectMailData(MailUID,User,MailBoxName);  

		
		if (stricmp(StoreType,"FLAGS") == 0)
		{
			//Replace status
			MailData.Status = 0;

			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
					
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Seen;
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Deleted;
					
				}				
			
			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
			
		
			
		
		} else if (stricmp(StoreType,"FLAGS.SILENT") == 0)
		{
			 // Replace status
			MailData.Status = 0;

			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
					
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Seen;
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Deleted;
					
				}				
			
			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
			
			ImapStr istr;	
			memset(&istr,0,sizeof(istr));

			AddVarString("* ",&istr);

			char sUid[50];
			char sSeq[50];

			itoa(MailData.MailUId,sUid,10);
			itoa(MailData.ImapSeqNo,sSeq,10);

			AddVarString(sSeq ,&istr);
			AddVarString(" FETCH (FLAGS (",&istr);
			MailStatusToStr(MailData.Status,&istr);
			AddVarString(") UID ",&istr);
			AddVarString(sUid,&istr);
			AddVarString(")",&istr);


			InsertQueSend(istr.str,&userdata->que);
		
		} else if (stricmp(StoreType,"+FLAGS") == 0)
		{
			// Add Status
			
			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
					
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Seen;
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Deleted;
					
				}			
				
			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
			
		
			ImapStr istr;	
			memset(&istr,0,sizeof(istr));

			AddVarString("* ",&istr);

			char sUid[50];
			char sSeq[50];

			itoa(MailData.MailUId,sUid,10);
			itoa(MailData.ImapSeqNo,sSeq,10);

			AddVarString(sSeq ,&istr);
			AddVarString(" FETCH (FLAGS (",&istr);
			MailStatusToStr(MailData.Status,&istr);
			AddVarString(") UID ",&istr);
			AddVarString(sUid,&istr);
			AddVarString(")",&istr);


			InsertQueSend(istr.str,&userdata->que);
		
		} else if (stricmp(StoreType,"+FLAGS.SILENT") == 0)
		{
		   // Add Status

			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
					
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Seen;
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
					MailData.Status &= (~Flag_Recent);
					MailData.Status |= Flag_Deleted;
					
				}
				
				//UpdateMailData(&im,User,MailBoxName);
			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
			
		
		
		} else if (stricmp(StoreType,"-FLAGS") == 0)
		{
			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
				
					MailData.Status &= (~Flag_Seen);
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
				
					MailData.Status &= (~Flag_Deleted);
					
				}
			
			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
		
			ImapStr istr;	
			memset(&istr,0,sizeof(istr));

			AddVarString("* ",&istr);

			char sUid[50];
			char sSeq[50];

			itoa(MailData.MailUId,sUid,10);
			itoa(MailData.ImapSeqNo,sSeq,10);

			AddVarString(sSeq ,&istr);
			AddVarString(" FETCH (FLAGS (",&istr);
			MailStatusToStr(MailData.Status,&istr);
			AddVarString(") UID ",&istr);
			AddVarString(sUid,&istr);
			AddVarString(")",&istr);


			InsertQueSend(istr.str,&userdata->que);

		
		} else if (stricmp(StoreType,"-FLAGS.SILENT") == 0)
		{
			//ParseData pData;
			//memset(&pData , 0 , sizeof(pData));

			//ToParseData(ImapQuery,&pData);

			for (int i = 0 ; i < pData.DataCount ; i ++)
			{
			
				if (stricmp(pData.Data[i],"\\Seen") == 0)
				{
					//im.Status &= (~Flag_Recent);
					//im.Status |= Flag_Seen;
					//im.Status |= Flag_Recent;
					MailData.Status &= (~Flag_Seen);
					
				}
				else if (stricmp(pData.Data[i],"\\Deleted") == 0)
				{
					//im.Status &= (~Flag_Recent);
					//im.Status |= Flag_Deleted;
					MailData.Status &= (~Flag_Deleted);
					
				}
				
				//UpdateMailData(&im,User,MailBoxName);
			

			}

			MailServerDB->SetMailStatus(UserId,FolderId,MailData.MailUId,MailData.Status);
		
		}

		MailServerDB->ListNextMail(&MailData);

			
		
	}


	/*char* SendEndCmd = new char[250];
	strcpy(SendEndCmd,TagCode);
	strcat(SendEndCmd," OK UID STORE completed");	
	
	SendLn(sock,SendEndCmd);
	delete SendEndCmd;*/
	
	return true;
}
void CIMAPServer::RFCToEnvStruct(char* RFCbuf,ImapStr* istr)
{

				    if (RFCbuf[0] == 0)
					{
						//fprintf(fp,"((NIL NIL NIL NIL)) ");
						AddVarString("((NIL NIL NIL NIL)) ",istr);
					}
					else
					{
						//fprintf(fp,"(");
						AddVarString("(",istr);
						//Transfer To Imap String
						char *tmp2 = new char[strlen(RFCbuf) + 10];
						memset(tmp2,0,strlen(RFCbuf) + 10);
						char *tmp3 = new char[strlen(RFCbuf) + 10];
						memset(tmp3,0,strlen(RFCbuf) + 10);

						ParseData pData;
						memset(&pData , 0 , sizeof(pData));

						char *RFCDupBuf = _strdup(RFCbuf);
						ToParseMailListData(RFCDupBuf,&pData);

						for (int i = 0 ; i < pData.DataCount ; i ++)
						{

							    TrimQuote(pData.Data[i]);
								//TrimQuote(pData.Data[i]);
						 
								/*char* pos1 = (char *) _mbsstr((unsigned char *)pData.Data[i],(unsigned char *)"\"");
								if (pos1 != NULL)
								{
								
									//Not Well Form
									AddVarString("(NIL NIL NIL NIL)",istr);
									continue ;
								
								}*/

								CEmail email;
								email.SplitEmailAddress(pData.Data[i],tmp2,tmp3);


								if (tmp2[0] == 0)
								{
								

									char* pos1 = (char *) _mbsstr((unsigned char *)pData.Data[i],(unsigned char *)"@");
									char* pos2 = (char *) _mbsstr((unsigned char *)pData.Data[i],(unsigned char *)",");
								

									if (pos1 != NULL && pos2 == NULL)
									{
										*pos1 = 0;
										//fprintf(fp,"(NIL NIL \"%s\" \"%s\")",pData.Data[i],pos1+1);
										AddVarString("(NIL NIL \"",istr);
										AddVarString(pData.Data[i],istr);
										AddVarString("\" \"",istr);
										AddVarString(pos1+1,istr);
										AddVarString("\")",istr);
									}
									else
									{							
										//fprintf(fp,"(NIL NIL NIL NIL)");									
										AddVarString("(",istr);
										AddVarImapString(pData.Data[i],istr);
										AddVarString(" NIL NIL NIL)",istr);
										
									}
								} 
								else
								{
									//ToImapString(tmp2);
								

									char* pos1 = strstr(tmp3,"@");							

									if (pos1 != NULL)
									{
										*pos1 = 0;
										//fprintf(fp,"(%s NIL \"%s\" \"%s\")",tmp2,tmp3,pos1+1);

										AddVarString("(",istr);
										AddVarImapString(tmp2,istr);
										AddVarString(" NIL \"",istr);
										AddVarString(tmp3,istr);
										AddVarString("\" \"",istr);
										AddVarString(pos1+1,istr);
										AddVarString("\")",istr);

									}
									else
									{							
										//fprintf(fp,"(%s NIL NIL NIL)",tmp2);
										AddVarString("(",istr);
										AddVarImapString(tmp2,istr);
										AddVarString(" NIL NIL NIL)",istr);
									}


								}
						}

						//fprintf(fp,") ");
						AddVarString(") ",istr);

					
						delete tmp2;
						delete tmp3;

						delete RFCDupBuf;

					}


}

bool CIMAPServer::FetchMail(unsigned int FromMailUId , unsigned int ToMailUId ,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock )
{
	IMAPData *userdata = (IMAPData *) sock->UserData;	
	DBMailData MailData;
	MailServerDB->ListFromMail(UserId,FolderId,FromMailUId,&MailData);

	
	while (MailData.MailUId > 0 && MailData.MailUId <= ToMailUId)
	{
	
		
		char *DupStr = strdup(ImapQuery);

		
		ParseData mpData;
		memset(&mpData , 0 , sizeof(mpData));

		ToParseData(DupStr,&mpData);

		char UID[255];
		itoa(MailData.MailUId,UID,10);
		

		ImapStr istr;
		memset(&istr,0,sizeof(istr));
						

		char sSEQNO[50];
		itoa(MailData.ImapSeqNo,sSEQNO,10);
	
		AddVarString("* ",&istr);		
		AddVarString(sSEQNO,&istr);	
		AddVarString(" FETCH (",&istr);


		for (int i = 0 ; i < mpData.DataCount ; i ++)
		{						
				
		 

		//	if (strnicmp(mpData.Data[i],"BODY",4) == 0  )
			if (strnicmp(mpData.Data[i],"BODY",4) == 0 )
			{
				bool bPeek = false;
				bool AllData = false;

				char *Buffer =  new char[1];
				Buffer[0] = 0;

				

				if (strnicmp(mpData.Data[i]+4,"[",1) == 0)
				{
					//BODY[
					bPeek = false;

					if (*(mpData.Data[i]+6) == ']')
					{
						AllData = true;
					}
					else
					{
						Buffer = _strdup(mpData.Data[i]+5);
						Buffer[strlen(Buffer) - 1] = 0; //去調 ]
					}
				}
				else if  (strnicmp(mpData.Data[i]+4,".PEEK",5) == 0)
				{			
					bPeek = true;

					if (*(mpData.Data[i]+10) == ']')
					{
						AllData = true;
						
					}
					else
					{
						Buffer = _strdup(mpData.Data[i]+10);
						Buffer[strlen(Buffer) - 1] = 0; //去調 ]
					}
				}

				

				AddVarString("BODY[",&istr);
				AddVarString(Buffer,&istr);
				AddVarString("] ",&istr);

				//fprintf(Sendfp,"BODY[%s] ",Buffer);
				//strcat(RtnBuffer,"BODY[");
				//strcat(RtnBuffer, Buffer);
				//strcat(RtnBuffer, "] ");


				if (AllData)
				{
				
						//need all data
						/*char ImapFileName[255];								
					 

						strcpy(ImapFileName,PGPath);
						strcat(ImapFileName,"Mail\\");
						strcat(ImapFileName,User);
						strcat(ImapFileName,"\\");
						strcat(ImapFileName,MailBoxName);
						strcat(ImapFileName,"\\");
						strcat(ImapFileName,UID);
						strcat(ImapFileName,".eml");

						FILE *fp = NULL;

						fp = fopen(ImapFileName,"rb");
						if (fp != NULL)
						{
						
							fseek(fp,0,SEEK_END);
							int size = ftell(fp);
							fseek(fp,0,SEEK_SET);

							char * StrBuf = new char [size + 255];
							fread(StrBuf,1,size,fp);
							StrBuf[size] = 0;

							ToImapString(StrBuf);
						

							AddVarString(StrBuf,&istr);
							AddVarString(" ",&istr);
							
							fclose(fp);
						}*/

					char *MailBuffer = MailServerDB->RetriveMail(&MailData);

				
					

				
					AddVarImapString(MailBuffer,&istr);
					AddVarString(" ",&istr);

					MailServerDB->FreeMail(MailBuffer);

				}
				else
				{


					ParseData pdata;
					memset(&pdata,0,sizeof(pdata));

					ToParseData(Buffer,&pdata);

					char * StrBufOutput = new char[1];
					StrBufOutput[0] = 0;

					for (int i = 0 ; i < pdata.DataCount ; i++)
					{
						if (strcmp(pdata.Data[i],"HEADER") == 0 )
						{
						
							/*
							char MailFilePath[MAX_PATH];

							strcpy(MailFilePath,PGPath);
							strcat(MailFilePath,"Mail\\");
							strcat(MailFilePath,User);
							strcat(MailFilePath,"\\");
							strcat(MailFilePath,MailBoxName);
							strcat(MailFilePath,"\\");
							strcat(MailFilePath,UID);
							strcat(MailFilePath,".ehd");

							FILE *fp = NULL;

							fp = fopen(MailFilePath,"rb");
							if (fp != NULL)
							{
							
								fseek(fp,0,SEEK_END);
								int size = ftell(fp);
								fseek(fp,0,SEEK_SET);

								char * StrBuf = new char [size + 1];
								fread(StrBuf,1,size,fp);
								StrBuf[size] = 0;

								char* tmpStrBufOutput = new char[strlen(StrBufOutput) + size + 50];
								strcpy(tmpStrBufOutput,StrBufOutput);
								strcat(tmpStrBufOutput,StrBuf);

								delete StrBufOutput;
								StrBufOutput = tmpStrBufOutput;
								



								delete StrBuf;
							
								fclose(fp);
							}*/

							delete StrBufOutput;
							StrBufOutput = MailServerDB->RetriveMailHeader(&MailData);

 						
						}
						else if (strcmp(pdata.Data[i],"HEADER.FIELDS") == 0 )
						{
							// For Andorid 	
							//if (i + 2 <= pdata.DataCount )
							//{
								 //取得指定的 header
								CEmail email;

								StrBufOutput = MailServerDB->RetriveMailHeader(&MailData);
								MailHeader *mh = email.GetMailHeader(StrBufOutput);	

								if (mh->Subject[0] != 0 &&
									mh->From[0] != 0 &&
									mh->To[0] != 0)
								{
									char *HeaderDataStr = new char[strlen(StrBufOutput)];
									memset(HeaderDataStr,0,strlen(StrBufOutput));

			   
									strcpy(HeaderDataStr,"Date: ");
									
									if (mh->Date[0] != 0)
										strcat(HeaderDataStr,mh->Date);

									strcat(HeaderDataStr,"\r\n");
									
								 

									strcat(HeaderDataStr,"Subject: ");
									strcat(HeaderDataStr,mh->Subject);
									//strcat(HeaderDataStr,"TEST");
									strcat(HeaderDataStr,"\r\n");


									strcat(HeaderDataStr,"From: ");
									strcat(HeaderDataStr,mh->From);
									//strcat(HeaderDataStr,"<noreply-comment@blogger.com>");
									strcat(HeaderDataStr,"\r\n");


									//暫用
									

									//strcat(HeaderDataStr,mh->Content_Type->text);									
									strcat(HeaderDataStr,"Content_Type: text/plain; charset=\"big-5\"");									
									strcat(HeaderDataStr,"\r\n");
									
									strcat(HeaderDataStr,"To: ");
									strcat(HeaderDataStr,mh->To);
									strcat(HeaderDataStr,"\r\n");

									strcat(HeaderDataStr,"Cc: ");
									if (mh->Cc[0] != 0)
									{									  
									  strcat(HeaderDataStr,mh->Cc); 
									}
									strcat(HeaderDataStr,"\r\n");


									strcat(HeaderDataStr,"Message-ID: ");

									if (mh->Message_Id[0] != 0)
									{									
										strcat(HeaderDataStr,mh->Message_Id);
									}
									strcat(HeaderDataStr,"\r\n");


									delete StrBufOutput;
								    StrBufOutput = HeaderDataStr;


									//delete HeaderDataStr;

									
								
								
								//}

								
					
								email.FreeMailHeader(mh);

							}

						
						}

					 
					
					}

					if (StrBufOutput[0] != 0)
					{
						//ConvertToImapStr(&StrBufOutput);
						AddVarImapString(StrBufOutput,&istr);
						AddVarString(" ",&istr);
						//fprintf(Sendfp,"%s",StrBufOutput);

						//char *newRtnBuffer = new char[strlen(RtnBuffer) + strlen(StrBufOutput) + 50];
						//strcpy(newRtnBuffer , RtnBuffer);
						//strcat(newRtnBuffer , StrBufOutput);

						//delete RtnBuffer;
						//RtnBuffer = newRtnBuffer;

					
					}
					else
					{
						//都找不到資料
						//fprintf(Sendfp,"{0}\r\n ");
						AddVarString("{0}\r\n ",&istr);
						 
						//strcat(RtnBuffer,"{0} \r\n ");
					}

					delete StrBufOutput;

				}


		
					delete Buffer;			


			} 
			else if (strnicmp(mpData.Data[i],"ENVELOPE",8) == 0)
			{

				char* imapenvelope = MailServerDB->RetriveImapEnvelope(&MailData);
				AddVarString(imapenvelope,&istr);
				MailServerDB->FreeMail(imapenvelope);


				

				/*
				char MailFilePath[MAX_PATH];

				strcpy(MailFilePath,PGPath);
				strcat(MailFilePath,"Mail\\");
				strcat(MailFilePath,User);
				strcat(MailFilePath,"\\");
				strcat(MailFilePath,MailBoxName);
				strcat(MailFilePath,"\\");
				strcat(MailFilePath,UID);
				strcat(MailFilePath,".ehd");
				
				FILE *fp = NULL;

				fp = fopen(MailFilePath,"rb");
				if (fp != NULL)
				{
							
						fseek(fp,0,SEEK_END);
						int size = ftell(fp);
						fseek(fp,0,SEEK_SET);

						char * StrBuf = new char [size + 1];
						fread(StrBuf,1,size,fp);
						StrBuf[size] = 0;

						CEmail email;
						MailHeader *mh = email.GetMailHeader(StrBuf);

						//fprintf(Sendfp,"ENVELOPE (");
						AddVarString("ENVELOPE (",&istr);
						
						if (mh->Date[0] == 0)
						{
							//fprintf(Sendfp,"NIL ");
							AddVarString("NIL ",&istr);
						}
						else
						{
							AddVarString("\"",&istr);
							AddVarString(mh->Date,&istr);
							AddVarString("\" ",&istr);
							//fprintf(Sendfp,"\"%s\" ",mh->Date);
						}

						
						if (mh->Subject[0] == 0)
						{
							//fprintf(Sendfp,"NIL ");
							AddVarString("NIL ",&istr);
						}
						else
						{
						
								//Transfer To Imap String
							int len =  strlen(mh->Subject);
							char *tmp = new char[len + 10];
							strcpy(tmp,mh->Subject);

							ToImapString(tmp);

							//fprintf(Sendfp,"%s ",tmp);
							AddVarString(tmp,&istr);
							AddVarString(" ",&istr);

							delete tmp;

						}

					


						RFCToEnvStruct(mh->From,&istr);
						RFCToEnvStruct(mh->Sender,&istr);
						RFCToEnvStruct(mh->Reply_To,&istr);
						RFCToEnvStruct(mh->To,&istr);
						RFCToEnvStruct(mh->Cc,&istr);
						RFCToEnvStruct(mh->Bcc,&istr);

						
						//fprintf(Sendfp,"NIL "); //InReplyTo					
						AddVarString("NIL ",&istr);


						if (mh->Message_Id[0] != 0)
						{
							//fprintf(Sendfp,"\"%s\") ",mh->Message_Id);
							AddVarString("\"",&istr);
							AddVarString(mh->Message_Id,&istr);
							AddVarString("\") ",&istr);
						}
						else
						{
							//fprintf(Sendfp,"NIL) ");
							AddVarString("NIL) ",&istr);
						}

				

						email.FreeMailHeader(mh);
						delete StrBuf;
							
						fclose(fp);
				}

  */
				
			

			}
			else if (strnicmp(mpData.Data[i],"RFC822.SIZE",11) == 0)
			{
				AddVarString("RFC822.SIZE ",&istr);
			
				char RFCSize[255];
				itoa(MailData.RawMailDataLen,RFCSize,10);				
				AddVarString(RFCSize,&istr);			
				AddVarString(" ",&istr);

			}
			else if (strnicmp(mpData.Data[i],"UID",3) == 0)
			{
			
				
		 

				//fprintf(Sendfp,"UID %s ",UID);
				AddVarString("UID ",&istr);
				AddVarString(UID,&istr);
				AddVarString(" ",&istr);

				


			}
			else if (strnicmp(mpData.Data[i],"FLAGS",5) == 0)
			{
				//strcat(RtnBuffer,"FLAGS (");

				 
				//fprintf(Sendfp,"FLAGS (");
				AddVarString("FLAGS (",&istr);
				MailStatusToStr(MailData.Status,&istr);

				/*


				if ((MailData.Status & Flag_Seen) == Flag_Seen)
				{
					//strcat(RtnBuffer,"\\Seen ");
					//fprintf(Sendfp,"\\Seen ");
					AddVarString("\\Seen ",&istr);
				}

				if ((MailData.Status & Flag_Answered) == Flag_Answered)
				{
					//strcat(RtnBuffer,"\\Answered ");
					//fprintf(Sendfp,"\\Answered ");
					AddVarString("\\Answered ",&istr);
				}

				if ((MailData.Status & Flag_Flagged) == Flag_Flagged)
				{
					//strcat(RtnBuffer,"\\Flagged ");
					//fprintf(Sendfp,"\\Flagged ");
					AddVarString("\\Flagged ",&istr);
				}

				if ((MailData.Status & Flag_Deleted) == Flag_Deleted)
				{
					//strcat(RtnBuffer,"\\Deleted ");
					//fprintf(Sendfp,"\\Deleted ");
					AddVarString("\\Deleted ",&istr);
				}

				if ((MailData.Status & Flag_Draft) == Flag_Draft)
				{
					//strcat(RtnBuffer,"\\Draft ");
					//fprintf(Sendfp,"\\Draft ");
					AddVarString("\\Draft ",&istr);
				}

				if ((MailData.Status & Flag_Recent) == Flag_Recent)
				{
					//strcat(RtnBuffer,"\\Recent ");
					//fprintf(Sendfp,"\\Recent ");
					AddVarString("\\Recent ",&istr);
				}*/

				//strcat(RtnBuffer,") ");

				//fprintf(Sendfp,") ");
				//istr.str[strlen(istr.str)-1] = 0;
				AddVarString(") ",&istr);

			}
			else if (strnicmp(mpData.Data[i],"INTERNALDATE",12) == 0)
			{
				

				if (MailData.ImapInternalDate[0] != 0)
				{
					//fprintf(Sendfp,"INTERNALDATE \"%s\" ",md.InternalDate);
					AddVarString("INTERNALDATE \"",&istr);
					AddVarString(MailData.ImapInternalDate,&istr);
					AddVarString("\" ",&istr);
				}

			}

			
							
		}


	
		delete DupStr;
		
	
		AddVarString(")",&istr);	
		InsertQueSend(istr.str,&userdata->que);
		MailServerDB->ListNextMail(&MailData);

			
		
	}

	
	 
	return true;


}


bool CIMAPServer::SearchMail(unsigned int FromMailUId , unsigned int ToMailUId ,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock )
{
	IMAPData *userdata = (IMAPData *) sock->UserData;	
	DBMailData MailData;
	MailServerDB->ListFromMail(UserId,FolderId,FromMailUId,&MailData);
	

	ImapStr istr;
	memset(&istr,0,sizeof(istr));	
	AddVarString("* SEARCH ",&istr);	

	while (MailData.MailUId > 0 && MailData.MailUId <= ToMailUId)
	{

		//char *DupStr = strdup(ImapQuery);

		
		//ParseData mpData;
		//memset(&mpData , 0 , sizeof(mpData));

	//	ToParseData(DupStr,&mpData);

		char UID[255];
		itoa(MailData.MailUId,UID,10);		

	
						

		//char sSEQNO[50];
		//itoa(MailData.ImapSeqNo,sSEQNO,10);
	
		//AddVarString("* ",&istr);		
		
		AddVarString(UID,&istr);		
		AddVarString(" ",&istr);		
	//	AddVarString(sSEQNO,&istr);	
	//	AddVarString(" SEARCH (",&istr);

	
		//delete DupStr;
		
	
	//	AddVarString(")",&istr);	
		
		MailServerDB->ListNextMail(&MailData);

		

			
		
	}

	if (istr.size > 0)
	InsertQueSend(istr.str,&userdata->que);
	
	 
	return true;


}

unsigned int CIMAPServer::GetMailUID()
{

	//unsigned int MailUID = 0;
	EnterCriticalSection(&MailUIDSection);

	 
	char IniPath[MAX_PATH];	 	  

	strcpy(IniPath,PGPath);	      
	strcat(IniPath,"SpamDogServer.ini");

	int MailUID = GetPrivateProfileInt("ServerSetup","MailUID",1,IniPath); 

	//OverFlow than Reset
	if (MailUID > 0xFFFFFFFE) MailUID = 10000000;

	InterlockedIncrement((long *)&MailUID);
	/*time_t now;
	time(&now);

	if ((long) now > MailUID)
	{
		MailUID = (long) now;
	}
	else
	{
		MailUID = (long) now + 1; //offset
	}*/

	char MailUIDStr[255];
	itoa(MailUID,MailUIDStr,10);

	WritePrivateProfileString("ServerSetup","MailUID",MailUIDStr,IniPath);


	LeaveCriticalSection(&MailUIDSection);

	return MailUID;

}

unsigned int CIMAPServer::GetBoxUID()
{

	EnterCriticalSection(&MailUIDSection);

	char IniPath[MAX_PATH];	 	  

	strcpy(IniPath,PGPath);	      
	strcat(IniPath,"SpamDogServer.ini");

	int BoxUID = GetPrivateProfileInt("ServerSetup","BoxUID",1,IniPath); 

	//OverFlow than Reset
	if (BoxUID > 0xFFFFFFFE) BoxUID = 1;

	InterlockedIncrement((long *)&BoxUID);

	char BoxUIDStr[255];
	itoa(BoxUID,BoxUIDStr,10);

	WritePrivateProfileString("ServerSetup","BoxUID",BoxUIDStr,IniPath);


	LeaveCriticalSection(&MailUIDSection);

	return BoxUID;

}


ImapUserList* CIMAPServer::SearchAccountIdList(unsigned int AccountId)
{

	ImapUserList *ptr = m_ImapUserList;
	while (ptr)
	{	  
	    if (ptr->DBAccountId == AccountId)
		{
			return ptr;
			
		} 
		ptr = ptr->next; 	
	  
	}


 
	return NULL;



}


ImapUserList* CIMAPServer::SearchUserList(char* User)
{


	ImapUserList *ptr = m_ImapUserList;
	while (ptr)
	{	  
	    if (strcmp(ptr->USER,User) == 0)
		{
			return ptr;
			
		} 
		ptr = ptr->next; 	
	  
	}


 
	return NULL;
}

void CIMAPServer::NilMailData(ImapMailData *mdata)
{
	memset(mdata,0,sizeof(ImapMailData));
	
	/*strcpy(mdata->Date,"NIL");
	strcpy(mdata->Subject,"NIL");

	strcpy(mdata->From.AtDomainList,"NIL");
	strcpy(mdata->From.HostName,"NIL");
	strcpy(mdata->From.MailBoxName,"NIL");
	strcpy(mdata->From.PersonalName,"NIL");

	strcpy(mdata->Sender.AtDomainList,"NIL");
	strcpy(mdata->Sender.HostName,"NIL");
	strcpy(mdata->Sender.MailBoxName,"NIL");
	strcpy(mdata->Sender.PersonalName,"NIL");

	strcpy(mdata->ReplyTo.AtDomainList,"NIL");
	strcpy(mdata->ReplyTo.HostName,"NIL");
	strcpy(mdata->ReplyTo.MailBoxName,"NIL");
	strcpy(mdata->ReplyTo.PersonalName,"NIL");

	strcpy(mdata->To.AtDomainList,"NIL");
	strcpy(mdata->To.HostName,"NIL");
	strcpy(mdata->To.MailBoxName,"NIL");
	strcpy(mdata->To.PersonalName,"NIL");

	strcpy(mdata->Cc.AtDomainList,"NIL");
	strcpy(mdata->Cc.HostName,"NIL");
	strcpy(mdata->Cc.MailBoxName,"NIL");
	strcpy(mdata->Cc.PersonalName,"NIL");

	strcpy(mdata->Bcc.AtDomainList,"NIL");
	strcpy(mdata->Bcc.HostName,"NIL");
	strcpy(mdata->Bcc.MailBoxName,"NIL");
	strcpy(mdata->Bcc.PersonalName,"NIL");

	strcpy(mdata->InReplyTo,"NIL");
	strcpy(mdata->MessageId,"NIL");
	*/



}

void  CIMAPServer::ListDBAccountFolder(MailFolderData *FolderData,SOCKET_OBJ* sock ,const char* Cmd , unsigned int AccountId)
{

	char FolderBuffer[255]={0};
	strcpy(FolderBuffer,"* ");
	strcat(FolderBuffer,Cmd);
	strcat(FolderBuffer," () \"/\" \"");

	if (FolderData == NULL)
	{	
		//取第一個 FolderID
	   MailFolderData m_FolderData;
	   MailServerDB->GetAccountFolderData(AccountId,0,&m_FolderData);
	   
	   strcat(FolderBuffer,m_FolderData.FolderName);
	   strcat(FolderBuffer,"\"");
	   SendLn(sock,FolderBuffer);	  
	   
	   ListDBAccountFolder(&m_FolderData,sock,Cmd,AccountId);
	}
	else
	{
		//先看右邊
		if (FolderData->SubMailFolderDataPos > 0)
		{
		
			MailFolderData r_FolderData ;
			MailServerDB->GetAccountFolderDataPos(FolderData->SubMailFolderDataPos,&r_FolderData);
			//MailFolderData r_FolderData = MailServerDB->GetAccountFolderData(AccountId,FolderData->SubMailFolderDataPos));
			if (r_FolderData.FolderUId != 0)
			{
			//	strcat(FolderBuffer,FolderData->FolderName);
			//	strcat(FolderBuffer,"/");
				char SubFolderBuffer[255]={0};
				strcpy(SubFolderBuffer,FolderBuffer);

  			    strcat(SubFolderBuffer,r_FolderData.FolderName);
				strcat(SubFolderBuffer,"\"");
				SendLn(sock,SubFolderBuffer);	 

				ListDBAccountFolder(&r_FolderData,sock,Cmd,AccountId);
			}

		}
		
		if (FolderData->NextMailFolderDataPos > 0)
		{
			//MailFolderData n_FolderData = MailServerDB->GetAccountFolderData(AccountId,FolderData->NextMailFolderDataPos);
			MailFolderData n_FolderData ;
			MailServerDB->GetAccountFolderDataPos(FolderData->NextMailFolderDataPos,&n_FolderData);

			    strcat(FolderBuffer,n_FolderData.FolderName);
				strcat(FolderBuffer,"\"");
				SendLn(sock,FolderBuffer);

	        ListDBAccountFolder(&n_FolderData,sock,Cmd,AccountId);
		}
	}
	//SendLn(sock,"* LIST () \"/\" \"IpHistory\"");

}

void CIMAPServer::InsertUserList(SOCKET_OBJ* sock , char* User)
{
	EnterCriticalSection(&MailListSection);

	ImapUserList *end=NULL, 
               *ptr=NULL;

	ImapUserList  *obj = new ImapUserList;
	obj->sock = sock;
	strcpy(obj->USER,User);

	obj->DBAccountId =  MailServerDB->GetAccountId(obj->USER);

    // Find the end of the list
    ptr = m_ImapUserList;
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
        m_ImapUserList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	LeaveCriticalSection(&MailListSection);

}
void CIMAPServer::RemoveUserList(char* User)
{

	EnterCriticalSection(&MailListSection);

	ImapUserList *obj = SearchUserList(User);
	
	if (m_ImapUserList != NULL && obj != NULL)
    {
        // Fix up the next and prev pointers
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;

        if (m_ImapUserList == obj)
           m_ImapUserList = obj->next;

		delete obj;
    }

	LeaveCriticalSection(&MailListSection);
}

int CIMAPServer::OnAfterAccept(SOCKET_OBJ* sock) //連線後可以送的第一個 command
{
	sock->LastCmd = IM_NORMAL;
	
	HANDLE ProcessHeap = GetProcessHeap();
	sock->UserData =  (char *) CHeap::GetHeap(sizeof(IMAPData),&ProcessHeap);

	return 	SendLn(sock,"* OK Wellcome to AntiSpam Server");

}

void CIMAPServer::OnDisconnect(int SockHandle)
{

	//_cprintf("Disconnect\r\n");
}

void CIMAPServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

	//_cprintf("Before Disconnect\r\n");
	IMAPData *userdata = (IMAPData *) sock->UserData;
	//RemoveUserList(userdata->USER);

	    ImapUserList *obj = SearchUserList(userdata->USER);

				
		if (obj != NULL)
		{
				//if (userdata->SelectedBoxName[0] != 0)
				//{
					
					RemoveUserList(userdata->USER);
				//}
		}
			
		//CleanDelMail(userdata);	
  
}

int CIMAPServer::OnConnect(sockaddr* RemoteIp) //在 配至資源 之前 , 可拒絕 client
{
	return NO_ERROR;
}

void CIMAPServer::ToParseMailListData(char *AStr , ParseData* aPData)
{
	int len = strlen(AStr);
	int SavePos = 0;
	bool QuoteIgnore = false;


	for (int i = 0 ; i < len ; i++)
	{
		
		if (AStr[i] == '"')
		{
		   if (QuoteIgnore == true)
		   {
			   QuoteIgnore = false;
		   } else
		   {
			   QuoteIgnore = true;
		   }

		   continue;


		}
	
		if (QuoteIgnore == true)
			continue;

		if (AStr[i] == ',' || AStr[i] == ';' || i == len - 1)
		{
		
			aPData->DataCount ++;
			aPData->Data[aPData->DataCount -1] =  AStr + SavePos;


			if (AStr[i] == ',' || AStr[i] == ';')
			AStr[i] = 0;

			SavePos = i + 1;

		}
	}

	if (aPData->DataCount == 0)
	{
			aPData->DataCount ++;
			aPData->Data[0] =  AStr ;
	}

}

void CIMAPServer::ToParseSeqData(char *AStr , ParseData* aPData)
{
	int len = strlen(AStr);
	int SavePos = 0;


	for (int i = 0 ; i < len ; i++)
	{
		if (AStr[i] == ',' || i == len - 1)
		{
		
			aPData->DataCount ++;
			aPData->Data[aPData->DataCount -1] =  AStr + SavePos;


			if (AStr[i] == ',')
			AStr[i] = 0;

			SavePos = i + 1;

		}
	}

	if (aPData->DataCount == 0)
	{
			aPData->DataCount ++;
			aPData->Data[0] =  AStr ;
	}


}

void CIMAPServer::ToParseData(char *AStr , ParseData* aPData)
{
	int len = strlen(AStr);
	int ignore = 0;
	int SavePos = 0;
	bool bBreak = false;
	int QuoteCount = 0;

	for (int i = 0 ; i < len ; i++)
	{
		
		if (AStr[i] == '"')
		{
			QuoteCount ++;
		}
		else if (AStr[i] == '(' || (AStr[i] == '[' )  )		
		{
			if (ignore == 0)
			{
				if (i - SavePos == 0)
				SavePos = i+1; 				
			}		 

		/*	if (AStr[i] == '[')
			{
				if (SavePos != i )
				{
				
						aPData->DataCount ++;
						aPData->Data[aPData->DataCount -1] =  AStr + SavePos;

						if (AStr[i] == char(13))
							 bBreak = true;

						if (i != len - 1 || AStr[i] == ')' || AStr[i] == ']' )
							AStr[i] = 0;

						if (bBreak) break;

					SavePos = i + 1;
				}

			
			
			}*/
		

			if (AStr[i] == '[' && AStr[i+1] == ']' )
			{
				if (ignore != 0)
				ignore ++;
			}
			else
			{
				ignore ++;
			}
		}
		else if (AStr[i] == ')' || AStr[i] == ']'  || (ignore == 0 && AStr[i] == ' ') || i == len - 1 || AStr[i] == char(13))			
		//else if (AStr[i] == ')'  || (ignore == 0 && AStr[i] == ' ') || i == len - 1 || AStr[i] == char(13))			
		{
			if ((AStr[i] == ')' || AStr[i] == ']' ) && ignore > 0)
			//if (AStr[i] == ')' )
			ignore --;

			if (i ==  len -1  && ignore == 0 && AStr[i] == ']' && (QuoteCount % 2) == 0)
			{
					QuoteCount = 0 ;

					if (SavePos != i )
					{
						aPData->DataCount ++;
						aPData->Data[aPData->DataCount -1] =  AStr + SavePos;
						

						if (AStr[i] == char(13))
							 bBreak = true;					

						if (bBreak) break;
					}

				SavePos = i + 1;
			
			} else
			if (ignore == 0 && AStr[i] != ']' && (QuoteCount % 2) == 0)
			//if (ignore == 0 && (QuoteCount % 2) == 0)
			{
				 
					QuoteCount = 0 ;

					if (SavePos != i )
					{
						aPData->DataCount ++;
						aPData->Data[aPData->DataCount -1] =  AStr + SavePos;
						

						if (AStr[i] == char(13))
							 bBreak = true;

						//if (i != len - 1 || AStr[i] == ')' || AStr[i] == ']' )
						if (i != len - 1 || AStr[i] == ')')
							AStr[i] = 0;

						if (bBreak) break;
					}

				SavePos = i + 1;
				 
			
			}


		}
			
	
	}
	

}

int CIMAPServer::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{

	CBaseServer::OnDataWrite(sock, buf, BytesSent);

	IMAPData *userdata = (IMAPData *) sock->UserData;

	if (userdata->que != NULL)
	{
	
		RemoveQueSend(userdata->que,&userdata->que);
		
		if (userdata->que != NULL)
		SendLn(sock,userdata->que->cmd);
		
		
	
	
	}

	/*
	if (userdata->Sendfp != NULL)
	{
	
		int readsize;
		char data[DEFAULT_BUFFER_SIZE];
		

		if(0 != (readsize = fread(data, sizeof(char), sizeof(data), userdata->Sendfp)))
		{			
			
			if (readsize > buf->buflen) 
			{
				 //_cprintf("......Buffer Couldn't Re-Send\n");
				SendBufferData(sock, data, readsize);
				
			}
			else
			{
				memcpy(buf->buf,data,readsize);
				buf->buflen = readsize;

				CResourceMgr::SendBuffer(sock,buf);

				return IO_NO_FREEBUFFER;			
			
			}
			
		 
		}
		else
		{
		    //資料送完

			fclose(userdata->Sendfp);
			userdata->Sendfp = NULL;

			DeleteFile(userdata->SendDataFilePath);

			userdata->SendDataFilePath[0] = 0;	

			if (userdata->SendDataEndCmd != NULL)
			{
				SendLn(sock,userdata->SendDataEndCmd);
				delete userdata->SendDataEndCmd ;
				userdata->SendDataEndCmd  = NULL;
			
			}
			

		}

	
	}*/

	return NO_ERROR;
}

int CIMAPServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	int rc = NO_ERROR;

	IMAPData *userdata = (IMAPData *) sock->UserData;

	CBaseServer::OnDataRead(sock,buf,BytesRead);
    //_cprintf("%s\n",buf->buf); 
	//char value[10][255];	
	char SendBuf[255]={0};
	char FinalSendBuf[255]={0};

	if (sock->LastCmd == IM_NORMAL)
	{
		
		if (strnicmp(buf->buf,"DONE",4) == 0)
		{
		   if (userdata->IDLEcmd[0] != 0)
		   {
				strcpy(SendBuf,userdata->IDLEcmd);
				strcat(SendBuf," OK IDLE completed.");
				SendLn(sock,SendBuf);

				userdata->IDLEcmd[0] = 0;

				return rc;
		   }
		
		}

		char* tag = NULL;
		char* cmd = NULL;

		ParseData pData;
		memset(&pData , 0 , sizeof(pData));

		char *buffer1 = NULL;

		

		//if (userdata->IDLEcmd != true)
		//{
			buffer1 = _strdup(buf->buf);
			
			ToParseData(buffer1,&pData);
		
			tag = pData.Data[0];
			cmd = pData.Data[1];

		

			strcpy(SendBuf,tag);
			strcat(SendBuf," ");
		//}
		//else
		//{
		
		//	strcpy(SendBuf,userdata->tmpcmd);
		//	strcat(SendBuf," OK IDLE completed.");
		//	SendLn(sock,SendBuf);

		//	userdata->IDLEcmd = false;

		//	return rc;
		//}

		if (stricmp(cmd,"IDLE") == 0)
		{
		
			strcpy(userdata->IDLEcmd,tag);			
			SendLn(sock,"+ IDLE accepted.");		 
			
		}
		else 
		if (stricmp(cmd,"STATUS") == 0)
		{
		 	//char StatusReturn[255];

			TrimQuote(pData.Data[2]);

			strcpy(FinalSendBuf,"* STATUS ");
			strcat(FinalSendBuf,pData.Data[2]);
			strcat(FinalSendBuf," (");

			
			MailFolderData FolderData;
			memset(&FolderData,0,sizeof(FolderData));			

			MailServerDB->GetAccountFolderNameData(userdata->DBAccountId,pData.Data[2],&FolderData);
		

			char charnum[20];  
			
			if (strstr(pData.Data[3] , "MESSAGES") != NULL)
			{
				strcat(FinalSendBuf,"MESSAGES ");			
				itoa(FolderData.Exists,charnum,10);
				strcat(FinalSendBuf,charnum);
				strcat(FinalSendBuf," ");
			}

			if (strstr(pData.Data[3] , "RECENT") != NULL)
			{
				strcat(FinalSendBuf,"RECENT ");
				itoa(FolderData.Recent,charnum,10);
				strcat(FinalSendBuf,charnum);
				strcat(FinalSendBuf," ");
			}

			if (strstr(pData.Data[3] , "UIDNEXT") != NULL)
			{
				
				unsigned int UidNext = FolderData.MailUIdIndex+1;
				char NextUidStr[50];
				itoa(UidNext , NextUidStr , 10);				

				strcat(FinalSendBuf,"UIDNEXT ");
				strcat(FinalSendBuf,NextUidStr);
				strcat(FinalSendBuf," ");
			
			}

			if (strstr(pData.Data[3] , "UIDVALIDITY") != NULL)
			{
				strcat(FinalSendBuf,"UIDVALIDITY ");
				itoa(FolderData.FolderUId,charnum,10);
				strcat(FinalSendBuf,charnum);
				strcat(FinalSendBuf," ");
			}

			if (strstr(pData.Data[3] , "UNSEEN") != NULL)
			{
				strcat(FinalSendBuf,"UNSEEN ");

				/*char tmpchr[50];
				itoa(FolderData.Seen,tmpchr,10);
				_cprintf("seen: %s\r\n\r\n",tmpchr);*/

				itoa(FolderData.Exists - FolderData.Seen,charnum,10);
				strcat(FinalSendBuf,charnum);
				strcat(FinalSendBuf," ");
			}

			// ) 補上最後
			
			int len = strlen(FinalSendBuf);
			FinalSendBuf[len-1] = ')';
			FinalSendBuf[len] = 0;
			
			//SendLn(sock,StatusReturn);
			

		
			///SendLn(sock,FinalSendBuf);
			//strcat(SendBuf,"OK STATUS completed");
			//SendLn(sock,SendBuf);

			
			
			strcat(SendBuf,"OK STATUS completed");
			strcat(FinalSendBuf,"\r\n");
			strcat(FinalSendBuf,SendBuf);
		 

			
			SendLn(sock,FinalSendBuf);
			

			
			
		}
		else if (stricmp(cmd,"CAPABILITY") == 0)
		{	
		
			
			//strcpy(FinalSendBuf,"* CAPABILITY IMAP4rev1 IDLE\r\n");
			//strcat(FinalSendBuf,SendBuf);

			//SendLn(sock,"* CAPABILITY IMAP4rev1 ");
			char tmpstr[50];
			strcpy(tmpstr,"* CAPABILITY IMAP4rev1 UIDPLUS IDLE\r\n");

			//send(sock->s,tmpstr,strlen(tmpstr),0);
			strcat(SendBuf,"OK CAPABILITY completed");
			strcpy(FinalSendBuf,tmpstr);
			strcat(FinalSendBuf,SendBuf);

			SendLn(sock,FinalSendBuf);
		
		}
		else if (stricmp(cmd,"LOGIN") == 0)
		{
			

			//check AD verify
			if ( pData.DataCount == 4) //id and password
			{
			
				TrimQuote(pData.Data[2]);
				strncpy(userdata->USER , &pData.Data[2][0],strlen(pData.Data[2]));
				userdata->USER[strlen(pData.Data[2])] = 0;		

				char pass[255];
				TrimQuote(pData.Data[3]);
				strncpy(pass , &pData.Data[3][0],strlen(pData.Data[3]));
				pass[strlen(pData.Data[3])] = 0;

				if (AccountCheckMode == 0 && AD_IP[0] != 0 )
				{


					if (CAntiSpamServer::VerifyAccount(AD_IP,userdata->USER,pass))
					{
						//strcpy(userdata->USER,userdata->USER);
						strcat(SendBuf,"OK LOGIN completed");

						
						CAntiSpamServer::GetAccountFromLogin(AD_IP,userdata->USER,ADLoginId,ADPassWord,userdata->USER,sizeof(userdata->USER));
						//crm to service
						

						//確認信箱是否已建立
						if (MailServerDB->CheckAccountExists(userdata->USER) != true)
						{					
							MailServerDB->AddAccount(userdata->USER);
						}

						userdata->DBAccountId = MailServerDB->GetAccountId(userdata->USER);			
						
						
					}
					else
					{
						strcat(SendBuf,"BAD LOGIN fail");
					}
				}
				else
				{

					if (VerifyAccountFile(userdata->USER,pass,userdata->USER) == true)
					{
						strcat(SendBuf,"OK LOGIN completed");
						//確認信箱是否已建立
						if (MailServerDB->CheckAccountExists(userdata->USER) != true)
						{					
							MailServerDB->AddAccount(userdata->USER);
						}

						userdata->DBAccountId = MailServerDB->GetAccountId(userdata->USER);	
					
					}
					else
					{
						strcat(SendBuf,"BAD LOGIN fail");
					}

				
				
				}

				SendLn(sock,SendBuf);				
			
			}
			
			
			
		}
		else if (stricmp(cmd,"LIST") == 0)
		{
			
			if (strcmp(pData.Data[2],"\"\"") == 0)
			{
				if (strcmp(pData.Data[3],"\"*\"") == 0 || pData.Data[3][0] == 0 || strcmp(pData.Data[3],"\"\"") == 0 )
				{
				
					//到 ServerDB 去取該 user 的目錄
					//MailServerDB->ListAccountFolder(userdata->USER); //處理遞回
					ListDBAccountFolder(NULL,sock,"LIST",userdata->DBAccountId);
					
					/*SendLn(sock,"* LIST () \"/\" \"IpHistory\"");
					SendLn(sock,"* LIST () \"/\" \"IpHistory/OkIp\"");	
					SendLn(sock,"* LIST () \"/\" \"IpHistory/BadIp\"");
					SendLn(sock,"* LIST () \"/\" \"SystemLog\"");
					SendLn(sock,"* LIST () \"/\" \"SpamMail\"");
					SendLn(sock,"* LIST () \"/\" \"LearnOk\"");*/
					//SendLn(sock,"* LIST () \"/\" \"\\\"");	
					strcat(SendBuf,"OK LIST completed");
				
				}
				else
				{
				
					SendLn(sock,"* LIST (\\Noselect) \"/\" \"\"");	
					strcat(SendBuf,"NO LIST completed");
				
				}			
			
			}
			else
			{
				SendLn(sock,"* LIST (\\Noselect) \"/\" \"\"");	
				strcat(SendBuf,"NO LIST completed");
				
			}

			SendLn(sock,SendBuf);

			/*if (strcmp(value,"\"\" \"*\"") == 0 || strcmp(value,"\"\" \"\"") == 0)
			{
			
				SendLn(sock,"* LIST () \"/\" \"ReportSpam\"");	
				strcat(SendBuf,"OK LIST completed");
			}
			else
			{
				SendLn(sock,"* LIST (\\Noselect) \"/\" \"\"");	
				strcat(SendBuf,"NO LIST completed");

			}

			SendLn(sock,SendBuf);*/
		}
		else if (stricmp(cmd,"LSUB") == 0)
		{
			
			if (strcmp(pData.Data[2],"\"\"") == 0)
			{
				if (strcmp(pData.Data[3],"\"*\"") == 0 || pData.Data[3][0] == 0)
				{
				
					/*SendLn(sock,"* LSUB () \"/\" \"IpHistory\"");
					SendLn(sock,"* LSUB () \"/\" \"IpHistory/OkIp\"");	
					SendLn(sock,"* LSUB () \"/\" \"IpHistory/BadIp\"");
					SendLn(sock,"* LSUB () \"/\" \"SystemLog\"");
					SendLn(sock,"* LSUB () \"/\" \"SpamMail\"");
					SendLn(sock,"* LSUB () \"/\" \"LearnOk\"");*/

					ListDBAccountFolder(NULL,sock,"LSUB",userdata->DBAccountId);
					
					strcat(SendBuf,"OK LSUB completed");
				
				}
				else
				{
				
					SendLn(sock,"* LSUB (\\Noselect) \"/\" \"\"");	
					strcat(SendBuf,"NO LSUB completed");
				
				}			
			
			}
			else
			{
				SendLn(sock,"* LSUB (\\Noselect) \"/\" \"\"");	
				strcat(SendBuf,"NO LSUB completed");
				
			}

			SendLn(sock,SendBuf);

			/*if (strcmp(value,"\"\" \"*\"") == 0 || strcmp(value,"\"\" \"\"") == 0)
			{
			
				SendLn(sock,"* LSUB () \"/\" \"ReportSpam\"");	
				 
			}
			else
			{
				SendLn(sock,"* LSUB (\\Noselect) \"//\" \"\"");
			}

			strcat(SendBuf,"OK LSUB completed");
			SendLn(sock,SendBuf);*/
		}
		else if (stricmp(cmd,"CREATE") == 0)
		{
			
			strcat(SendBuf,"NO CANNOT CREATE BOX");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"DELETE") == 0)
		{
			
			strcat(SendBuf,"NO CANNOT DELETE BOX");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"RENAME") == 0)
		{
			
			strcat(SendBuf,"NO CANNOT RENAME BOX");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"SUBSCRIBE") == 0)
		{		
			
			strcat(SendBuf,"OK SUBSCRIBE completed");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"UNSUBSCRIBE") == 0)
		{		
			
			strcat(SendBuf,"OK UNSUBSCRIBE completed");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"SELECT") == 0)
		{
			
			ImapUserList *obj = SearchUserList(userdata->USER);
			
			if (obj == NULL)
			  InsertUserList(sock,userdata->USER);

			TrimQuote(pData.Data[2]);
			strcpy(userdata->SelectedBoxName,pData.Data[2]);

			//從 Mail DB 取出
			MailFolderData m_FolderData;
			
			MailServerDB->GetAccountFolderNameData(userdata->DBAccountId,userdata->SelectedBoxName,&m_FolderData);
			userdata->DBSelectedFolderId = m_FolderData.FolderUId;

			userdata->DelSpamFolder = true;
			
			//MailServerDB->GetAccountFolderData(userdata->DBAccountId,FolderUId,&m_FolderData);

			
			//BoxInfo bi = CacuBoxInfo(userdata->USER,pData.Data[2]);

			char tmpcmd[255];
			tmpcmd[0] = 0;

			char charnum[20];

			ImapStr istr;
			memset(&istr,0,sizeof(istr));
			
			AddVarString("* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft) \r\n",&istr);

			strcpy(tmpcmd,"* ");
			itoa(m_FolderData.Exists,charnum,10);
			strcat(tmpcmd,charnum);
			strcat(tmpcmd," EXISTS");
			strcat(tmpcmd,"\r\n");
			AddVarString(tmpcmd,&istr);
			//SendLn(sock,tmpcmd);
			

			
			strcpy(tmpcmd,"* ");
			itoa(m_FolderData.Recent,charnum,10);
			strcat(tmpcmd,charnum);
			strcat(tmpcmd," RECENT");
			strcat(tmpcmd,"\r\n");
			AddVarString(tmpcmd,&istr);
			//SendLn(sock,tmpcmd);

			
			strcpy(tmpcmd,"* OK [UIDVALIDITY ");
			itoa(m_FolderData.FolderUId,charnum,10);
			strcat(tmpcmd,charnum);
			strcat(tmpcmd,"]");
			strcat(tmpcmd,"\r\n");
			AddVarString(tmpcmd,&istr);
			//SendLn(sock,tmpcmd);

		

			if (m_FolderData.FirstUnseen > 0 )
			{
				strcpy(tmpcmd,"* OK [UNSEEN ");
				itoa(m_FolderData.FirstUnseen,charnum,10);
				strcat(tmpcmd,charnum);
				strcat(tmpcmd,"]");
				strcat(tmpcmd,"\r\n");
				AddVarString(tmpcmd,&istr);
				//SendLn(sock,tmpcmd);
			}

			unsigned int UidNext = m_FolderData.MailUIdIndex+1;
			char NextUidStr[50];
			itoa(UidNext , NextUidStr , 10);

			strcpy(tmpcmd,"* OK [UIDNEXT ");			
			strcat(tmpcmd,NextUidStr);
			strcat(tmpcmd,"]");
			strcat(tmpcmd,"\r\n");
			AddVarString(tmpcmd,&istr);



			strcat(SendBuf,"OK [READ-WRITE] SELECT completed");

			
			AddVarString(SendBuf,&istr);
			

			SendLn(sock,istr.str);

			delete istr.str;

			if (userdata->DBSelectedFolderId == Folder_DelSpam)
			{
				CleanDelMail(userdata);
				//userdata->StatusChanged = true;

				if (userdata->IDLEcmd[0] != 0)
					SendIDLEResult(sock);
			}


			//SendLn(sock,"* 1 EXISTS");		
			//SendLn(sock,"* 1 RECENT");
			//SendLn(sock,"* OK [UIDVALIDITY 1010434677] UID validity status");
			//SendLn(sock,"* OK [UNSEEN 1] first unseen");
			//SendLn(sock,"* OK [UIDNEXT 2] Predicted next UID");

			//Suppose Status
			//SendLn(sock,"* FLAGS (\\Answered \\Flagged \\Deleted \\Seen)");

			//strcat(SendBuf,"OK [READ-WRITE] SELECT completed");
			//SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"NOOP") == 0)
		{
		 			
			strcat(SendBuf,"OK NOOP completed");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"LOGOUT") == 0)
		{
		 
			

		//	MailServerDB->FlushDB();

			//MailServerDB->CleanDelMail(userdata->DBAccountId);
		    strcpy(FinalSendBuf,"* BYE IMAP4rev1 Server logging out\r\n");
			//SendLn(sock,"* BYE IMAP4rev1 Server logging out");
			strcat(SendBuf,"OK LOGOUT completed");
			strcat(FinalSendBuf,SendBuf);
			SendLn(sock,FinalSendBuf);
		}
		else if (stricmp(cmd,"APPEND") == 0)
		{
		 			
			char* FlagStr = NULL;
			char* BoxNameStr = NULL;
			char* InternalDateStr = NULL;

			TrimQuote(pData.Data[2]);
			BoxNameStr = pData.Data[2];

			TrimQuote(BoxNameStr);
			
			MailFolderData m_FolderData;
			MailServerDB->GetAccountFolderNameData(userdata->DBAccountId,BoxNameStr,&m_FolderData);



			if (m_FolderData.FolderUId == Folder_DelSpam || m_FolderData.FolderUId == Folder_LearnOk)
			{			
				//Stop Append Mail
				char tmpstr[50] = {0};
				strcpy(tmpstr,tag);
				strcat(tmpstr," NO Cannot APPEND This Folder !");

				SendLn(sock,tmpstr);

				return rc;
			}


			char *tmpchar = new char[50];
			strcpy(tmpchar,tag);
			strcat(tmpchar," ");	

			//strcat(tmpchar," OK APPEND completed");

			userdata->RecvDataEndCmd = tmpchar;

			//確定是否存在 flag
			int literalTagPos = 3; //pvir APPEND "LearnOk"  {1701} --> 如果沒有任何 option

			if ( pData.Data[literalTagPos][0] == '\\')
			{
				

				TrimParenthesis(pData.Data[literalTagPos]);
				FlagStr = pData.Data[literalTagPos];

				literalTagPos ++;


			}

			//確定  InternalDate 是否存在
			if (pData.Data[literalTagPos][0] == '"')
			{
				

				TrimQuote(pData.Data[literalTagPos]);
				InternalDateStr = pData.Data[literalTagPos];

				literalTagPos ++;			
			}

			//取大小
			char SizeChar[20];
			memset(SizeChar,0,20);
			int sizelen = strlen(pData.Data[literalTagPos]);
			strncpy(SizeChar,pData.Data[literalTagPos]+1,sizelen-2);
			userdata->SaveCount = atoi(SizeChar) + 2;

			ImapUserList *obj = SearchUserList(userdata->USER);
			
			if (obj == NULL)
				  InsertUserList(sock,userdata->USER);

			
		    AddImapAppendMail(userdata,FlagStr,InternalDateStr,BoxNameStr);

			/*strcpy(userdata->RecvDataFilePath,PGPath);
			strcat(userdata->RecvDataFilePath,"Mail\\");
			strcat(userdata->RecvDataFilePath,userdata->USER);
			strcat(userdata->RecvDataFilePath,"\\");
			strcat(userdata->RecvDataFilePath,BoxNameStr);
			strcat(userdata->RecvDataFilePath,"\\");*/
			

			 

			sock->LastCmd = IM_DATA; //Apped 專用
			SendLn(sock,"+ Ready for literal data");
			//strcat(SendBuf,"+ Ready for literal data");
			//SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"UID") == 0)
		{			
			
			if (stricmp(pData.Data[2],"FETCH") == 0 )
			{
				
				ExecuteCmd("FETCH",pData.Data[3],pData.Data[4],userdata->DBAccountId,userdata->DBSelectedFolderId,sock);

				//process queue send
						
				strcat(SendBuf,"OK UID FETCH completed");

				char *newbuf = new char[strlen(SendBuf) + 1];
				strcpy(newbuf,SendBuf);

				InsertQueSend(newbuf,&userdata->que);

				//first Send Queue
				
				SendLn(sock,userdata->que->cmd);
				//SendLn(sock,"* 495 FETCH (UID 495 )");

				//RemoveQueSend(userdata->que,&userdata->que);

				//SendLn(sock,SendBuf);

				
				//{
				//	strcat(SendBuf,"OK UID FETCH completed");				
				//}
				//else
				//{
					//strcat(SendBuf,"NO FETCH completed");
				//	strcat(SendBuf,"OK UID FETCH completed");
				//}

			
				//SendLn(sock,SendBuf);				
				
			
			}
			else if (stricmp(pData.Data[2],"SEARCH") == 0)
			{
			
				ExecuteCmd("SEARCH",pData.Data[3],pData.Data[4],userdata->DBAccountId,userdata->DBSelectedFolderId,sock);

				//strcat(SendBuf,"OK UID SEARCH completed");

				//char *newbuf = new char[strlen(SendBuf) + 1];
				//strcpy(newbuf,SendBuf);

				//InsertQueSend(newbuf,&userdata->que);*/

				//process queue send
						
				strcat(SendBuf,"OK UID SEARCH completed");
				//strcat(SendBuf,"No Not Support");

				//char *newbuf = new char[strlen(SendBuf) + 1];
				//strcpy(newbuf,SendBuf);

				//InsertQueSend(newbuf,&userdata->que);

				//first Send Queue
				//SendLn(sock,userdata->que->cmd);
				SendLn(sock,userdata->que->cmd);
				//strcat(SendBuf,"OK UID SEARCH completed");
				//strcat(SendBuf,"OK UID SEARCH completed");
				SendLn(sock,SendBuf);
			
			
			}
			else if(stricmp(pData.Data[2],"STORE") == 0)
			{					
				if (pData.DataCount > 5)
				{
					char ReAssemCmd[50];
					strcpy(ReAssemCmd,"STORE ");
					strcat(ReAssemCmd,pData.Data[4]);
				
					ExecuteCmd(ReAssemCmd,pData.Data[3],pData.Data[5],userdata->DBAccountId,userdata->DBSelectedFolderId,sock);
				}
				
				strcat(SendBuf,"OK STORE completed");

				char *newbuf = new char[strlen(SendBuf) + 1];
				strcpy(newbuf,SendBuf);

				InsertQueSend(newbuf,&userdata->que);

				//first Send Queue
				SendLn(sock,userdata->que->cmd);

				
				//strcat(SendBuf,"OK STORE completed");
				//SendLn(sock,SendBuf);	

				
				//{
				//	strcat(SendBuf,"OK UID STORE completed");				
				//}
				//else
				//{
				//	strcat(SendBuf,"NO STORE completed");
				//}

			
				//SendLn(sock,SendBuf);	
			}
			else if(stricmp(pData.Data[2],"COPY") == 0)
			{
				TrimQuote(pData.Data[4]);


				MailFolderData m_FolderData;
				memset(&m_FolderData,0,sizeof(m_FolderData));								

				MailServerDB->GetAccountFolderNameData(userdata->DBAccountId,pData.Data[4],&m_FolderData);

				if (m_FolderData.FolderUId == Folder_DelSpam)
				{			
					//Stop Copy Mail
					char tmpstr[50] = {0};
					strcpy(tmpstr,tag);
					strcat(tmpstr," NO Cannot COPY This Folder !");

					SendLn(sock,tmpstr);

					return rc;
				}

			 
				ExecuteCmd("COPY",pData.Data[3],pData.Data[4],userdata->DBAccountId,userdata->DBSelectedFolderId,sock);

				CacuBoxInfo(userdata->USER,pData.Data[4]);
				CacuBoxInfo(userdata->USER,userdata->SelectedBoxName);
				
				strcat(SendBuf,"OK COPY completed");
				SendLn(sock,SendBuf);
				
				//SendStatus(userdata->USER,userdata->SelectedBoxName);
			}


		}
	
		else if (stricmp(cmd,"CLOSE") == 0)
		{
			
			//scan all box 
			ImapUserList *obj = SearchUserList(userdata->USER);
			
			if (obj != NULL)
			  RemoveUserList(userdata->USER);

		    //CleanDelMail(userdata);
			//MailServerDB->CleanDelMail(userdata->DBAccountId);


			userdata->SelectedBoxName[0] = 0;					
			
			strcat(SendBuf,"OK CLOSE completed");
			SendLn(sock,SendBuf);
		}
		else if (stricmp(cmd,"EXPUNGE") == 0)
		{
           EXPUNGEMail(sock);
		   strcat(SendBuf,"OK EXPUNGE completed");
		   SendLn(sock,SendBuf);

		}

		delete buffer1;

	}
	
	else if (sock->LastCmd == IM_DATA)
	{
		
		userdata->SaveCount -= BytesRead;
		if (userdata->SaveCount <= 0)
		{
		
			unsigned int NewMailUid = 0;

			sock->LastCmd = IM_NORMAL;
			userdata->SaveCount = 0;

			fwrite(buf->buf,1,BytesRead,userdata->Recvfp);

			//處理學習
			fseek(userdata->Recvfp,0,SEEK_END);
			int MailLen = ftell(userdata->Recvfp);

			if (MailLen <= MaxNoSpamSize)
			{
				char *MailBuffer = new char[MailLen+1];
				memset(MailBuffer,0,MailLen+1);

				fseek(userdata->Recvfp,0,SEEK_SET);
				fread(MailBuffer,1,MailLen,userdata->Recvfp);

				
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

					fclose(userdata->Recvfp);
					userdata->Recvfp = NULL; 
								
					return rc;							
				}

				MailHeader *mh = cem.GetMailHeader(mb->mailHeader);


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

											CMailCodec ccd;	
											char *TmpBuffer = new char[strlen(out) + 1];

											ccd.TrimHTML(out,TmpBuffer);
											strcat(MailText,TmpBuffer);

											delete TmpBuffer;

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
				itoa(userdata->DBAccountId,DBNum,10);

				CChecker cc(DBSection,DBNum,"SpamDogServer.ini");

				double SpamRate = MailServerDB->CountSpamRate(&cc,
			 												  MailContent,
															  MailText,
															  mh->From,
															  mh->Subject,
															  mh->SpamDogXMailer);


				

				//更新 SpamDB
				if (userdata->DBSelectedFolderId == Folder_SpamMail)
				{
					MailServerDB->UpdateSpamRate(&cc,-1,1,false);
					
					char *EnvStr = GetEnvelopeStr(userdata->Recvfp);
					if (EnvStr != NULL)
					{
						NewMailUid = MailServerDB->NewImapMail(userdata->DBAccountId,userdata->DBSelectedFolderId,userdata->Status,userdata->InternalDate,EnvStr,userdata->Recvfp);
						delete EnvStr;				
					}

					if (mh->SpamDogXMailer[0] != 0)
					  cc.MainTainRBL(SpamDogCenterIp ,true,mh->SpamDogXMailer);
				
				}
				else if (userdata->DBSelectedFolderId == Folder_LearnOk)
				{			
					//直接 update spam db 不放入 ServerDB
					MailServerDB->UpdateSpamRate(&cc,1,-1,false);

					if (mh->SpamDogXMailer[0] != 0)
					  cc.MainTainRBL(SpamDogCenterIp ,false,mh->SpamDogXMailer);
				}

				
				delete MailText;
				delete MailContent;
				delete MailBuffer;


				cem.FreeMailHeader(mh);
				cem.FreeMailData(mb);
			}
			else
			{
				//超大信件不處理 RBL
				if (userdata->DBSelectedFolderId == Folder_SpamMail)
				{
					
					char *EnvStr = GetEnvelopeStr(userdata->Recvfp);
					if (EnvStr != NULL)
					{
						NewMailUid = MailServerDB->NewImapMail(userdata->DBAccountId,userdata->DBSelectedFolderId,userdata->Status,userdata->InternalDate,EnvStr,userdata->Recvfp);
						delete EnvStr;				
					}
				
				}			
			
			}

			


			fclose(userdata->Recvfp);
			userdata->Recvfp = NULL;

			DeleteFile(userdata->RecvDataFilePath);
			



			//寫入 ServerDB

			/*

			fclose(userdata->Recvfp.EmlFp);

		 
			if (userdata->Recvfp.ImpFp != NULL)
				fclose(userdata->Recvfp.ImpFp);


			//Rename to Nomal data
			 WIN32_FIND_DATA FindFileData;
			 HANDLE hFind;

			 char TmpFilePath[MAX_PATH];
			 strcpy(TmpFilePath,userdata->RecvDataFilePath);
			 strcat(TmpFilePath,"*.tmp");
		

			 hFind = FindFirstFile(TmpFilePath,&FindFileData);
 			 if (hFind != INVALID_HANDLE_VALUE)
			 {
				 do
				 {			
					if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
					{
					

						char FromFilePath[MAX_PATH];
						strcpy(FromFilePath,userdata->RecvDataFilePath);
						strcat(FromFilePath,FindFileData.cFileName);

						char ToFilePath[MAX_PATH];
						strcpy(ToFilePath,userdata->RecvDataFilePath);
						strncat(ToFilePath,FindFileData.cFileName,strlen(FindFileData.cFileName) - 4);

					    MoveFile(FromFilePath,ToFilePath);
						
					}		
				 
				 }while(FindNextFile(hFind, &FindFileData) != 0);

				 
				 FindClose(hFind);
				
			 }

			 userdata->RecvDataFilePath[0] = 0;*/

		
			

            char FolderUIDStr[50];
			char NewMailUidStr[50];

			itoa(userdata->DBSelectedFolderId ,FolderUIDStr , 10);
			itoa(NewMailUid , NewMailUidStr , 10);

			//strcpy(SendBuf,userdata->SaveData);
			strcat(userdata->RecvDataEndCmd,"OK [APPENDUID ");
			strcat(userdata->RecvDataEndCmd,FolderUIDStr);
			strcat(userdata->RecvDataEndCmd," ");
			strcat(userdata->RecvDataEndCmd,NewMailUidStr);
			strcat(userdata->RecvDataEndCmd,"] APPEND completed");			
			
			SendLn(sock,userdata->RecvDataEndCmd);

			delete userdata->RecvDataEndCmd;
			userdata->RecvDataEndCmd = NULL;

			if (userdata->IDLEcmd[0] != 0)
					SendIDLEResult(sock);

			//SendStatus(userdata->DBAccountId,userdata->DBSelectedFolderId);

			

			//memset(&userdata->Recvfp,0,sizeof(userdata->Recvfp));
		
		}
		else
		{
		
			fwrite(buf->buf,1,BytesRead,userdata->Recvfp);

		
		
		}
		
	}



	

	return rc;
}