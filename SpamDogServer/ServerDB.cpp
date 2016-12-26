// ServerDB.cpp: implementation of the CServerDB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerDB.h"
#include "../../SpamDog/Swparser/MailParser.h"


#include <TCHAR.H>
#include <conio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerDB::CServerDB(CRITICAL_SECTION*  DBSection , char* DBFileName, char* AccountDBExt)
{
	   
	   mDBSection = DBSection;
	   mAccountDBExt = AccountDBExt;	   

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
	   
	   strcpy(DBPath,PGPath);
	   strcat(DBPath,"\\DB");
	   CreateDirectory(DBPath,NULL);

	   strcpy(DBFile,DBPath);

	   if (DBFileName == NULL)
	   {
	     strcat(DBFile,"\\AntiSpamServer.db");
	   }
	   else
	   {
		 strcpy(DBFile,DBFileName);	   
	   }

	   

	 


}

CServerDB::~CServerDB()
{


}


bool CServerDB::CheckAccountExists(char* Account)
{	
	AccountIndexDB->OpenDB();

	SResult sres = AccountIndexDB->SelectKey(Account);

	AccountIndexDB->CloseDB();

	if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
	{
		   
		  return false;		  
	}
	else
	{   
		   return true;

	}
}

AccountData CServerDB::GetAccountData(unsigned int AccountId)
{

	AccountData accdata;
	memset(&accdata,0,sizeof(accdata));
	AccountIndexDB->OpenDB();
	AccountIndexDB->SelectData(AccountId,(char *) &accdata,sizeof(accdata));
	AccountIndexDB->CloseDB();
	return accdata;

}

unsigned int CServerDB::GetMailUId(unsigned int MailDataPos)
{

	unsigned int MailUId = 0;

	EnterCriticalSection(mDBSection);

	fseek(DBfp,MailDataPos + FIELD_OFFSET(DBMailData,MailUId) ,SEEK_SET);
	fread(&MailUId,1,sizeof(MailUId),DBfp);

	LeaveCriticalSection(mDBSection);
	return MailUId;


}





unsigned int CServerDB::AddAccount(char* Account)
{

	if (!CheckAccountExists(Account))
	{
		
		EnterCriticalSection(mDBSection);

		AccountData accdata;
		memset(&accdata,0,sizeof(accdata));
		accdata.FolderUIdIndex = IniUserFolderId;
		//accdata.AccountId = sysdata.AccountIdIndex ++;
		strcpy(accdata.ID,Account);

		//建立 system Folder..LearnOkFolder
		MailFolderData folderdata;
		memset(&folderdata,0,sizeof(folderdata));

		folderdata.FolderUId = Folder_LearnOk;
		LoadString(GetModuleHandle(NULL),ID_LEARNOKFOLDER,folderdata.FolderName,sizeof(folderdata.FolderName));

		//Folder Name Convert
		CMailCodec cm; 
		int count = strlen(folderdata.FolderName);
		char *Res = cm.ImapUTF7Encode(folderdata.FolderName,count);
		strcpy(folderdata.FolderName , Res);
		cm.Free(Res);

		fseek(DBfp,0,SEEK_END);
		long WritePos = ftell(DBfp) ;
		fwrite(&folderdata,sizeof(char),sizeof(folderdata),DBfp);
	
		accdata.LearnOkFolderPos = WritePos;
		accdata.FirstMailFolderPos = WritePos;

		//建立 system Folder..SpamBox
		memset(&folderdata,0,sizeof(folderdata));
		folderdata.FolderUId = Folder_SpamMail;
		folderdata.PrevMailFolderDataPos = WritePos;
		LoadString(GetModuleHandle(NULL),ID_SPAMFOLDER,folderdata.FolderName,sizeof(folderdata.FolderName));

		count = strlen(folderdata.FolderName);
		Res = cm.ImapUTF7Encode(folderdata.FolderName,count);
		strcpy(folderdata.FolderName , Res);
		cm.Free(Res);

		char SpamFolderName[50];
		strcpy(SpamFolderName,folderdata.FolderName);

		fseek(DBfp,0,SEEK_END);
		long WritePos2 = ftell(DBfp) ;
		fwrite(&folderdata,sizeof(char),sizeof(folderdata),DBfp);
		accdata.SpamMailFolderPos = WritePos2;

		//再取出前面的內容來做資料對應	 
		fseek(DBfp,WritePos + FIELD_OFFSET(MailFolderData,NextMailFolderDataPos) ,SEEK_SET); //need confirm pos
		fwrite(&WritePos2,sizeof(char) , sizeof(folderdata.NextMailFolderDataPos),DBfp);	

		//建立 system Folder..DelSpam
		memset(&folderdata,0,sizeof(folderdata));
		folderdata.FolderUId = Folder_DelSpam;		
		folderdata.PrevMailFolderDataPos = WritePos2;
		LoadString(GetModuleHandle(NULL),ID_DELSPAMFOLDER,folderdata.FolderName,sizeof(folderdata.FolderName));

		count = strlen(folderdata.FolderName);
		Res = cm.ImapUTF7Encode(folderdata.FolderName,count);
		strcpy(folderdata.FolderName , SpamFolderName); //上層 FolderName
		strcat(folderdata.FolderName , "/");
		strcat(folderdata.FolderName , Res);
		cm.Free(Res);

		fseek(DBfp,0,SEEK_END);
		long WritePos3 = ftell(DBfp) ;
		fwrite(&folderdata,sizeof(char),sizeof(folderdata),DBfp);
		

		//再取出前面的內容來做資料對應	 
		fseek(DBfp,WritePos2 + FIELD_OFFSET(MailFolderData,SubMailFolderDataPos) ,SEEK_SET); //need confirm pos
		fwrite(&WritePos3,sizeof(char) , sizeof(folderdata.SubMailFolderDataPos),DBfp);


		//建立 InBox
		memset(&folderdata,0,sizeof(folderdata));
		folderdata.FolderUId = Folder_InBox;		
		folderdata.PrevMailFolderDataPos = WritePos2;
		LoadString(GetModuleHandle(NULL),ID_INBOXFOLDER,folderdata.FolderName,sizeof(folderdata.FolderName));

		count = strlen(folderdata.FolderName);
		Res = cm.ImapUTF7Encode(folderdata.FolderName,count);	
		strcpy(folderdata.FolderName , Res);
		cm.Free(Res);

		fseek(DBfp,0,SEEK_END);
		long WritePos4 = ftell(DBfp) ;
		fwrite(&folderdata,sizeof(char),sizeof(folderdata),DBfp);
		

		//再取出前面的內容來做資料對應	 
		fseek(DBfp,WritePos2 + FIELD_OFFSET(MailFolderData,NextMailFolderDataPos) ,SEEK_SET);
		fwrite(&WritePos4,sizeof(char) , sizeof(folderdata.NextMailFolderDataPos),DBfp);

	
		AccountIndexDB->OpenDB();
		int datapos = AccountIndexDB->InsertData((char *) &accdata,sizeof(accdata));
		
		accdata.AccountId = datapos; //重要的 key
		//再 update 一次		
		AccountIndexDB->UpdateData(&datapos,sizeof(datapos),datapos,FIELD_OFFSET(AccountData,AccountId)); 
		AccountIndexDB->InsertKey(Account,datapos);

		AccountIndexDB->CloseDB();

		//fseek(DBfp,0,SEEK_SET);
		//fwrite(&sysdata,sizeof(char),sizeof(sysdata),DBfp);		

		


		fflush(DBfp);
		LeaveCriticalSection(mDBSection);

		return accdata.AccountId;
	
	}
	else
	{
		return 0;
	}
}

