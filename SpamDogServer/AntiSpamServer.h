// AntiSpamServer.h: interface for the CAntiSpamServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANTISPAMSERVER_H__D3A49143_C9A4_4504_AF9F_5DFF3A2F92AD__INCLUDED_)
#define AFX_ANTISPAMSERVER_H__D3A49143_C9A4_4504_AF9F_5DFF3A2F92AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <objbase.h>
#include <activeds.h>
#include <conio.h>
#include <Tchar.h>

#include "../../SpamDog/Swfiledb/DB.h"
#include "../Swsocket2/CoreClass.h"
#include "../Swsocket2/HTTPClient.h"

#include "MsnClient.h"

#include "ManageHttpServer.h"
#include "IMAPServer.h"
#include "ServerDB.h"




typedef struct _AntiSpamServerData
{

   time_t LastConnectTime;   
   int OkRate;
   int BadRate;
   bool Blocked; //保留
   time_t LastBlockTime; //保留
   int Reserve;//保留空間   

} AntiSpamServerData;


class  CAntiSpamServerDB 
{
private:
	CDB *IpDB;
	CRITICAL_SECTION*  mDBSection;

public:
	CAntiSpamServerDB(CRITICAL_SECTION*  DBSection);
	virtual ~CAntiSpamServerDB();
	char PGPath[MAX_PATH];

	void RenewDB();
	time_t LastConnectTime(char* IP);
	void OpenIpDB();
	void CloseIpDB();
	bool CheckBadIpExists(char* IP);
	bool CheckOkIpExists(char* IP);
	void UpdateIp(char* IP , int OkRate , int BadRate);

};

/*

Server Setting 說明 :



ServerMode 說明:

//RBLOnConnect[OnConnect]: 0 : 不檢查 (在 AferAccept 檢查), 1 : 檢查後如為 spam 斷線(no log) , 2: 檢查後如為 spam 斷線寫入 admin 的 log
//RCPTSpamConnection: 0 : 不檢查 , 1: 檢查後如為 spam 斷線寫入 personal log , 2 : 檢查後如為 spam 繼續收信 => Check RBL on AferAccept
//RBLRcptSpamConnection[OnAfterAccept] : 0 : 不檢查 , 1 : 檢查後如為 spam 斷線(no log) , 2: 檢查後如為 spam 斷線寫入 personal log
//CheckPersonalRate[DATA Finished] : 0  : 不檢查 , 1 : 檢查後如為 spam 斷線(no log) , 2: 檢查後如為 spam 斷線寫入 personal log , 3: 檢查後如為 spam 放入 spam mail folder
//CheckPersonalIp[RCPT] : 0  : 不檢查 , 1 : 檢查後如為 spam 斷線(no log) , 2: 檢查後如為 spam 斷線寫入 log , 3: 檢查後如為 spam 繼續收信




H (沒 imap 的信件記錄): 
  OnConnect : check RBL 當沒過就直接斷線
  Rcpt to : 沒人直接斷線
  Rcpt CheckPersonalIP : 被拒決直接斷線
  Data CheckPersonalRate : 被拒決直接斷線
  Delay Send : 被拒決直接斷線


S:
  Rcpt to : 沒人直接斷線
  Rcpt to : 有人但如在 rbl 則放到 Ip history , 斷線
  Rcpt CheckPersonalIP : 被拒決直接斷線
  Data CheckPersonalRate : 被拒決放到 Spam mail box 斷線,
  Delay Send : 被拒決則放到 Ip history , 斷線

L: (全部收)
  Rcpt to : 沒人直接斷線
  Rcpt to : 有人但如在 rbl 則放到 Spam mail box
  Rcpt CheckPersonalIP : 被拒決直接斷線
  Data CheckPersonalRate : 被拒決放到 Spam mail box 斷線,
  Delay Send : 不起動 

*/
struct APsetup
{
	 int OkDelayResponse;
	 int SpamDelayResponse;
	 int LimitRetryTime;
	 char ServerMode[2]; //S:Standard, L: Low Safe , H:high level
	 char RBL1[255];
	 char RBL2[255];
	 char ServerName[255]; //softworking.com
	 char RelayToServerIp[16];
	 char SpamDogCenterIp[16];
	 char DnsIp[16];
	 char ADServerIp[16];
	 char ADLoginId[125];
	 char ADPassWord[40];
	 long LastRebuildDBTime;
	 unsigned int MailNotifyNum;
	 char AdminEmail[255];
	 char RcptCheck;
	 char ImapAuthType;
	 char POP3Ip[16];
	 unsigned int MaxNoSpamSize; //超過此大小為 No Spam
	 unsigned int AccountCheckMode; //AD : 0 , FILE : 1
	 char RelayPopMode ;
	// unsigned int MailUID;
	 
};	 

typedef struct _AccountList
{
	char Account[120]; //目前 Account = Email 
	char EMail[120];
	//char MailBoxAccount[120];
	//char PassWord[50];
	unsigned int DBAccountId;
	int PersonalAcceptType;
	bool SpamMail;
	struct _AccountList  *prev,*next; 

} AccountList;

typedef struct _AllAccountList
{
  AccountList *AList;  
  int AccCount;
  struct _AllAccountList  *prev,*next; 
} AllAccountList;


typedef struct _MailListHeader
{
   unsigned int MailCount;
   time_t LastUsedTime;
} MailListHeader;

