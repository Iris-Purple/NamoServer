#pragma once

extern class ThreadManager* GThreadManager;
extern class GlobalQueue* GGlobalQueue;
extern class JobTimer* GJobTimer;
extern class JobStats* GJobStats;


// 패킷 암호화 ON/OFF (true: 암호화 사용, false: 평문)
extern bool GEncryptionEnabled;

// 암호화 키 (AES-128: 16바이트) 
extern BYTE GEncryptionKey[16];