void CServerDB::DelAccount(char* Account)
{

}

char* CServerDB::GetAccount(unsigned int AccountId)
{


	return NULL;
}

unsigned int CServerDB::GetAccountId(char* Account)
{

	AccountIndexDB->OpenDB();
	SResult sres = AccountIndexDB->SelectKey(Account);
	AccountIndexDB->CloseDB();

	if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
	{
		   return false;		  
	}
	else
	{   
		   if (sres.FindPosInKey != -1)
		   {
		   
			    AccountData accdata;
				memset(&accdata,0,sizeof(accdata));
				AccountIndexDB->OpenDB();
			   	AccountIndexDB->SelectData(sres.DataFilePos,(char *) &accdata , sizeof(accdata));
				AccountIndexDB->CloseDB();

				return accdata.AccountId;
		   
		   }

	}

	return 0;
}

int CServerDB::CheckPersonalIp(unsigned int AccountId,char*IP)
{
	
	//需在思考要放入一般性 rule 中, 或另立  ip db
	return 0;

}

unsigned int  CServerDB::GetFolderUId(char* Account  , char* FolderName)
{

	return 0;
}

unsigned int CServerDB::GetFolderUIdId(unsigned int AccountId, char* FolderName)
{

	return 0;
}

void CServerDB::GetAccountFolderDataPos(unsigned int FolderDataPos,MailFolderData *FolderData)
{

		//MailFolderData folderdata;
		EnterCriticalSection(mDBSection);

	    memset(FolderData,0,sizeof(MailFolderData));

	    fseek(DBfp,FolderDataPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);

		//return folderdata;


}

unsigned int CServerDB::GetAccountFolderNameData(unsigned int AccountId ,char* FolderName,MailFolderData *FolderData)
{

	EnterCriticalSection(mDBSection);
	memset(FolderData,0,sizeof(MailFolderData));

	AccountData accdata =  GetAccountData(AccountId);

	//先讀第一個 FolderData
	
	fseek(DBfp,accdata.FirstMailFolderPos,SEEK_SET);
	
	
	fread(FolderData,1,sizeof(MailFolderData),DBfp);

	if (strcmp(FolderData->FolderName,FolderName) == 0)
	{	
		//match First
		LeaveCriticalSection(mDBSection);
		return accdata.FirstMailFolderPos;	
	}
	else
	{
	    LeaveCriticalSection(mDBSection);
		return SearchFolderDataByName(FolderData,FolderName);	
		
	}
	
	
 

	

}

unsigned int CServerDB::GetAccountFolderData(unsigned int AccountId ,unsigned int FolderUId,MailFolderData *FolderData)
{
//	MailFolderData folderdata;
	

	memset(FolderData,0,sizeof(MailFolderData));

	AccountData accdata =  GetAccountData(AccountId);
	
	if (FolderUId == 0)
	{

		EnterCriticalSection(mDBSection);

		fseek(DBfp,accdata.FirstMailFolderPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);

		return accdata.FirstMailFolderPos;
	
	
	} else	if (FolderUId == Folder_LearnOk)
	{
		EnterCriticalSection(mDBSection);

		fseek(DBfp,accdata.LearnOkFolderPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);
		return accdata.LearnOkFolderPos;

		
	
	}
	else if (FolderUId == Folder_SpamMail)
	{	 

		EnterCriticalSection(mDBSection);

		fseek(DBfp,accdata.SpamMailFolderPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);
		return accdata.SpamMailFolderPos;
	
	}
	else
	{
	
		//先讀出 first folder
		//需要 search folder 的遞迴 func
		EnterCriticalSection(mDBSection);

		fseek(DBfp,accdata.FirstMailFolderPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);
		return SearchFolderData(FolderData,FolderUId);

	
	}
	
	//return folderdata;
}

unsigned int CServerDB::SearchFolderDataByName(MailFolderData *FolderData,char* FolderName)
{

	
	MailFolderData Afolderdata;
	//memset(Afolderdata,0,sizeof(MailFolderData));
	memcpy(&Afolderdata,FolderData,sizeof(MailFolderData));

   
	unsigned int FindFolderDataPos = 0;

   

	//先向右
	if (Afolderdata.SubMailFolderDataPos != 0)
	{
	
		MailFolderData Bfolderdata;	
	    memcpy(&Bfolderdata,&Afolderdata,sizeof(MailFolderData));

	    EnterCriticalSection(mDBSection);
		fseek(DBfp,Afolderdata.SubMailFolderDataPos,SEEK_SET);
		fread(&Bfolderdata,1,sizeof(MailFolderData),DBfp);
		LeaveCriticalSection(mDBSection);

		if (strcmp(Bfolderdata.FolderName,FolderName) == 0 )
		{	
			
			memcpy(FolderData,&Bfolderdata,sizeof(MailFolderData));
			return Afolderdata.SubMailFolderDataPos;
		}
		else
		{
			
			FindFolderDataPos = SearchFolderDataByName(&Bfolderdata,FolderName);

						
			if (FindFolderDataPos != 0)
			{
				//func 結構性問題 , 選擇再讀出一次
				EnterCriticalSection(mDBSection);

				fseek(DBfp,FindFolderDataPos,SEEK_SET);
         		fread(FolderData,1,sizeof(MailFolderData),DBfp);

				LeaveCriticalSection(mDBSection);

				return FindFolderDataPos;
			}
		}
	}
	
	if (Afolderdata.NextMailFolderDataPos != 0)
	{
	
		MailFolderData Bfolderdata;	
	    memcpy(&Bfolderdata,&Afolderdata,sizeof(MailFolderData));
		
		EnterCriticalSection(mDBSection);

		fseek(DBfp,Afolderdata.NextMailFolderDataPos,SEEK_SET);
		fread(&Bfolderdata,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);

		if (strcmp(Bfolderdata.FolderName,FolderName) == 0 )
		{		
			
			memcpy(FolderData,&Bfolderdata,sizeof(MailFolderData));
			return Afolderdata.NextMailFolderDataPos;
		}
		else
		{
			
			FindFolderDataPos =  SearchFolderDataByName(&Bfolderdata,FolderName);

			if (FindFolderDataPos != 0) 
			{
				//func 結構性問題 , 選擇再讀出一次
				EnterCriticalSection(mDBSection);

				fseek(DBfp,FindFolderDataPos,SEEK_SET);
         		fread(FolderData,1,sizeof(MailFolderData),DBfp);

				LeaveCriticalSection(mDBSection);

				return FindFolderDataPos;
			}
		}
	
	}

    
	return 0;

/*
 
	EnterCriticalSection(mDBSection);

	//先向右
	if (FolderData->SubMailFolderDataPos != 0)
	{
	
		fseek(DBfp,FolderData->SubMailFolderDataPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		if (strcmp(FolderData->FolderName , FolderName) == 0 )
		{	
			LeaveCriticalSection(mDBSection);
			return FolderData->SubMailFolderDataPos;
		}
		else
		{
			SearchFolderDataByName(FolderData,FolderName);		
		}
	}
	else if (FolderData->NextMailFolderDataPos != 0)
	{
	
		fseek(DBfp,FolderData->NextMailFolderDataPos,SEEK_SET);
		fread(FolderData,1,sizeof(MailFolderData),DBfp);

		if (strcmp(FolderData->FolderName , FolderName) == 0)
		{		
			LeaveCriticalSection(mDBSection);
			return FolderData->NextMailFolderDataPos;
		}
		else
		{
			SearchFolderDataByName(FolderData,FolderName);		
		}
	
	}

	LeaveCriticalSection(mDBSection);
	
	return 0;

*/

}

