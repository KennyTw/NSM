// ServerDB.h: interface for the CServerDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERDB_H__2BBA138F_E28C_4BA5_A84F_397DF7B24B96__INCLUDED_)
#define AFX_SERVERDB_H__2BBA138F_E28C_4BA5_A84F_397DF7B24B96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Windows.h>
#include <stdlib.h>
#include "../../SpamDog/Swfiledb/DB.h"
#include "../../SpamDog/SwSpam/Checker.h"


#define System_Account Admin

#define Folder_InBox	10
#define Folder_SpamMail	11
#define Folder_LearnOk  12
#define Folder_DelSpam	13
#define SysFolder_Incoming 1
#define IniUserFolderId  100


#define Flag_Seen	1
#define Flag_Answered	2
#define Flag_Flagged	4
#define Flag_Deleted	8
#define Flag_Draft	16
#define Flag_Recent	32




typedef struct _AccountData
{
	char ID[20];
	unsigned int AccountId; //就是 data pos
	unsigned int FirstMailFolderPos;
	unsigned int LastMailFolderPos;
	unsigned int LearnOkFolderPos; //sys
	unsigned int SpamMailFolderPos; //sys
	unsigned int FirstRulePos;
	unsigned int LastRulePos;
	unsigned int FirstIpRulePos;
	unsigned int LastIpRulePos;

	

	unsigned int FolderUIdIndex; //Folder 的 key id

//	unsigned int NextAccountData;
	bool bDel;

    
} AccountData;

typedef struct _MailFolderData
{
    unsigned int FolderUId; //存自己的 FilePos
	char FolderName[50];//存 Imap Folder UTF7
	unsigned int Exists;
    unsigned int Recent;
    unsigned int FirstUnseen;
    unsigned int Seen;
    unsigned int LastMailUID;
	unsigned int FirstMailUID;    

    unsigned int MailUIdIndex;//Mail 的 key id

	unsigned int FirstMailDataPos;
	unsigned int LastMailDataPos;
	
	unsigned int SubMailFolderDataPos; //下層目錄
	unsigned int NextMailFolderDataPos;
	unsigned int PrevMailFolderDataPos;
} MailFolderData;

typedef struct _SystemData
{
    unsigned int FirstRelayInfoPos;
	unsigned int LastRelayInfoPos;
	unsigned int FirstRelayedInfoPos; //已寄出的 mail
	unsigned int LastRelayedInfoPos; //已寄出的 mail
} SystemData;


/*

typedef struct _DBIMAPMailHeader
{
   
	unsigned int From_Pos;
	unsigned int Sender_Pos;
	unsigned int Reply_To_Pos;
	unsigned int To_Pos;
	unsigned int Cc_Pos;
	unsigned int Bcc_Pos;
	unsigned int Date_Pos;
	unsigned int Subject_Pos;
	unsigned int Message_Id_Pos;



	unsigned int SeqNo;
    
} DBIMAPMailHeader;

typedef struct _IMAPMailHeader
{
   
	//需轉換成 IMAP 的特殊格式
	char* From;
	char* Sender;
	char* Reply_To;
	char* To;
	char* Cc;
	char* Bcc;

	//轉換為 IMAP String
	char* Date;
	char* Subject;
	char* Message_Id;

	
    
} IMAPMailHeader;*/


typedef struct _DBMailData
{
	//DBStr RawMailData;
	unsigned int RawMailDataPos;
	unsigned int RawMailDataLen;
	unsigned int HeaderDataPos; //指向 RawMailData 的 file header pos;
	unsigned int HeaderDataLen;

	unsigned int Status;

	//IMAP MAIL DATA struct
	//DBIMAPMailHeader IMAPHeader;
	unsigned int ImapEnvelopePos;
	unsigned int ImapEnvelopeLen;
	unsigned int ImapSeqNo;
	char ImapInternalDate[50];

	unsigned int MailUId;
	
	unsigned int NextMailDataPos;
	unsigned int PrevMailDataPos;
    
} DBMailData;

typedef struct _RelayInfo
{
	unsigned int RawMailDataPos;
	unsigned int RawMailDataLen;

	unsigned int NextRelayInfoPos;
	unsigned int PrevRelayInfoPos;

} RelayInfo;


class CServerDB  
{
private:
	
