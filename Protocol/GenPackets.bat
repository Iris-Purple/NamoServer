pushd %~dp0

:: c++ 빌드된 *.cc, *.h  기존 파일 삭제
DEL /Q /F "Protocol\cpp_output\*.*"

:: c++ 신규빌드 파일 생성
protoc.exe -I=./Protocol --cpp_out=./Protocol/cpp_output ./Protocol/*.proto
:: c#  신규빌드 파일 생성
protoc.exe -I=./Protocol --csharp_out=./Protocol/csharp_output ./Protocol/*.proto


PUSHD PacketGenerator 
	python PacketGenerator.py --path=../Protocol/Protocol.proto --output=ClientPacketHandler --recv=S2C_ --send=C2S_
	python PacketGenerator.py --path=../Protocol/Protocol.proto --output=ServerPacketHandler --recv=C2S_ --send=S2C_
POPD

IF ERRORLEVEL 1 PAUSE

:: c++ 변환된 프로토콜 게임서버에 복사
DEL /Q /F "..\GameServer\Protocol\*.*"
XCOPY /Y "Protocol\cpp_output\*.*" "..\GameServer\Protocol\"
:: 패킷핸들러 게임서버 복사
XCOPY /Y "PacketGenerator\ServerPacketHandler.h" "..\GameServer\"

:: c++ 변환된 프로토콜 더미클라이언트에 복사
DEL /Q /F "..\DummyClient\Protocol\*.*"
XCOPY /Y "Protocol\cpp_output\*.*" "..\DummyClient\Protocol\"
:: 패킷핸들러 더미클라이언트 복사
XCOPY /Y "PacketGenerator\ClientPacketHandler.h" "..\DummyClient\"

:: c# 변환된 프로토콜 클라이언트에 복사 
XCOPY /Y "Protocol\csharp_output\*.*" "..\..\Client\Assets\Scripts\Packet"


:: protobuf 빌드에 필요한 소스코드 복사
IF NOT EXIST "..\Libraries\Include\google" XCOPY /Y /S /E /I "Protocol\google" "..\Libraries\Include\google"

:: 라이브러리 Debug 복사 
IF NOT EXIST "..\Libraries\Libs\Protobuf\Debug\libprotobufd.lib" COPY /Y "Protocol\Lib\Debug\libprotobufd.lib" "..\Libraries\Libs\Protobuf\Debug\libprotobufd.lib"
IF NOT EXIST "..\Libraries\Libs\Protobuf\Debug\libprotobufd.pdb" COPY /Y "Protocol\Lib\Debug\libprotobufd.pdb" "..\Libraries\Libs\Protobuf\Debug\libprotobufd.pdb"

:: 라이브러리 Release 복사
IF NOT EXIST "..\Libraries\Libs\Protobuf\Release\libprotobuf.lib" COPY /Y "Protocol\Lib\Release\libprotobuf.lib" "..\Libraries\Libs\Protobuf\Release\libprotobuf.lib"


:: StatData.json 복사
XCOPY /Y "Data\*.*" "..\Common\Data\"
XCOPY /Y "Data\*.*" "..\..\Client\Assets\Resources\Data\"

PAUSE