unsigned int CServerDB::SearchFolderData(MailFolderData *FolderData,unsigned int FolderUId)
{

	MailFolderData Afolderdata;
	//memset(Afolderdata,0,sizeof(MailFolderData));
	memcpy(&Afolderdata,FolderData,sizeof(MailFolderData));

   
	unsigned int FindFolderDataPos = 0;

   

	//先向右
	if (Afolderdata.SubMailFolderDataPos != 0)
	{
	
		MailFolderData Bfolderdata;	
	    memcpy(&Bfolderdata,&Afolderdata,sizeof(MailFolderData));

	    EnterCriticalSection(mDBSection);
		fseek(DBfp,Afolderdata.SubMailFolderDataPos,SEEK_SET);
		fread(&Bfolderdata,1,sizeof(MailFolderData),DBfp);
		LeaveCriticalSection(mDBSection);

		if (Bfolderdata.FolderUId == FolderUId)
		{	
			
			memcpy(FolderData,&Bfolderdata,sizeof(MailFolderData));
			return Afolderdata.SubMailFolderDataPos;
		}
		else
		{
			
			FindFolderDataPos = SearchFolderData(&Bfolderdata,FolderUId);

						
			if (FindFolderDataPos != 0)
			{
				//func 結構性問題 , 選擇再讀出一次
				EnterCriticalSection(mDBSection);

				fseek(DBfp,FindFolderDataPos,SEEK_SET);
         		fread(FolderData,1,sizeof(MailFolderData),DBfp);

				LeaveCriticalSection(mDBSection);

				return FindFolderDataPos;
			}
		}
	}
	
	if (Afolderdata.NextMailFolderDataPos != 0)
	{
	
		MailFolderData Bfolderdata;	
	    memcpy(&Bfolderdata,&Afolderdata,sizeof(MailFolderData));
		
		EnterCriticalSection(mDBSection);

		fseek(DBfp,Afolderdata.NextMailFolderDataPos,SEEK_SET);
		fread(&Bfolderdata,1,sizeof(MailFolderData),DBfp);

		LeaveCriticalSection(mDBSection);

		if (Bfolderdata.FolderUId == FolderUId)
		{		
			
			memcpy(FolderData,&Bfolderdata,sizeof(MailFolderData));
			return Afolderdata.NextMailFolderDataPos;
		}
		else
		{
			
			FindFolderDataPos =  SearchFolderData(&Bfolderdata,FolderUId);

			if (FindFolderDataPos != 0) 
			{
				//func 結構性問題 , 選擇再讀出一次
				EnterCriticalSection(mDBSection);

				fseek(DBfp,FindFolderDataPos,SEEK_SET);
         		fread(FolderData,1,sizeof(MailFolderData),DBfp);

				LeaveCriticalSection(mDBSection);

				return FindFolderDataPos;
			}
		}
	
	}

    
	return 0;


}

void CServerDB::UpdateSpamRate(CChecker *cc , int OKFix, int BadFix,bool AutoUpdate )
{
	cc->UpdateSpamRate(OKFix,BadFix,AutoUpdate);
}

double CServerDB::CountSpamRate(CChecker *cc ,  char* MailBuff,char* MailTextbuf,char* Sender,char* Subject,char* IP)
{
	


	double mailrate = 0.5;
	

	//CChecker mc(mDBSection,"SpamDogServer.ini",DBNum);

	cc->GetCheckKeys(MailBuff,MailTextbuf,Sender,Subject,IP,true);

	if (cc->ResList.size() > 0  || cc->SecResList.size() > 0)
	{
	
		cc->GetSpamData();
		
		if (cc->DataList.size()  > 0 || cc->SecDataList.size() > 0)
		{
			mailrate = cc->GetSpamRate(0.5);//unknown rate = 0.5		
		}	
	}

	int     decimal,   sign;
    char    *buffer;
    int     precision = 10;
     

    buffer = _ecvt( mailrate, precision, &decimal, &sign );
	_cprintf("Mail Rate : %s \r\n",buffer);

	if (cc->DomainList.size() > 0)
	{
		    _cprintf("ResolveDomains \r\n");

			if (cc->ResolveDomains() == S_OK)
			{
				//有得到 ip													
				mailrate = cc->GetDomainSpamRate();

				_cprintf("ResolveDomains OK !\r\n");
													
			}
	
	
	}

	//可以考慮統一做 domain resolve , 目前先不用
	/*
	if (mc->DomainList.size() > 0 )
										{
											    
												if (mc->ResolveDomains() == S_OK)
												{
													//有得到 ip
													
													mailrate = mc->GetDomainSpamRate();
													
												}
										}
										*/
	

	// 可以維護 RBL 目前先不用
	//mc->MainTainRBL(dlg->Settings.ServerIp,false,IP);

	return mailrate;
}