	CDB *AccountIndexDB;
	char* mAccountDBExt;

	CRITICAL_SECTION*  mDBSection;
	SystemData sysdata;

	char PGPath[_MAX_PATH];
	char DBPath[_MAX_PATH];
	char DBFile[_MAX_PATH];

	unsigned int SearchFolderData(MailFolderData *FolderData,unsigned int FolderUId);
	unsigned int SearchFolderDataByName(MailFolderData *FolderData,char* FolderName);
	unsigned int GetMailUId(unsigned int MailDataPos);
public:
	FILE *DBfp;

	CServerDB(CRITICAL_SECTION*  DBSection , char* DBFileName = NULL , char* AccountDBExt = NULL);
	virtual ~CServerDB();
	void OpenDB();
	void CloseDB();

	void FlushDB();


	

	//Account Func
	bool CheckAccountExists(char* Account);
	unsigned int AddAccount(char* Account);
	void DelAccount(char* Account);
	unsigned int GetAccountId(char* Account);
	char* GetAccount(unsigned int AccountId);
	AccountData GetAccountData(unsigned int AccountId);

	//Mail Func
	//void NewMail(char* Account,unsigned int FolderUId,char* MailFrom , AccountList* RcptList,char* MailContent );
	unsigned int NewImapMail(unsigned int AccountId,unsigned int FolderUId,unsigned int Flag ,char* InternalDate , char* ImapEnvelopeStr ,FILE *fp , unsigned int MailUId = 0 );
	
	
	unsigned int  ListMail(unsigned int AccountId , unsigned int FolderUId,unsigned int MailUId,DBMailData* MailData);		
	unsigned int  ListFromMail(unsigned int AccountId , unsigned int FolderUId,unsigned int MailUId,DBMailData* MailData);		
	unsigned int  ListNextMail(DBMailData* MailData);
	char* RetriveMail(DBMailData* MailData);
	char* RetriveMailHeader(DBMailData* MailData);
	char* RetriveImapEnvelope(DBMailData* MailData);

	void FreeMail(char* mail);

	void SetMailStatus(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId,unsigned int Status);
	void SetImapSeqNo(unsigned int MailDataPos , unsigned int SeqNo);
	unsigned int DelMail(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId);
	//void CleanDelMail(unsigned int AccountId);
	unsigned int MoveMail(unsigned int AccountId,unsigned int OldFolderUId,unsigned int OldMailUId , unsigned int NewFolderUId);//回傳 new MailUId
	



	//Folder Func	
    
	unsigned int GetAccountFolderData(unsigned int AccountId ,unsigned int FolderUId,MailFolderData *FolderData);
	
	unsigned int GetAccountFolderNameData(unsigned int AccountId ,char* FolderName,MailFolderData *FolderData);
	void GetAccountFolderDataPos(unsigned int FolderDataPos,MailFolderData *FolderData);
	




	unsigned int  GetFolderUId(char* Account  , char* FolderName);
	unsigned int GetFolderUIdId(unsigned int AccountId, char* FolderName);
	


	//Spam Func
	void FixSpamRate(unsigned int FolderUId,unsigned int MailUId , int OkRate , int BadRate); 
	int CheckPersonalIp(unsigned int AccountId,char*IP);
	double CountSpamRate(CChecker *cc ,  char* MailBuff,char* MailTextbuf,char* Sender,char* Subject,char* IP);
	void UpdateSpamRate(CChecker *cc , int OKFix, int BadFix,bool AutoUpdate );
	//Rule Func

	//RelayInfo
	unsigned int ListFirstRelayInfo(RelayInfo *rinfo);
	unsigned int ListNextRelayInfo(RelayInfo *rinfo);
	void SetRelayedInfo(RelayInfo rinfo); //已寄出的 mail queue
	void DumpMail(RelayInfo *rinfo , char* DumpBuffer , unsigned int BufferSize , unsigned int FpPos);
	char *GetDBFile();
	void RelayMail(FILE *fp);
	void RelayDBMail(unsigned int AccountId,unsigned int FolderUId,unsigned int MailUId);
	//FILE* GetMailFp(unsigned int FpPos);

};

#endif // !defined(AFX_SERVERDB_H__2BBA138F_E28C_4BA5_A84F_397DF7B24B96__INCLUDED_)
