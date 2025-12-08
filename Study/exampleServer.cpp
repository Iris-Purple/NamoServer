#include "pch.h"
#include "exampleServer.h"
#include "SocketUtils.h"

void exampleServer::Do()
{
	SOCKET socket = SocketUtils::CreateSocket();
	
	SocketUtils::BindAnyAddress(socket, 7777);

	SocketUtils::Listen(socket);

	SOCKET clientSocket = ::accept(socket, nullptr, nullptr);

	cout << "Client Connected!" << endl;
}