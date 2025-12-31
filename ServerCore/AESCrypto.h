#pragma once
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

/*--------------
	AESCrypto
---------------*/

// HMAC-SHA256 출력 크기
constexpr int32 HMAC_SIZE = 32;

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

	// HMAC-SHA256 계산: 데이터에 대한 HMAC 생성 (outHmac은 32바이트 버퍼)
	bool	ComputeHMAC(const BYTE* data, int32 dataLen, BYTE* outHmac);

	// HMAC 검증: 데이터와 기대 HMAC 비교
	bool	VerifyHMAC(const BYTE* data, int32 dataLen, const BYTE* expectedHmac);

private:
	BCRYPT_ALG_HANDLE	_hAlgorithm = nullptr;
	BCRYPT_KEY_HANDLE	_hKey = nullptr;
	BYTE				_iv[16] = {};			// 초기화 벡터
	BYTE				_ivBackup[16] = {};		// IV 원본 보관 (매 암호화/복호화 시 복원)
	PBYTE				_keyObject = nullptr;
	DWORD				_keyObjectSize = 0;
	bool				_initialized = false;

	// HMAC 관련
	BCRYPT_ALG_HANDLE	_hHmacAlgorithm = nullptr;
	BYTE				_hmacKey[16] = {};		// HMAC 키 (AES 키와 동일하게 사용)
};
