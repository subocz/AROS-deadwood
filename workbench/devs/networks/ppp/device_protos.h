

struct PPP_DevUnit *InitPPPUnit(LIBBASETYPEPTR LIBBASE,ULONG s2unit);
VOID ExpungeUnit(LIBBASETYPEPTR LIBBASE);
BOOL ReadConfig(LIBBASETYPEPTR LIBBASE);

VOID PerformIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID TermIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);

ULONG AbortReq(LIBBASETYPEPTR LIBBASE,struct MinList *minlist,struct IOSana2Req *ios2);
VOID ConfigInterface(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID GetStationAddress(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID DeviceQuery(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID WritePacket(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
				 
VOID SendPacket( LIBBASETYPEPTR LIBBASE ,struct IOSana2Req *ios2 );
VOID ReadPacket(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
//VOID ReadOrphan(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
				
				

VOID DeinitSerial(LIBBASETYPEPTR LIBBASE);
VOID OnEvent(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
			 
BOOL OpenSerial(LIBBASETYPEPTR LIBBASE);

VOID MarkTimeOnline(LIBBASETYPEPTR LIBBASE);

VOID CloseSerial(LIBBASETYPEPTR LIBBASE);

VOID Online(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID Offline(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2);
VOID DoEvent(LIBBASETYPEPTR LIBBASE,
             ULONG event);
VOID DoOffline(LIBBASETYPEPTR LIBBASE);
VOID CMD_WRITE_Ready(LIBBASETYPEPTR LIBBASE);
VOID CMD_READ_Ready(LIBBASETYPEPTR LIBBASE,struct IOExtSer *ioSer);
VOID Incoming_IP_Packet(LIBBASETYPEPTR LIBBASE, BYTE *p , ULONG length);
VOID QueueSerRequest(LIBBASETYPEPTR LIBBASE);

BOOL SendStr(LIBBASETYPEPTR LIBBASE,const STRPTR str ,LONG timeout);
void DoBYTE(LIBBASETYPEPTR LIBBASE,const BYTE b);
void DoBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p,ULONG len);
void SendBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p,ULONG len);
VOID DoStr(LIBBASETYPEPTR LIBBASE,const STRPTR str);
void SerDelay(LIBBASETYPEPTR LIBBASE, LONG timeout);

BYTE ReadBYTE(LIBBASETYPEPTR LIBBASE);

BOOL WaitStr(LIBBASETYPEPTR LIBBASE,const STRPTR str, LONG timeout);
void SetTimer(LIBBASETYPEPTR LIBBASE,const ULONG t);

void init_ppp(LIBBASETYPEPTR LIBBASE);
void byte_received(UBYTE c);
void send_IP_packet( BYTE *ptr ,ULONG len );
void SendTerminateReq();
void RunRoute();