void CServerDB::OpenDB()
{

	   char IndexDbPath[_MAX_PATH];
	   char overflowDbPath[_MAX_PATH];
	   char datadbPath[_MAX_PATH];	   
   
	   strcpy(IndexDbPath,DBPath);
	   strcpy(overflowDbPath,DBPath);
	   strcpy(datadbPath,DBPath);

	   
	     strcat(IndexDbPath,"\\Account.db1");
	     strcat(overflowDbPath,"\\Account.db2");
	     strcat(datadbPath,"\\Account.db3");
	   
	   if (mAccountDBExt != NULL)
	   {
		   strcat(IndexDbPath,mAccountDBExt);
		   strcat(overflowDbPath,mAccountDBExt);
	       strcat(datadbPath,mAccountDBExt);	   	   
	   }

	   AccountIndexDB = new CDB(mDBSection,IndexDbPath ,overflowDbPath ,datadbPath ,0,1024 * 10);
	   
	   
	   DBfp = NULL;
	   DBfp = fopen(DBFile,"r+b");
	   if (DBfp == NULL)
	   {
			DBfp = fopen(DBFile,"w+b");
			SystemData sysd;
			memset(&sysd,0,sizeof(sysd));
			fwrite(&sysd,1,sizeof(sysd),DBfp);

			AccountIndexDB->OpenDB();
			AccountIndexDB->InsertData(&sysd,sizeof(SystemData));
			AccountIndexDB->CloseDB();

	   }

	   fseek(DBfp,0,SEEK_SET);
	   fread(&sysdata,sizeof(char),sizeof(sysdata),DBfp);


	  


	   

}

void CServerDB::FlushDB()
{

	fflush(DBfp);
}

void CServerDB::CloseDB()
{

		fclose(DBfp);

	//	AccountIndexDB->CloseDB();
		delete AccountIndexDB;
}


//Mail Func
//void CServerDB::NewMail(char* Account,unsigned int FolderUId,char* MailFrom , AccountList* RcptList,char* MailContent )
//{

//}	

void CServerDB::RelayDBMail(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId)
{
	DBMailData MailData;
	memset(&MailData,0,sizeof(MailData));

	unsigned int DataPos = ListMail(AccountId,FolderUId,MailUId,&MailData);

	if (MailData.MailUId != 0)
	{	

	
			EnterCriticalSection(mDBSection);
	
			
			SystemData sysdata;
			memset(&sysdata , 0 , sizeof(sysdata));

			fseek(DBfp,0,SEEK_SET);
			fread(&sysdata,1,sizeof(sysdata),DBfp);

			RelayInfo relayinfo;
			memset(&relayinfo,0,sizeof(relayinfo));

			relayinfo.RawMailDataPos = MailData.RawMailDataPos;
			relayinfo.RawMailDataLen = MailData.RawMailDataLen;

			if (sysdata.LastRelayedInfoPos  == 0)
			{				
				fseek(DBfp,0,SEEK_END);
				int WritePos = ftell(DBfp);
				fwrite(&relayinfo,1,sizeof(relayinfo),DBfp);

				sysdata.FirstRelayInfoPos = WritePos;
				sysdata.LastRelayInfoPos = WritePos;
			
			}
			else
			{
				

				relayinfo.PrevRelayInfoPos = sysdata.LastRelayInfoPos;
			
				fseek(DBfp,0,SEEK_END);
				int WritePos = ftell(DBfp);
				fwrite(&relayinfo,1,sizeof(relayinfo),DBfp);

				//上對下
				fseek(DBfp,sysdata.LastRelayInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos), SEEK_SET);
				fwrite(&WritePos,1,sizeof(WritePos),DBfp);			
			
			}

			//更新 sysinfo
			fseek(DBfp,0,SEEK_SET);
			fwrite(&sysdata,1,sizeof(sysdata),DBfp);

			fflush(DBfp);
	   

			LeaveCriticalSection(mDBSection);

	}
}

void CServerDB::RelayMail(FILE *fp)
{
		EnterCriticalSection(mDBSection);

		fseek(fp,0,SEEK_SET);
		fseek(DBfp,0,SEEK_END);

		int RawMailPos = ftell(DBfp);

		char MailBuffer[1024]={0};
		int ReadSize = 0;
		int ReadCount = 0;
	
		

		while ((ReadSize = fread(MailBuffer,1,sizeof(MailBuffer)-1,fp)) > 0)
		{			

			ReadCount +=  ReadSize;			
			fwrite(MailBuffer,1,ReadSize,DBfp);			
		}

		if (ReadCount > 0)
		{
			fseek(DBfp,0,SEEK_SET);
			
			SystemData sysdata;
			memset(&sysdata , 0 , sizeof(sysdata));

			fread(&sysdata,1,sizeof(sysdata),DBfp);

			RelayInfo relayinfo;
			memset(&relayinfo,0,sizeof(relayinfo));

			relayinfo.RawMailDataPos = RawMailPos;
			relayinfo.RawMailDataLen = ReadCount;

			if (sysdata.LastRelayedInfoPos  == 0)
			{				
				fseek(DBfp,0,SEEK_END);
				int WritePos = ftell(DBfp);
				fwrite(&relayinfo,1,sizeof(relayinfo),DBfp);

				sysdata.FirstRelayInfoPos = WritePos;
				sysdata.LastRelayInfoPos = WritePos;
			
			}
			else
			{
				

				relayinfo.PrevRelayInfoPos = sysdata.LastRelayInfoPos;
			
				fseek(DBfp,0,SEEK_END);
				int WritePos = ftell(DBfp);
				fwrite(&relayinfo,1,sizeof(relayinfo),DBfp);

				//上對下
				fseek(DBfp,sysdata.LastRelayInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos), SEEK_SET);
				fwrite(&WritePos,1,sizeof(WritePos),DBfp);			
			
			}

			//更新 sysinfo
			fseek(DBfp,0,SEEK_SET);
			fwrite(&sysdata,1,sizeof(sysdata),DBfp);

			fflush(DBfp);


		
		}

		LeaveCriticalSection(mDBSection);


}

