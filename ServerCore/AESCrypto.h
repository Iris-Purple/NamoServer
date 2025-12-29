#pragma once
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

/*--------------
	AESCrypto
---------------*/

class AESCrypto
{
public:
	AESCrypto();
	~AESCrypto();

	bool	Init(const BYTE* key, int32 keyLen);
	void	Cleanup();

	// 암호화: 입력 데이터를 암호화하여 output에 저장, 반환값은 암호화된 길이 (-1: 실패)
	int32	Encrypt(const BYTE* input, int32 inputLen, BYTE* output, int32 outputSize);

	// 복호화: 입력 데이터를 복호화하여 output에 저장, 반환값은 복호화된 길이 (-1: 실패)
	int32	Decrypt(const BYTE* input, int32 inputLen, BYTE* output, int32 outputSize);

	// 암호화 후 예상 크기 계산 (PKCS7 패딩 포함)
	static int32 GetEncryptedSize(int32 plainSize);

private:
	BCRYPT_ALG_HANDLE	_hAlgorithm = nullptr;
	BCRYPT_KEY_HANDLE	_hKey = nullptr;
	BYTE				_iv[16] = {};			// 초기화 벡터
	BYTE				_ivBackup[16] = {};		// IV 원본 보관 (매 암호화/복호화 시 복원)
	PBYTE				_keyObject = nullptr;
	DWORD				_keyObjectSize = 0;
	bool				_initialized = false;
};
