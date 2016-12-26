// IMAPServer.h: interface for the CIMAPServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAPSERVER_H__E993D4BD_16DD_44A2_80A7_1493D3CFC48A__INCLUDED_)
#define AFX_IMAPSERVER_H__E993D4BD_16DD_44A2_80A7_1493D3CFC48A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Swsocket2/CoreClass.h"
#include "../../SpamDog/Swparser/MailParser.h"
#include "ServerDB.h"
#include <Tchar.h>


#define Flag_Seen	1
#define Flag_Answered	2
#define Flag_Flagged	4
#define Flag_Deleted	8
#define Flag_Draft	16
#define Flag_Recent	32


typedef struct _ImapQueSend
{
    char* cmd;
    struct _ImapQueSend  *prev,*next; 	

} ImapQueSend;


typedef struct _ImapMailFp
{
	FILE *EmlFp;
	FILE *EhdFp;
	FILE *DatFp;
	FILE *ImpFp;

} ImapMailFp;

typedef struct _IMAPData
{

    //char SaveData[255];
	int SaveCount;
	char USER[255];
	char SelectedBoxName[255];
	unsigned int DBAccountId;
	unsigned int DBSelectedFolderId;
	bool DelSpamFolder;
	char IDLEcmd[5];
	//bool StatusChanged;
	//bool IDLEcmd;
	//char tmpcmd[6];
	
	//char SendDataFilePath[MAX_PATH];
	//FILE *Sendfp;
	//char* SendDataEndCmd;

	char RecvDataFilePath[MAX_PATH];
	FILE *Recvfp;
	char* RecvDataEndCmd;
	ImapQueSend *que;

	char InternalDate[50];
	unsigned int Status;


} IMAPData;

typedef struct _ParseData
{
    char* Data[20480];	//Data Pointer Array
	unsigned int DataCount;
} ParseData;


typedef struct _ImapAddr
{
  char PersonalName[100];
  char AtDomainList[100];
  char MailBoxName[100];
  char HostName[100];

} ImapAddr;

typedef struct _ImapMailData
{
   
   unsigned int Status; //SEEN DELETE
   unsigned int MailUID;
   unsigned int SeqNo;
   char RemoteIp[16];
   char InternalDate[50];
   
   

  // MailHeader mailHeader;
   
  /* char Date[25];
   char Subject[255];
   ImapAddr From;
   ImapAddr Sender;
   ImapAddr ReplyTo;
   ImapAddr To;
   ImapAddr Cc;
   ImapAddr Bcc;
   char InReplyTo[100];
   char MessageId[100];*/

   //date, subject, from, sender, reply-to, to, cc, bcc, in-reply-to, and message-id

   /*The fields of an address structure
         are in the following order: personal name, [SMTP]
         at-domain-list (source route), mailbox name, and host name*/

} ImapMailData;


typedef struct _ImapUserList
{
    char USER[255];	
	unsigned int DBAccountId;
    struct _ImapUserList  *prev,*next; 
	SOCKET_OBJ* sock;

} ImapUserList;

typedef struct _ImapStr
{
    char* str;
    unsigned int size;

} ImapStr;


//Box Info Data

typedef struct _BoxInfo
{
    unsigned int Exists;
    unsigned int Recent;
	unsigned int FirstUnseen;
	unsigned int Seen;
	unsigned int LastMsgUID;
	unsigned int FirstMsgUID;
	unsigned int UID;	

} BoxInfo;

typedef struct _IMAPAccountList
{
	char Account[120];
	char EMail[120];
	char MailBoxAccount[120];
	char PassWord[50];	
	struct _IMAPAccountList  *prev,*next; 

} IMAPAccountList;


class CIMAPServer : public CBaseServer  
{
public:
	CIMAPServer(CRITICAL_SECTION*  mDBSection);
	 ~CIMAPServer();

	char AD_IP[16]; 
	char ADLoginId[125];
	char ADPassWord[40];
	unsigned int MaxNoSpamSize; 
	char SpamDogCenterIp[16];

	//void AddImapSpamMail( char* User , char* MailFileName , char* RemoteIp);
	//void AddImapHistoryIpMail(char* User , char* Domain , char* RemoteIp,char* From , char* Subject,char* Desc);