unsigned int CServerDB::NewImapMail(unsigned int AccountId,unsigned int FolderUId,unsigned int Flag ,char* InternalDate ,char* ImapEnvelopeStr, FILE *fp , unsigned int MailUId)
{	
	

	unsigned RtnMailUid = 0;

	MailFolderData folderdata ;	
	unsigned int FolderDataPos = GetAccountFolderData(AccountId , FolderUId,&folderdata);
	
	if (folderdata.FolderUId != 0 )
	{ 
		EnterCriticalSection(mDBSection);
		
		DBMailData MailData;
		memset(&MailData,0,sizeof(MailData));
		
		//先寫資料在 update folder
		
		fseek(fp,0,SEEK_SET);
		fseek(DBfp,0,SEEK_END);

		
		int RawMailPos =ftell(DBfp);
		 

		char MailBuffer[1024]={0};
		int ReadSize = 0;
		int ReadCount = 0;
		char *FindPos = NULL;
		int HeaderLen = -1;

		

		while ((ReadSize = fread(MailBuffer,1,sizeof(MailBuffer)-1,fp)) > 0)
		{			
			//找 headerpos
			if (FindPos == NULL)
			{
				MailBuffer[ReadSize] = 0;
				FindPos = _tcsstr(MailBuffer,"\r\n\r\n");
				if (FindPos != NULL)
				{
				    HeaderLen= ReadCount + (FindPos - MailBuffer);				
				}
			}

			ReadCount +=  ReadSize;
			
			fwrite(MailBuffer,1,ReadSize,DBfp);

			
		}

		if (ReadCount > 0) 
		{
			MailData.RawMailDataPos = RawMailPos;
			MailData.RawMailDataLen = ReadCount;
			if (MailUId != 0)
			{
				folderdata.MailUIdIndex = MailUId;
				MailData.MailUId = MailUId;
			}
			else
			{
				folderdata.MailUIdIndex ++;
				MailData.MailUId = folderdata.MailUIdIndex ;
			}

			RtnMailUid = MailData.MailUId;
			folderdata.Exists ++;

			//char nums[50];
			
			//itoa(folderdata.Exists,nums,10);
		//	OutputDebugString("NewImapMail Exits:");
			//OutputDebugString(nums);
			//OutputDebugString("\r\n");

		 
			
			if (InternalDate != NULL)
		      strcpy(MailData.ImapInternalDate,InternalDate);
			
			MailData.Status = Flag;

			MailData.ImapEnvelopeLen = strlen(ImapEnvelopeStr);

			fseek(DBfp,0,SEEK_END);
			int Pos = ftell(DBfp);

			MailData.ImapEnvelopePos = Pos;
			fwrite(ImapEnvelopeStr,1,MailData.ImapEnvelopeLen,DBfp);

			//fseek(DBfp,0,SEEK_END);
			//Pos = ftell(DBfp);

			MailData.HeaderDataPos = RawMailPos; //與 RawMail Pos 共用
			MailData.HeaderDataLen = HeaderLen;

			//fseek(fp,0,SEEK_SET);

			//char *tmpbuff = new char[MailData.HeaderDataLen ];
			//memset(tmpbuff,0,MailData.HeaderDataLen);
			//fread(tmpbuff,1,MailData.HeaderDataLen,fp);
			//fwrite(tmpbuff,1,MailData.HeaderDataLen ,DBfp);
			//delete tmpbuff;

			if(folderdata.FirstMailDataPos > 0)
			{	
				//如果 Folder 有信

				int DataOffset = FIELD_OFFSET(DBMailData,ImapSeqNo);
				unsigned int pSeqNo = 0;
				fseek(DBfp,folderdata.LastMailDataPos + DataOffset,SEEK_SET);				
				fread(&pSeqNo,1,sizeof(pSeqNo),DBfp);
				MailData.ImapSeqNo = pSeqNo + 1;
				MailData.PrevMailDataPos = folderdata.LastMailDataPos;

				fseek(DBfp,0,SEEK_END);
				int MailPos = ftell(DBfp);

				fwrite(&MailData,1,sizeof(MailData),DBfp);

				//更新上層			 
				DataOffset = FIELD_OFFSET(DBMailData,NextMailDataPos);
				fseek(DBfp,folderdata.LastMailDataPos + DataOffset,SEEK_SET);
				fwrite(&MailPos,1,sizeof(MailPos),DBfp);

			
				folderdata.LastMailDataPos = MailPos;				
				folderdata.LastMailUID = MailData.MailUId ;

						
			}
			else
			{
				MailData.ImapSeqNo = 1;	

				fseek(DBfp,0,SEEK_END);
				int MailPos = ftell(DBfp);


				fwrite(&MailData,1,sizeof(MailData),DBfp);
				
				folderdata.FirstMailDataPos = MailPos;
				folderdata.FirstMailUID = MailData.MailUId ;
				folderdata.LastMailDataPos = MailPos;
				folderdata.LastMailUID = MailData.MailUId ;
						
			}

			if ((Flag & Flag_Recent)  == Flag_Recent)
			{
				folderdata.Recent ++;					
			} 
			
			if ((Flag & Flag_Seen)  == Flag_Seen)
			{
				folderdata.Seen ++;
				//folderdata.FirstUnseen = MailData.ImapSeqNo;
					
			}

			//fseek(DBfp,FolderDataPos,SEEK_SET);

			fseek(DBfp,FolderDataPos,SEEK_SET);
			fwrite(&folderdata,1,sizeof(folderdata),DBfp);
	
		}
		

		
	
		fflush(DBfp);
		LeaveCriticalSection(mDBSection);

	
	}
 

	return RtnMailUid;

}

unsigned int CServerDB::ListFirstRelayInfo(RelayInfo *rinfo)
{

	//取 sysdata
	EnterCriticalSection(mDBSection);

	SystemData sysdata;
	memset(&sysdata,0,sizeof(sysdata));

	fseek(DBfp,0,SEEK_SET);
	fread(&sysdata,1,sizeof(sysdata),DBfp);


	fseek(DBfp,sysdata.FirstRelayInfoPos,SEEK_SET);
	fread(rinfo,1,sizeof(RelayInfo),DBfp);

	LeaveCriticalSection(mDBSection);

	return sysdata.FirstRelayInfoPos;
}

unsigned int CServerDB::ListNextRelayInfo(RelayInfo *rinfo)
{

	unsigned int SavePos = 0 ;

	EnterCriticalSection(mDBSection);

	if (rinfo->NextRelayInfoPos > 0)
	{
		fseek(DBfp,rinfo->NextRelayInfoPos,SEEK_SET);
		SavePos = rinfo->NextRelayInfoPos;

		fread(rinfo,1,sizeof(RelayInfo),DBfp);

	}

	LeaveCriticalSection(mDBSection);
	
	return SavePos;
	

}

char *CServerDB::GetDBFile()
{
	return DBFile;

}

void CServerDB::DumpMail(RelayInfo *rinfo , char* DumpBuffer , unsigned int BufferSize , unsigned int FpPos)
{
	EnterCriticalSection(mDBSection);

	fseek(DBfp,FpPos,SEEK_SET);

	if (rinfo->RawMailDataLen < BufferSize)
	{
	
		fread(DumpBuffer,1,rinfo->RawMailDataLen,DBfp);
		DumpBuffer[rinfo->RawMailDataLen] = 0;

	}
	else
	{
		fread(DumpBuffer,1,BufferSize-1,DBfp);
		DumpBuffer[BufferSize] = 0;
	
	
	}

	LeaveCriticalSection(mDBSection);

}