typedef struct _MailListData
{
   _TCHAR MailFrom[120];   
   char Ip[16];   
   time_t ReceiveTime;
   char Status; // 1: rbl , 2: delay send disconnect (rbl) , 3: delay send disconnect (no rbl)
   bool SendNotify;
   char MailFileName[MAX_PATH]; //不含目錄   
} MailListData;



typedef struct _SMTPData
{
   bool SpamConnection;
   //bool PersonalPass;
   FILE *fp;
   unsigned int MailSize;
   //FILE *DataFp;
   _TCHAR RcptTo[120];
   _TCHAR MailFrom[120];     
   time_t ReceiveTime;
   //char Account[50];  
   _TCHAR Domain[50];
   char TempFileName[MAX_PATH];
   time_t DelayResponseTime;
   char DelayCmdTxt[255];
   char PTR[255];
   unsigned char DelayCmd;
   //MailListData MailList;
   AccountList *AccList;
   int AccCount;
   //CRITICAL_SECTION     SMTPCritSec;
} SMTPData;

/*
typedef struct _IPLIST
{
   char Ip[16];
   char Status; // 1 : ok , 2 : bad 
} IpList;
*/

typedef struct _IPLIST
{
   char Ip[16];
   char Status; // 1 : ok , 2 : bad 
   struct _IPLIST  *prev,*next; 
} IpList;

typedef struct _IPZone
{
	char Zone1[4];
	char Zone2[4];
	char Zone3[4];
	char Zone4[4];
	
} IPZone;



class CAntiSpamServer : public CBaseServer  
{
public:
	
	CAntiSpamServer();
	 ~CAntiSpamServer();

	CRITICAL_SECTION  mDBSection,mMailDBSection;

	bool Running;
	CAntiSpamServerDB *ServerDB;
	CServerDB  *MailServerDB;
	AllAccountList *AllAccount;

	APsetup Settings;

	HANDLE RelayMailEvent;
	HANDLE RelayMailThreadHandle;
	HANDLE ServerProcThreadHandle;

	#define SM_HELO       1
	#define SM_MAIL       2
	#define SM_RCPT       3
	#define SM_DATA       4
	#define SM_QUIT       5

	static bool CheckADExist(char* AD_IP,char* Email,char* UserId,char* Password);
 
	void GetADAccList(char* AD_IP,char* Email,char* UserId,char* Password,SMTPData *userdata);	
	static void GetAccountFromLogin(char* AD_IP,char* LoginId,char* UserId,char* Password,char* Account,int BufferSize);
	static bool VerifyAccount(char* AD_IP, char* Account,char* Password);
    

private:
	
	long SocketCount ; 
	IpList *BlockIpList;

	CMsnClient *LoginClient;

	CDnsClient *dns;
	CHTTPClient http;
	CManageHttpServer *mHttpServer;
	CIMAPServer *mImapServer;

	FILE *LogFp;
	char LogName[255];
	
	//char PGPath[MAX_PATH];
	

	void IniServer();
	void IniLogFile();

	int CheckRBL(char *IP);

	#define Personal_Accept       1
	#define Personal_Deny       2
	#define Personal_None	3

	int CheckPersonalIP(unsigned int AccountId , char *IP);


	void ProcessMailList(SOCKET_OBJ* sock);
	static void GetCN(char* AdsPath);
	void GetMailFromCN(IDirectorySearch *pSearch,char *CN , char* Mail , int BufferSize);
	//void GetAccount(_TCHAR* Email);
	//void InsertAccountListFile(AccountList **AList,char* Email,char *MailBoxAccount,char *PassWord);
	void InsertAccountList(AccountList **AList,char* Email);
	//void InsertAccountListFile(SMTPData *userdata,char* Email,char *MailBoxAccount,char *PassWord);
	void DestoryAccountList(SMTPData *userdata);
	void DestoryAccountList(AccountList *AList);
	void DeleteAccountList(SMTPData *userdata,char* Account);
	AllAccountList* InsertAllAccountList(char *Email);
	void InsertAccountList(SMTPData *userdata,char* Email);
	void DestoryAllAccountList();
	void RetriveAccountList(SMTPData *userdata,char* Email);
	bool CheckFromIp(_TCHAR* EmailDomain , char* RemoteIp,char* PTR);
	void GetIPZone(char* Ip , IPZone *AIpZone);
	void GetDomainIdent(char* URL , char* DomainIdent);
	void ReadAccountFile();
	void WriteLog(char *LogStr);
	void ToLowCase(char *AStr);
	void SendMSG(char *Msg);
	void ReadBlockIpList();
	bool CheckBlockIp(char *Ip);
    void SendErrMail(char* ErrorStr);
	
//	int GetMailUID();

protected:
	virtual int OnAfterAccept(SOCKET_OBJ* sock); //連線後可以送的第一個 command
	virtual int OnConnect(sockaddr* RemoteIp); //在 配至資源 之前 , 可拒絕 client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	virtual void HandleSocketMgr(SOCKET_OBJ *sock);

};

unsigned __stdcall RelayMailThread(LPVOID lpParam);
unsigned __stdcall ServerProcThread(LPVOID lpParam);
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);


#endif // !defined(AFX_ANTISPAMSERVER_H__D3A49143_C9A4_4504_AF9F_5DFF3A2F92AD__INCLUDED_)
