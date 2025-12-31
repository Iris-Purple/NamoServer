#include "pch.h"
#include "AESCrypto.h"

/*--------------
	AESCrypto
---------------*/

AESCrypto::AESCrypto()
{
}

AESCrypto::~AESCrypto()
{
	Cleanup();
}

bool AESCrypto::Init(const BYTE* key, int32 keyLen)
{
	if (_initialized)
		return true;

	if (keyLen != 16)  // AES-128은 16바이트 키 필요
		return false;

	// AES 알고리즘 열기
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (status != 0)
		return false;

	// CBC 모드 설정
	status = BCryptSetProperty(_hAlgorithm, BCRYPT_CHAINING_MODE,
		(PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
	if (status != 0)
	{
		Cleanup();
		return false;
	}

	// 키 오브젝트 크기 조회
	DWORD dataSize = 0;
	status = BCryptGetProperty(_hAlgorithm, BCRYPT_OBJECT_LENGTH,
		(PBYTE)&_keyObjectSize, sizeof(DWORD), &dataSize, 0);
	if (status != 0)
	{
		Cleanup();
		return false;
	}

	_keyObject = new BYTE[_keyObjectSize];

	// 대칭 키 생성
	status = BCryptGenerateSymmetricKey(_hAlgorithm, &_hKey, _keyObject,
		_keyObjectSize, (PBYTE)key, keyLen, 0);
	if (status != 0)
	{
		Cleanup();
		return false;
	}

	// IV 설정 (키를 IV로 사용 - 서버/클라이언트 동일하게 설정 필요)
	memcpy(_ivBackup, key, 16);

	// HMAC 키 저장 (AES 키와 동일하게 사용)
	memcpy(_hmacKey, key, 16);

	// HMAC-SHA256 알고리즘 열기
	status = BCryptOpenAlgorithmProvider(
		&_hHmacAlgorithm,
		BCRYPT_SHA256_ALGORITHM,
		nullptr,
		BCRYPT_ALG_HANDLE_HMAC_FLAG
	);
	if (status != 0)
	{
		Cleanup();
		return false;
	}

	_initialized = true;
	return true;
}

void AESCrypto::Cleanup()
{
	if (_hKey)
	{
		BCryptDestroyKey(_hKey);
		_hKey = nullptr;
	}
	if (_hAlgorithm)
	{
		BCryptCloseAlgorithmProvider(_hAlgorithm, 0);
		_hAlgorithm = nullptr;
	}
	if (_keyObject)
	{
		delete[] _keyObject;
		_keyObject = nullptr;
	}
	if (_hHmacAlgorithm)
	{
		BCryptCloseAlgorithmProvider(_hHmacAlgorithm, 0);
		_hHmacAlgorithm = nullptr;
	}
	_initialized = false;
}

int32 AESCrypto::Encrypt(const BYTE* input, int32 inputLen, BYTE* output, int32 outputSize)
{
	if (!_initialized || inputLen <= 0)
		return -1;

	// IV 복원 (매 암호화마다 동일한 IV 사용)
	memcpy(_iv, _ivBackup, 16);

	// 출력 크기 계산 (PKCS7 패딩 포함)
	DWORD paddedLen = 0;
	NTSTATUS status = BCryptEncrypt(_hKey, (PBYTE)input, inputLen, nullptr, _iv, 16,
		nullptr, 0, &paddedLen, BCRYPT_BLOCK_PADDING);
	if (status != 0)
		return -1;

	if ((int32)paddedLen > outputSize)
		return -1;

	// IV 다시 복원 (BCryptEncrypt가 IV를 변경하므로)
	memcpy(_iv, _ivBackup, 16);

	// 실제 암호화
	DWORD resultLen = 0;
	status = BCryptEncrypt(_hKey, (PBYTE)input, inputLen, nullptr, _iv, 16,
		output, outputSize, &resultLen, BCRYPT_BLOCK_PADDING);
	if (status != 0)
		return -1;

	return (int32)resultLen;
}

int32 AESCrypto::Decrypt(const BYTE* input, int32 inputLen, BYTE* output, int32 outputSize)
{
	if (!_initialized || inputLen <= 0)
		return -1;

	// AES 블록 크기(16)의 배수가 아니면 실패
	if (inputLen % 16 != 0)
		return -1;

	// IV 복원
	memcpy(_iv, _ivBackup, 16);

	// 복호화
	DWORD resultLen = 0;
	NTSTATUS status = BCryptDecrypt(_hKey, (PBYTE)input, inputLen, nullptr, _iv, 16,
		output, outputSize, &resultLen, BCRYPT_BLOCK_PADDING);
	if (status != 0)
		return -1;

	return (int32)resultLen;
}

int32 AESCrypto::GetEncryptedSize(int32 plainSize)
{
	// PKCS7 패딩: 16바이트 블록 단위로 올림
	// 정확히 16의 배수인 경우에도 16바이트 패딩 추가
	return ((plainSize / 16) + 1) * 16;
}

bool AESCrypto::ComputeHMAC(const BYTE* data, int32 dataLen, BYTE* outHmac)
{
	if (!_initialized || data == nullptr || dataLen <= 0 || outHmac == nullptr)
		return false;

	// HMAC 해시 객체 크기 조회
	DWORD hashObjectSize = 0;
	DWORD dataSize = 0;
	NTSTATUS status = BCryptGetProperty(
		_hHmacAlgorithm,
		BCRYPT_OBJECT_LENGTH,
		(PBYTE)&hashObjectSize,
		sizeof(DWORD),
		&dataSize,
		0
	);
	if (status != 0)
		return false;

	// 해시 객체 메모리 할당
	PBYTE hashObject = new BYTE[hashObjectSize];

	// HMAC 해시 핸들 생성
	BCRYPT_HASH_HANDLE hHash = nullptr;
	status = BCryptCreateHash(
		_hHmacAlgorithm,
		&hHash,
		hashObject,
		hashObjectSize,
		(PBYTE)_hmacKey,
		16,
		0
	);
	if (status != 0)
	{
		delete[] hashObject;
		return false;
	}

	// 데이터 해시
	status = BCryptHashData(hHash, (PBYTE)data, dataLen, 0);
	if (status != 0)
	{
		BCryptDestroyHash(hHash);
		delete[] hashObject;
		return false;
	}

	// HMAC 결과 추출 (32바이트)
	status = BCryptFinishHash(hHash, outHmac, HMAC_SIZE, 0);

	BCryptDestroyHash(hHash);
	delete[] hashObject;

	return (status == 0);
}

bool AESCrypto::VerifyHMAC(const BYTE* data, int32 dataLen, const BYTE* expectedHmac)
{
	if (expectedHmac == nullptr)
		return false;

	BYTE calculatedHmac[HMAC_SIZE];
	if (!ComputeHMAC(data, dataLen, calculatedHmac))
		return false;

	// 상수 시간 비교 (타이밍 공격 방지)
	int32 diff = 0;
	for (int32 i = 0; i < HMAC_SIZE; i++)
	{
		diff |= (calculatedHmac[i] ^ expectedHmac[i]);
	}

	return (diff == 0);
}