void CServerDB::SetRelayedInfo(RelayInfo rinfo) //已寄出的 mail queue
{	

	//處理拔出
	EnterCriticalSection(mDBSection);

	SystemData sysdata;
	memset(&sysdata,0,sizeof(sysdata));

	fseek(DBfp,0,SEEK_SET);
	fread(&sysdata,1,sizeof(sysdata),DBfp);

	unsigned int SelfDataPos = 0;

	if (rinfo.NextRelayInfoPos == 0 && rinfo.PrevRelayInfoPos == 0)
	{
		//在 head 歸零
		sysdata.FirstRelayInfoPos = 0;
		sysdata.LastRelayInfoPos = 0;
	
	}
	else if (rinfo.PrevRelayInfoPos == 0)
	{
	
		//自己是頭
		if (sysdata.FirstRelayInfoPos == sysdata.LastRelayInfoPos )
		{
			sysdata.LastRelayInfoPos = rinfo.NextRelayInfoPos;
		}

		SelfDataPos = sysdata.FirstRelayInfoPos ;
		sysdata.FirstRelayInfoPos = rinfo.NextRelayInfoPos;

		//下對上		
		unsigned int Zeroint = 0;
		fseek(DBfp,rinfo.NextRelayInfoPos + FIELD_OFFSET(RelayInfo,PrevRelayInfoPos),SEEK_SET);
		fwrite(&Zeroint,1,sizeof(Zeroint),DBfp);

	
	} 
	else if (rinfo.NextRelayInfoPos == 0)
	{
	
		//自己是尾		
		SelfDataPos = sysdata.LastRelayInfoPos;
		sysdata.LastRelayInfoPos = rinfo.PrevRelayInfoPos;

		//上對下
		unsigned int Zeroint = 0;
		fseek(DBfp,rinfo.PrevRelayInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos),SEEK_SET);
		fwrite(&Zeroint,1,sizeof(Zeroint),DBfp);
	
	}
	else
	{
		//自己在中間

		fseek(DBfp,rinfo.PrevRelayInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos),SEEK_SET);
		fread(&SelfDataPos,1,sizeof(SelfDataPos),DBfp);		 
		
		//上對下		
		fseek(DBfp,rinfo.PrevRelayInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos),SEEK_SET);
		fwrite(&rinfo.NextRelayInfoPos,1,sizeof(rinfo.NextRelayInfoPos),DBfp);

		//下對上
		fseek(DBfp,rinfo.NextRelayInfoPos+ FIELD_OFFSET(RelayInfo,PrevRelayInfoPos),SEEK_SET);
		fwrite(&rinfo.PrevRelayInfoPos,1,sizeof(rinfo.PrevRelayInfoPos),DBfp);


	}

	//處理插入
	if (sysdata.FirstRelayedInfoPos == 0 && sysdata.LastRelayedInfoPos == 0)
	{
		sysdata.FirstRelayedInfoPos = SelfDataPos;
		sysdata.LastRelayedInfoPos = SelfDataPos;
		
		rinfo.PrevRelayInfoPos = 0;
		rinfo.PrevRelayInfoPos = 0;

		fseek(DBfp,SelfDataPos,SEEK_SET);
		fwrite(&rinfo,1,sizeof(RelayInfo),DBfp);

	}
	else
	{

		fseek(DBfp,sysdata.LastRelayedInfoPos + FIELD_OFFSET(RelayInfo,NextRelayInfoPos),SEEK_SET);
		fwrite(&SelfDataPos,1,sizeof(SelfDataPos),DBfp);

		rinfo.PrevRelayInfoPos = sysdata.LastRelayedInfoPos;
		rinfo.PrevRelayInfoPos = 0;

		sysdata.LastRelayedInfoPos = SelfDataPos;

		fseek(DBfp,SelfDataPos,SEEK_SET);
		fwrite(&rinfo,1,sizeof(RelayInfo),DBfp);
	
	}


	fseek(DBfp,0,SEEK_SET);
	fwrite(&sysdata,1,sizeof(sysdata),DBfp);
	fflush(DBfp);

	LeaveCriticalSection(mDBSection);




}

unsigned int CServerDB::ListMail(unsigned int AccountId ,unsigned int FolderUId,unsigned int MailUId,DBMailData *MailData)
{

	//如果 MailUId = 0 , 則從第一封開始列
	EnterCriticalSection(mDBSection);

	memset(MailData,0,sizeof(DBMailData));
	 
	MailFolderData folderdata ;
	unsigned int FolderDataPos  = GetAccountFolderData(AccountId , FolderUId,&folderdata);

	if (folderdata.FirstMailDataPos > 0)
	{
		
		fseek(DBfp,folderdata.FirstMailDataPos,SEEK_SET);
		fread(MailData,1,sizeof(DBMailData),DBfp);

		if (MailData->MailUId == MailUId || MailUId == 0)
		{
			
			LeaveCriticalSection(mDBSection);
			return folderdata.FirstMailDataPos;
		}
		else
		{

			while (MailData->NextMailDataPos > 0)
			{
				unsigned int SavePos = MailData->NextMailDataPos; 
				fseek(DBfp,MailData->NextMailDataPos,SEEK_SET);
				fread(MailData,1,sizeof(DBMailData),DBfp);

				if (MailData->MailUId == MailUId)
				{				
					LeaveCriticalSection(mDBSection);
					return SavePos;
				}		
			}	
		
		}



	
	}



	memset(MailData,0,sizeof(DBMailData)); //如果都沒找到,就清空

	LeaveCriticalSection(mDBSection);
	return 0;
}

unsigned int CServerDB::ListFromMail(unsigned int AccountId ,unsigned int FolderUId,unsigned int MailUId,DBMailData *MailData)
{

	//如果 MailUId = 0 , 則從第一封開始列
	EnterCriticalSection(mDBSection);

	memset(MailData,0,sizeof(DBMailData));
	 
	MailFolderData folderdata ;
	unsigned int FolderDataPos  = GetAccountFolderData(AccountId , FolderUId,&folderdata);

	if (folderdata.FirstMailDataPos > 0)
	{
		
		fseek(DBfp,folderdata.FirstMailDataPos,SEEK_SET);
		fread(MailData,1,sizeof(DBMailData),DBfp);

		if (MailData->MailUId == MailUId || MailUId == 0 || MailData->MailUId > MailUId)
		{
			LeaveCriticalSection(mDBSection);
			return folderdata.FirstMailDataPos;
		}
		else
		{

			while (MailData->NextMailDataPos > 0)
			{
				unsigned int SavePos = MailData->NextMailDataPos; 
				fseek(DBfp,MailData->NextMailDataPos,SEEK_SET);
				fread(MailData,1,sizeof(DBMailData),DBfp);

				if (MailData->MailUId == MailUId || MailData->MailUId > MailUId)
				{				
					LeaveCriticalSection(mDBSection);
					return SavePos;
				}		
			}	
		
		}



	
	}



	memset(MailData,0,sizeof(DBMailData)); //如果都沒找到,就清空

	LeaveCriticalSection(mDBSection);
	return 0;
}