	void CreateBoxInfo(char* BoxPath);
	void AddString(char* StrBuffer , char* InputStr);
	void ToImapString(char* InputStr);
	//void ConvertToImapStr(char* InputStr);
	void AddVarString(char* AppendStr , ImapStr* OldStr);
	void AddVarImapString(char* AppendStr , ImapStr* OldStr);
	char* GetEnvelopeStr(FILE *MailFp);
	char* GetInternalDate();
	bool SendStatus(unsigned int AccountId , unsigned int FolderId);
	void SetRelayMailEvent(HANDLE RelayMailEvent);
	 

	#define IM_NORMAL       1
	#define IM_DATA       2

protected:
	virtual int OnAfterAccept(SOCKET_OBJ* sock); //連線後可以送的第一個 command
	virtual int OnConnect(sockaddr* RemoteIp); //在 配至資源 之前 , 可拒絕 client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );	
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	virtual void OnDisconnect(int SockHandle);
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	virtual void HandleSocketMgr(SOCKET_OBJ *sock);

private:
    CRITICAL_SECTION     MailUIDSection , MailListSection , *DBSection; 

	CServerDB  *MailServerDB;
	HANDLE mRelayMailEvent;
	IMAPAccountList *m_AccountList;
	int AccountCheckMode;

	char PGPath[MAX_PATH];
	void CleanDelMail(IMAPData *userdata);
	void EXPUNGEMail(SOCKET_OBJ* sock);

	void TrimQuote(char *AStr);
	void TrimParenthesis(char *AStr);
	ImapUserList *m_ImapUserList ;

	void RFCToEnvStruct(char* RFCbuf,ImapStr* istr);
	void AddImapAppendMail(IMAPData *imapdata,char* Flag ,char* InternalDate , char* MailBoxName );
	 
	void NilMailData(ImapMailData *mdata);
	void ToParseData(char *AStr , ParseData* aPData);
	void ToParseSeqData(char *AStr , ParseData* aPData);
	void ToParseMailListData(char *AStr , ParseData* aPData);
		
   	
    bool ExecuteCmd(char* Cmd ,char* MailList ,char* ImapQuery , unsigned int UserId, unsigned int FolderId , SOCKET_OBJ* sock );
	bool FetchMail(unsigned int FromMailUId , unsigned int ToMailUId ,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock );	
	bool SearchMail(unsigned int FromMailUId , unsigned int ToMailUId ,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock );	
	bool StoreMail(unsigned int FromMailUId,unsigned int ToMailUId , char* StoreType,char* ImapQuery , unsigned int UserId, unsigned int FolderId, SOCKET_OBJ* sock );
	bool CopyMail(unsigned int FromMailUId,unsigned int ToMailUId  ,unsigned int UserId, unsigned int FromFolderId , unsigned int ToFolderId , SOCKET_OBJ* sock );
	

	ImapUserList* SearchUserList(char* User);
	ImapUserList* SearchAccountIdList(unsigned int AccountId);

	void InsertUserList(SOCKET_OBJ* sock , char* User);
	void RemoveUserList(char* User);
	
	void InsertQueSend(char* AStr , ImapQueSend **qSend);
	void RemoveQueSend(ImapQueSend *obj , ImapQueSend **qSend);
	
	
	BoxInfo SelectBoxInfo(char* User , char* MailBoxName);
	

	BoxInfo CacuBoxInfo(char* User , char* MailBoxName);
	void ReAssignBoxSeq(unsigned int UserId , unsigned int FolderId );

	ImapMailData SelectMailData(unsigned int MailUID ,char* User , char* MailBoxName);
	void SelectMailDataFile(ImapMailData *md,char* MailFilePath ,char* User , char* MailBoxName);

	void UpdateBoxInfo(BoxInfo *bi,char* User , char* MailBoxName);
	void UpdateMailData(ImapMailData *im,char* User , char* MailBoxName);

	void MailStatusToStr(unsigned int MailStatus , ImapStr *Astr);

	unsigned int GetMailUID();
	unsigned int GetBoxUID();

	

	void ListDBAccountFolder(MailFolderData *FolderData,SOCKET_OBJ* sock ,const char* Cmd, unsigned int AccountId);
	void SendIDLEResult(SOCKET_OBJ* sock);
	bool VerifyAccountFile(char* BoxAccount,char* Password,char* Account);

	void InsertAccountList(char* Email,char *MailBoxAccount,char *PassWord);
	void DestoryAccountList();
	//void GetAccount(_TCHAR* Email);
	void ReadAccountFile();
	


};

#endif // !defined(AFX_IMAPSERVER_H__E993D4BD_16DD_44A2_80A7_1493D3CFC48A__INCLUDED_)