char* CServerDB::RetriveMail(DBMailData* MailData)
{

	EnterCriticalSection(mDBSection);

	fseek(DBfp,MailData->RawMailDataPos,SEEK_SET);

	char *Buff = new char [MailData->RawMailDataLen + 1];
	memset(Buff,0,MailData->RawMailDataLen + 1);

	fread(Buff,1,MailData->RawMailDataLen,DBfp);


	LeaveCriticalSection(mDBSection);
	return Buff;
}

char* CServerDB::RetriveMailHeader(DBMailData* MailData)
{


	EnterCriticalSection(mDBSection);

	fseek(DBfp,MailData->HeaderDataPos,SEEK_SET);

	char *Buff = new char [MailData->HeaderDataLen + 1];
	memset(Buff,0,MailData->HeaderDataLen + 1);

	fread(Buff,1,MailData->HeaderDataLen,DBfp);


	LeaveCriticalSection(mDBSection);
	return Buff;

}

char* CServerDB::RetriveImapEnvelope(DBMailData* MailData)
{
	EnterCriticalSection(mDBSection);

	fseek(DBfp,MailData->ImapEnvelopePos,SEEK_SET);

	char *Buff = new char [MailData->ImapEnvelopeLen + 1];
	memset(Buff,0,MailData->ImapEnvelopeLen + 1);

	fread(Buff,1,MailData->ImapEnvelopeLen,DBfp);

	LeaveCriticalSection(mDBSection);

	return Buff;
}

void CServerDB::FreeMail(char* mail)
{
	delete mail;

}



unsigned int CServerDB::ListNextMail(DBMailData* MailData)
{
	EnterCriticalSection(mDBSection);

	if (MailData->NextMailDataPos > 0)
	{
	
		unsigned int SavePos = MailData->NextMailDataPos;
		fseek(DBfp,MailData->NextMailDataPos,SEEK_SET);
		fread(MailData,1,sizeof(DBMailData),DBfp);

		LeaveCriticalSection(mDBSection);
		return SavePos;

		
	
	}
	else
	{
		memset(MailData,0,sizeof(DBMailData));

	
	}

	LeaveCriticalSection(mDBSection);
	return 0;
}


void CServerDB::SetMailStatus(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId,unsigned int Status)
{

	DBMailData MailData;
	memset(&MailData,0,sizeof(MailData));

	unsigned int DataPos = ListMail(AccountId,FolderUId,MailUId,&MailData);

	if (MailData.MailUId != 0)
	{		
		//
		EnterCriticalSection(mDBSection);

		MailFolderData FolderData;
		unsigned int fDataPos = GetAccountFolderData(AccountId,FolderUId,&FolderData);

		//if ((MailData.Status & Flag_Recent)  == Flag_Recent)
		//{
		//		FolderData.Recent --;

					
		//} else

		//本來沒有 Seen
		if (((MailData.Status & Flag_Seen) !=  Flag_Seen) &&  ((Status & Flag_Seen)  == Flag_Seen) ) //delete 不處理 seen counter
		{
				FolderData.Seen ++;
				
			
				//folderdata.FirstUnseen = MailData.ImapSeqNo;
				

				//fseek(DBfp,fDataPos+FIELD_OFFSET(MailFolderData,Seen) , SEEK_SET);
				//fwrite(&FolderData.Recent,1,sizeof(FolderData.Seen),DBfp);
				fseek(DBfp,fDataPos, SEEK_SET);
				fwrite(&FolderData,1,sizeof(FolderData),DBfp);
					
		}
		else if (((MailData.Status & Flag_Seen) ==  Flag_Seen) &&  ((Status & Flag_Seen)  != Flag_Seen)  )
		{
			//Seen Check
		    // -FLAGS Seen

			FolderData.Seen --;
			fseek(DBfp,fDataPos, SEEK_SET);
			fwrite(&FolderData,1,sizeof(FolderData),DBfp);
		}

		/*if (((MailData.Status & Flag_Deleted) !=  Flag_Deleted) &&  (Status & Flag_Deleted)  == Flag_Deleted)
		{
				//FolderData.Exists --;

				//if ((MailData.Status & Flag_Seen) ==  Flag_Seen)
				//{				
				//	FolderData.Seen --;
				//}
			
				//if ((MailData.Status & Flag_Recent) ==  Flag_Recent)
				//{				
				//	FolderData.Recent --;
				//}
			     
								

				fseek(DBfp,fDataPos, SEEK_SET);
				fwrite(&FolderData,1,sizeof(FolderData),DBfp);
					
		}*/
		
		


		if ((MailData.Status & Flag_Recent) ==  Flag_Recent)
		{
				
					FolderData.Recent --;
					MailData.Status &= (~Flag_Recent);

		}
		
		
		
		fseek(DBfp,DataPos + FIELD_OFFSET(DBMailData,Status) , SEEK_SET);
		fwrite(&Status,1,sizeof(Status),DBfp);

		//修改 Folder Counter
		//MailData.Status
		fflush(DBfp);
		LeaveCriticalSection(mDBSection);


	}

}

void CServerDB::SetImapSeqNo(unsigned int MailDataPos , unsigned int SeqNo)
{
	EnterCriticalSection(mDBSection);

	fseek(DBfp,MailDataPos + FIELD_OFFSET(DBMailData,ImapSeqNo) , SEEK_SET);
	fwrite(&SeqNo , 1, sizeof(SeqNo) , DBfp);

	//fflush(DBfp);

	LeaveCriticalSection(mDBSection);
}

unsigned int  CServerDB::DelMail(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId)
{

	DBMailData MailData;
	memset(&MailData,0,sizeof(MailData));

	unsigned int DataPos = ListMail(AccountId,FolderUId,MailUId,&MailData);

	if (MailData.MailUId > 0)
	{		
		EnterCriticalSection(mDBSection);
		//刪除就是把 MailUID = 0
		MailFolderData FolderData;
		unsigned int fDataPos = GetAccountFolderData(AccountId,FolderUId,&FolderData);
	
		if (MailData.PrevMailDataPos > 0)
		{
			//父子對應
			
			fseek(DBfp,MailData.PrevMailDataPos +			 
				FIELD_OFFSET(DBMailData,NextMailDataPos) , SEEK_SET);
			fwrite(&MailData.NextMailDataPos ,1,sizeof(MailData.NextMailDataPos) ,DBfp);

			//子父對應
			if (MailData.NextMailDataPos > 0)
			{
				fseek(DBfp,MailData.NextMailDataPos +			 
					FIELD_OFFSET(DBMailData,PrevMailDataPos) , SEEK_SET);
				fwrite(&MailData.PrevMailDataPos ,1,sizeof(MailData.PrevMailDataPos) ,DBfp);			

				if (FolderData.LastMailUID == MailData.MailUId)
				{				
					FolderData.LastMailUID = GetMailUId(MailData.NextMailDataPos);
					FolderData.LastMailDataPos = MailData.NextMailDataPos;
				}
			}
			else
			{
				//沒有子代
				FolderData.LastMailUID = GetMailUId(MailData.PrevMailDataPos);
				FolderData.LastMailDataPos = MailData.PrevMailDataPos;

			
			}
		}
		else
		{
		
			//自己是 First
			if (MailData.NextMailDataPos > 0 )
			{
			
				FolderData.FirstMailDataPos = MailData.NextMailDataPos;				
				

				unsigned int NextMailUId = GetMailUId(MailData.NextMailDataPos); 
				FolderData.FirstMailUID = NextMailUId;

				//Mail Data 上層拔除
				unsigned int tmpi = 0;
				fseek(DBfp,MailData.NextMailDataPos +			 
					FIELD_OFFSET(DBMailData,PrevMailDataPos) , SEEK_SET);
				fwrite(&tmpi ,1,sizeof(tmpi) ,DBfp);

				/*if (FolderData.LastMailUID == MailData.MailUId)
				{
					//最後也是自己
					FolderData.LastMailUID = 0;// NextMailUId;
					FolderData.LastMailDataPos = 0;//MailData.NextMailDataPos;

				
				}*/
			
			}
			else
			{			
				FolderData.FirstMailUID = 0;
				FolderData.FirstMailDataPos = 0;
				FolderData.LastMailDataPos = 0;
				FolderData.LastMailUID = 0;
				
			}		
		}

		//Counter Status cacul
		FolderData.Exists --;
			//char nums[50];
			
			//itoa(FolderData.Exists,nums,10);
			//OutputDebugString("DelMail Exits:");
			//OutputDebugString(nums);
			//OutputDebugString("\r\n");


		if ((MailData.Status & Flag_Seen)  == Flag_Seen)
		{
				FolderData.Seen --;			
		}

		if ((MailData.Status & Flag_Recent)  == Flag_Recent)
		{
				FolderData.Recent --;			
		}


		//Folder Update
		fseek(DBfp,fDataPos,SEEK_SET);
		fwrite(&FolderData,1,sizeof(FolderData),DBfp);

		//Mail Update
		MailData.MailUId = 0;
		
		fseek(DBfp,DataPos + FIELD_OFFSET(DBMailData,MailUId), SEEK_SET);
		fwrite(&MailData.MailUId,1,sizeof(MailData.MailUId),DBfp);

		fflush(DBfp);
		LeaveCriticalSection(mDBSection);
	}

	return DataPos;


}

unsigned int CServerDB::MoveMail(unsigned int AccountId,unsigned int OldFolderUId,unsigned int OldMailUId , unsigned int NewFolderUId)
{

 	unsigned int MailDataPos = DelMail(AccountId,OldFolderUId,OldMailUId);

	if (MailDataPos > 0)
	{	
			MailFolderData folderdata ;	
			unsigned int FolderDataPos = GetAccountFolderData(AccountId , NewFolderUId,&folderdata);
			
			if (folderdata.FolderUId != 0 )
			{ 
				EnterCriticalSection(mDBSection);
				
				DBMailData MailData;
				memset(&MailData,0,sizeof(MailData));
				fseek(DBfp,MailDataPos,SEEK_SET);
				fread(&MailData,1,sizeof(MailData),DBfp);

				
								
				folderdata.MailUIdIndex ++;
				//寫入 MailData
				MailData.MailUId = folderdata.MailUIdIndex ;
				folderdata.Exists ++;

			//	char nums[50];
			
			//	itoa(folderdata.Exists,nums,10);
			//	OutputDebugString("MoveMail Exits:");
			//	OutputDebugString(nums);
			//	OutputDebugString("\r\n");

				if(folderdata.FirstMailDataPos > 0)
				{	
						//如果 Folder 有信

						int DataOffset = FIELD_OFFSET(DBMailData,ImapSeqNo);
						unsigned int pSeqNo = 0;
						fseek(DBfp,folderdata.LastMailDataPos + DataOffset,SEEK_SET);				
						fread(&pSeqNo,1,sizeof(pSeqNo),DBfp);
						MailData.ImapSeqNo = pSeqNo + 1;
						MailData.PrevMailDataPos = folderdata.LastMailDataPos;

						fseek(DBfp,MailDataPos,SEEK_SET);
						fwrite(&MailData,1,sizeof(MailData),DBfp);

						//更新上層			 
						DataOffset = FIELD_OFFSET(DBMailData,NextMailDataPos);
						fseek(DBfp,folderdata.LastMailDataPos + DataOffset,SEEK_SET);
						fwrite(&MailDataPos,1,sizeof(MailDataPos),DBfp);

					
						folderdata.LastMailDataPos = MailDataPos;				
						folderdata.LastMailUID = MailData.MailUId ;

								
				}
				else
				{
						MailData.ImapSeqNo = 1;	
					
						fseek(DBfp,MailDataPos,SEEK_SET);
						fwrite(&MailData,1,sizeof(MailData),DBfp);
						
						folderdata.FirstMailDataPos = MailDataPos;
						folderdata.FirstMailUID = MailData.MailUId ;
						folderdata.LastMailDataPos = MailDataPos;
						folderdata.LastMailUID = MailData.MailUId ;
								
				}

				if ((MailData.Status & Flag_Recent)  == Flag_Recent)
				{
						folderdata.Recent ++;					
				} 
					
				if ((MailData.Status & Flag_Seen)  == Flag_Seen)
				{
						folderdata.Seen ++;
						//folderdata.FirstUnseen = MailData.ImapSeqNo;
							
				}

					//fseek(DBfp,FolderDataPos,SEEK_SET);

				fseek(DBfp,FolderDataPos,SEEK_SET);
				fwrite(&folderdata,1,sizeof(folderdata),DBfp);
			
				
				

				
			
				fflush(DBfp);
				LeaveCriticalSection(mDBSection);

	
			}

	}

	return 0;
}

void CServerDB::FixSpamRate(unsigned int FolderUId,unsigned int MailUId , int OkRate , int BadRate)
{

}

