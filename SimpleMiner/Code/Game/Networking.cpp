#include "Networking.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <WinSock2.h>
#include <Ws2Tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


WSADATA NET_WINSOCK_INFO = {};
bool NET_WINSOCK_INITIALIZED = false;
constexpr int NET_READ_BUFFER_SIZE = 8192;
constexpr int NET_WRITE_BUFFER_SIZE = 8192;

NetErrResult StartupNetworking()
{
	NetErrResult result;

	if (NET_WINSOCK_INITIALIZED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Already running!";
		return result;
	}


	result.m_errCode = WSAStartup(MAKEWORD(2, 2), &NET_WINSOCK_INFO);
	if (result.m_errCode != 0)
	{
		result.m_errMsg = "WSAStartup failed";
		return result;
	}

	result.m_errMsg = "success!";
	NET_WINSOCK_INITIALIZED = true;
	return result;
}


NetErrResult StopNetworking()
{
	NetErrResult result;

	if (!NET_WINSOCK_INITIALIZED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Not running!";
		return result;
	}

	WSACleanup();
	NET_WINSOCK_INFO = {};
	NET_WINSOCK_INITIALIZED = false;
	return result;
}

NetErrResult QueryState(SOCKET socket, bool write, bool& state)
{
	NetErrResult result;
	state = false;

	fd_set fd_sets = {};
	fd_sets.fd_count = 1;
	fd_sets.fd_array[0] = socket;
	timeval time = { 0L, 2000L };

	int selCount = select(0, write ? nullptr : &fd_sets, write ? &fd_sets : nullptr, nullptr, &time);
	if (selCount == SOCKET_ERROR) // error
	{
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = "select failed";
	}

	if (selCount != 1 && selCount != 0)
	{
		result.m_errCode = -1;
		result.m_errMsg = Stringf("unknown error(%d)", selCount);
	}

	state = selCount != 0;
	return result;
}

struct AddrInfoPtr
{
	~AddrInfoPtr()
	{
		if (pAddrInfo)
			freeaddrinfo(pAddrInfo);
	}

	addrinfo* pAddrInfo = nullptr;
};

NetErrResult NetworkManagerClient::CreateClient(const char* host, const char* port, int retryTMillis, int maxRetry)
{
	NetErrResult result;
	if (m_running)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Already connected!";
		return result;
	}

	std::string strHost = host;
	std::string strPort = port;

	m_thread = new std::thread([this, strHost, strPort, retryTMillis, maxRetry]() { RunNetworkThread(strHost.c_str(), strPort.c_str(), retryTMillis, maxRetry); });
	return result;
}

void NetworkManagerClient::ReleaseClient()
{
	if (!m_running)
		return;

	m_disconnect = 1;
	m_thread->join();
	delete m_thread;
	m_thread = nullptr;
}

void NetworkManagerClient::NetworkTickClient()
{
	Packet packet;
	while (m_bufferRecv.ReadPacket(packet))
		HandlePacket(packet);

	for (const auto& pkt : m_queue)
		m_bufferSend.WritePacket(pkt);
	m_queue.clear();
}

void NetworkManagerClient::SendToServer(Packet& pkt)
{
	m_queue.push_back(pkt);
}

void NetworkManagerClient::HandlePacket(Packet& pkt)
{
	auto ite = m_handlers.find(pkt.m_type);
	if (ite != m_handlers.end())
		(*ite).second(pkt);
}

void NetworkManagerClient::RegisterHandler(int type, PacketHandler handler)
{
	m_handlers[type] = handler;
}

const char* GetNameFromType(CONNECTION_STATE type)
{
	static const char* const names[4] = { "DISCONNECTED", "ESTABLISHING", "CONNECTED", "LISTENING" };
	return names[(unsigned int)type];
}

CONNECTION_STATE GetTypeByName(const char* name, CONNECTION_STATE defaultType)
{
	static const CONNECTION_STATE types[4] = { CONNECTION_STATE::CONNECTED, CONNECTION_STATE::ESTABLISHING, CONNECTION_STATE::CONNECTED, CONNECTION_STATE::LISTENING };
	for (CONNECTION_STATE type : types)
	{
		if (_stricmp(GetNameFromType(type), name) == 0)
			return type;
	}
	return defaultType;
}

void NetworkManagerClient::RunNetworkThread(const char* host, const char* port, int retryTimeMillis, int maxRetry)
{
	m_running = true;

	NetErrResult result;

	maxRetry++;
	while (maxRetry > 0 && m_disconnect == 0)
	{
		DebuggerPrintf(Stringf("[NETWORK] Connecting to %s:%s...\n", host, port).c_str());

		{
			const std::lock_guard<std::recursive_mutex> lockSend(m_bufferSend.m_lock);
			const std::lock_guard<std::recursive_mutex> lockRecv(m_bufferRecv.m_lock);
			m_bufferSend.m_data.clear();
			m_bufferRecv.m_data.clear();
		}

		Connect(host, port, true);

		if (m_state == CONNECTION_STATE::ESTABLISHING)
			DebuggerPrintf(Stringf("[NETWORK] Establishing connection...\n").c_str());
		while (m_state == CONNECTION_STATE::ESTABLISHING)
		{
			ValidateConnection();

			if (m_disconnect)
			{
				m_disconnect = 0;
				CloseConnection();
				DebuggerPrintf(Stringf("[NETWORK] Disconnected!\n").c_str());
				goto OUTTER_LOOP_BREAK;
			}
			std::this_thread::yield();
		}

		if (m_state == CONNECTION_STATE::CONNECTED)
			DebuggerPrintf(Stringf("[NETWORK] Connected!\n").c_str());

		while (m_state == CONNECTION_STATE::CONNECTED)
		{
			ClientSendData();
			ClientRecvData();

			if (m_disconnect)
			{
				m_disconnect = 0;
				CloseConnection();
				DebuggerPrintf(Stringf("[NETWORK] Disconnected!\n").c_str());
				goto OUTTER_LOOP_BREAK;
			}
			std::this_thread::yield();
		}


		if (m_disconnect)
		{
			m_disconnect = 0;
			CloseConnection();
			DebuggerPrintf(Stringf("[NETWORK] Disconnected!\n").c_str());
			goto OUTTER_LOOP_BREAK;
		}
		DebuggerPrintf(Stringf("[NETWORK] Server disconnected!\n").c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(retryTimeMillis));
		maxRetry--;
		DebuggerPrintf(Stringf("[NETWORK] Retry count left: %d\n", maxRetry).c_str());
	}
	
OUTTER_LOOP_BREAK:
	m_running = false;
}

NetErrResult NetworkManagerClient::Connect(const char* host, const char* port, bool nio)
{
	NetErrResult result = {};

	if (m_state != CONNECTION_STATE::DISCONNECTED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Already connected";
		return result;
	}

	if (!NET_WINSOCK_INITIALIZED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Winsock not initialized!";
		return result;
	}

	AddrInfoPtr pAIResult;
	addrinfo  addrinfoHints = {};

	addrinfoHints.ai_family = AF_UNSPEC;
	addrinfoHints.ai_socktype = SOCK_STREAM;
	addrinfoHints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	result.m_errCode = getaddrinfo(host, port, &addrinfoHints, &pAIResult.pAddrInfo);
	if (result.m_errCode != 0) {
		result.m_errMsg = "getaddrinfo failed";
		return result;
	}

	// Attempt to connect to the first address returned by the call to getaddrinfo

	// Create a SOCKET for connecting to server
	m_socket = socket(pAIResult.pAddrInfo->ai_family, pAIResult.pAddrInfo->ai_socktype, pAIResult.pAddrInfo->ai_protocol);

	if (m_socket == INVALID_SOCKET) {
		result.m_errMsg = Stringf("Error at socket(): %ld", WSAGetLastError());
		return result;
	}

	if (nio)
	{
		u_long mode = 1; // non-blocking
		result.m_errCode = ioctlsocket(m_socket, FIONBIO, &mode);
		if (result.m_errCode != 0)
		{
			result.m_errMsg = Stringf("ioctlsocket failed: %d", WSAGetLastError());
			return result;
		}
	}

	// Connect to server.
	m_state = CONNECTION_STATE::ESTABLISHING;
	result.m_errCode = connect(m_socket, pAIResult.pAddrInfo->ai_addr, (int)pAIResult.pAddrInfo->ai_addrlen);
	if (result.m_errCode == SOCKET_ERROR) {
		result.m_errCode = WSAGetLastError();
		if (result.m_errCode != WSAEWOULDBLOCK)
		{
			CloseConnection();
			result.m_errMsg = Stringf("Unable to connect to server!");
			return result;
		}

		result.m_errMsg = "establishing!";
		return result;
	}

	m_state = CONNECTION_STATE::CONNECTED;
	result.m_errMsg = "success!";
	return result;
}

NetErrResult NetworkManagerClient::ClientRecvData()
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::CONNECTED)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Not connected";
		return result;
	}

	bool state = false;
	result = QueryState(m_socket, false, state);
	if (result.m_errCode != 0)
	{
		CloseConnection();
		return result;
	}
	if (!state)
	{
		result.m_errCode = 0;
		result.m_errMsg = "blocking";
		return result;
	}

	char buffer[NET_READ_BUFFER_SIZE] = {};

	int lengthRecv = recv(m_socket, buffer, NET_READ_BUFFER_SIZE, 0);
	if (lengthRecv == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		if (result.m_errCode != WSAEWOULDBLOCK)
		{
			result.m_errMsg = "Socket error";
			CloseConnection();
			return result;
		}

		// blocking, do nothing
		result.m_errCode = 0;
		return result;
	}

	m_bufferRecv.WriteBytes(lengthRecv, buffer);

	return result;
}

NetErrResult NetworkManagerClient::ClientSendData()
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::CONNECTED)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Not connected";
		return result;
	}

	bool state = false;
	result = QueryState(m_socket, true, state);
	if (result.m_errCode != 0)
	{
		CloseConnection();
		return result;
	}
	if (!state)
	{
		result.m_errCode = 0;
		result.m_errMsg = "blocking";
		return result;
	}

	char buffer[NET_WRITE_BUFFER_SIZE] = {};
	int length = (int)m_bufferSend.ReadBytes(NET_WRITE_BUFFER_SIZE, buffer);

	if (length == 0)
	{
		result.m_errMsg = "empty";
		return result;
	}

	int lengthSent = send(m_socket, buffer, length, 0);
	if (lengthSent == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		if (result.m_errCode != WSAEWOULDBLOCK)
		{
			result.m_errMsg = "Socket error";
			CloseConnection();
			return result;
		}

		// blocking, do nothing
		result.m_errCode = 0;
		return result;
	}

	return result;
}

NetErrResult NetworkManagerClient::ValidateConnection()
{
	NetErrResult result;

	if (m_socket == INVALID_SOCKET)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Invalid socket";
		return result;
	}

	if (m_state != CONNECTION_STATE::ESTABLISHING)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Invalid state";
		return result;
	}

	bool state = false;
	result = QueryState(m_socket, true, state);
	if (result.m_errCode != 0)
	{
		CloseConnection();
		return result;
	}

	if (state)
		m_state = CONNECTION_STATE::CONNECTED;
	return result;
}

NetErrResult NetworkManagerClient::CloseConnection()
{
	NetErrResult result;

	if (m_socket == INVALID_SOCKET)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Invalid socket";
		return result;
	}

	result.m_errCode = closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	m_state = CONNECTION_STATE::DISCONNECTED;
	return result;
}

NetworkManagerClient::NetworkManagerClient()
{
}

NetworkManagerClient::~NetworkManagerClient()
{
	ReleaseClient();
}

NetworkManagerServer::ClientConnection::ClientConnection()
{
}

NetworkManagerServer::ClientConnection::~ClientConnection()
{
	ReleaseConnection();
}

void NetworkManagerServer::ClientConnection::CreateConnection()
{
	m_thread = new std::thread([this]() { RunNetworkThread(); });
}

void NetworkManagerServer::ClientConnection::ReleaseConnection()
{
	if (!m_running)
		return;

	m_disconnect = 1;
	m_thread->join();
	delete m_thread;
	m_thread = nullptr;
}

void NetworkManagerServer::ClientConnection::NetworkTickConnection(NetworkManagerServer* server)
{
	Packet packet;
	while (m_bufferRecv.ReadPacket(packet))
		server->HandlePacket(this, packet);

	for (const auto& pkt : m_queue)
		m_bufferSend.WritePacket(pkt);
	m_queue.clear();
}

void NetworkManagerServer::ClientConnection::Send(Packet& pkt)
{
	m_queue.push_back(pkt);
}

void NetworkManagerServer::ClientConnection::RunNetworkThread()
{
	m_running = true;
	m_state = CONNECTION_STATE::CONNECTED;

	while (m_state == CONNECTION_STATE::CONNECTED)
	{
		ClientRecvData();
		ClientSendData();

		if (m_disconnect)
		{
			m_disconnect = false;
			CloseConnection();
			break;
		}
	}

	m_running = false;
}

NetErrResult NetworkManagerServer::ClientConnection::ClientRecvData()
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::CONNECTED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Not connected!";
		return result;
	}

	bool state = false;
	result = QueryState(m_socket, false, state);
	if (result.m_errCode != 0)
	{
		CloseConnection();
		return result;
	}
	if (!state)
	{
		result.m_errCode = 0;
		result.m_errMsg = "blocking";
		return result;
	}

	char buffer[NET_READ_BUFFER_SIZE] = {};

	int lengthRecv = recv(m_socket, buffer, NET_READ_BUFFER_SIZE, 0);
	if (lengthRecv == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		if (result.m_errCode != WSAEWOULDBLOCK)
		{
			result.m_errMsg = "Socket error";
			CloseConnection();
			return result;
		}

		// blocking, do nothing
		result.m_errCode = 0;
		return result;
	}

	m_bufferRecv.WriteBytes(lengthRecv, buffer);
	return result;
}

NetErrResult NetworkManagerServer::ClientConnection::ClientSendData()
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::CONNECTED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Not connected!";
		return result;
	}

	bool state = false;
	result = QueryState(m_socket, true, state);
	if (result.m_errCode != 0)
	{
		CloseConnection();
		return result;
	}
	if (!state)
	{
		result.m_errCode = 0;
		result.m_errMsg = "blocking";
		return result;
	}

	char buffer[NET_WRITE_BUFFER_SIZE] = {};
	int length = (int)m_bufferSend.ReadBytes(NET_WRITE_BUFFER_SIZE, buffer);

	int lengthSent = send(m_socket, buffer, length, 0);
	if (lengthSent == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		if (result.m_errCode != WSAEWOULDBLOCK)
		{
			result.m_errMsg = "Socket error";
			CloseConnection();
			return result;
		}

		// blocking, do nothing
		result.m_errCode = 0;
		return result;
	}

	return result;
}

NetErrResult NetworkManagerServer::ClientConnection::CloseConnection()
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::CONNECTED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Not connected!";
		return result;
	}

	result.m_errCode = shutdown(m_socket, SD_SEND);
	if (result.m_errCode == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = " Failed to shutdown connection";
		closesocket(m_socket);
		m_state = CONNECTION_STATE::DISCONNECTED;
		return result;
	}

	m_state = CONNECTION_STATE::DISCONNECTED;
	return result;
}

NetworkManagerServer::NetworkManagerServer()
{
}

NetworkManagerServer::~NetworkManagerServer()
{
	ReleaseServer();
}

NetErrResult NetworkManagerServer::CreateServer(const char* port /*= DEFAULT_PORT*/)
{
	NetErrResult result;
	if (m_running)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Already running!";
		return result;
	}

	std::string strPort = port;

	m_thread = new std::thread([this, strPort]() { RunNetworkThread(strPort.c_str()); });
	return result;
}

void NetworkManagerServer::ReleaseServer()
{
	if (!m_running)
		return;

	m_disconnect = 1;
	SOCKET socket = m_socket;
	m_socket = INVALID_SOCKET;
	closesocket(socket);
	m_thread->join();

	{
		std::lock_guard<std::recursive_mutex> guard(m_lock);
		for (auto& conn : m_connections)
		{
			conn->ReleaseConnection();
			delete conn;
		}
		m_connections.clear();
	}

	delete m_thread;
	m_thread = nullptr;
}

void NetworkManagerServer::NetworkTickServer()
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
	for (auto& conn : m_connections)
		if (conn->m_state == CONNECTION_STATE::CONNECTED)
			conn->NetworkTickConnection(this);
}

int NetworkManagerServer::GetActiveConnections()
{
	int count = 0;

	std::lock_guard<std::recursive_mutex> guard(m_lock);
	for (auto& conn : m_connections)
		if (conn->m_state == CONNECTION_STATE::CONNECTED)
			count++;

	return count;
}

void NetworkManagerServer::SendTo(int client, Packet& pkt)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
	for (auto& conn : m_connections)
		if (conn->m_idx == client)
			conn->Send(pkt);
}

void NetworkManagerServer::Broadcast(Packet& pkt)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
	for (auto& conn : m_connections)
			conn->Send(pkt);
}

void NetworkManagerServer::HandlePacket(ClientConnection* connection, Packet& pkt)
{
	m_handlers[pkt.m_type](connection->m_idx, pkt);
}

void NetworkManagerServer::RegisterHandler(int type, ServerPacketHandler handler)
{
	m_handlers[type] = handler;
}

void NetworkManagerServer::RunNetworkThread(const char* port)
{
	m_running = true;

	NetErrResult result;

	DebuggerPrintf(Stringf("[NETWORK] Starting server on%s...\n", port).c_str());

	Bind(port);

	if (m_state == CONNECTION_STATE::LISTENING)
		DebuggerPrintf(Stringf("[NETWORK] Successfully bind port.\n").c_str());
	while (m_state == CONNECTION_STATE::LISTENING)
	{
		Accept(true);
		DebuggerPrintf(Stringf("[NETWORK] New connection initialized.\n").c_str());

		if (m_disconnect)
		{
			m_disconnect = 0;
			CloseConnection();
			m_state = CONNECTION_STATE::DISCONNECTED;
			DebuggerPrintf(Stringf("[NETWORK] Disconnected!\n").c_str());
			break;
		}
	}

	DebuggerPrintf(Stringf("[NETWORK] Server stopped!\n").c_str());
	m_running = false;
}

NetErrResult NetworkManagerServer::Bind(const char* port)
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::DISCONNECTED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Already listening!";
		return result;
	}

	if (!NET_WINSOCK_INITIALIZED)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Winsock not initialized!";
		return result;
	}

	AddrInfoPtr pAIresult;
	addrinfo aiHints = {};

	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;
	aiHints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	result.m_errCode = getaddrinfo(NULL, port, &aiHints, &pAIresult.pAddrInfo);
	if (result.m_errCode != 0) {
		result.m_errMsg = "getaddrinfo failed";
		return result;
	}

	// Create a SOCKET for server
	m_socket = socket(pAIresult.pAddrInfo->ai_family, pAIresult.pAddrInfo->ai_socktype, pAIresult.pAddrInfo->ai_protocol);
	if (m_socket == INVALID_SOCKET) {
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = "Failed to create socket";
		return result;
	}

	// Attempt to bind port
	result.m_errCode = bind(m_socket, pAIresult.pAddrInfo->ai_addr, (int) pAIresult.pAddrInfo->ai_addrlen);
	if (result.m_errCode == SOCKET_ERROR) {
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = "Failed to bind port";
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return result;
	}

	result.m_errCode = listen(m_socket, SOMAXCONN);
	if (result.m_errCode == SOCKET_ERROR)
	{
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = "Failed to listen on port";
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return result;
	}

	m_state = CONNECTION_STATE::LISTENING;
	return result;
}

NetErrResult NetworkManagerServer::Accept(bool nio)
{
	NetErrResult result;

	if (m_state != CONNECTION_STATE::LISTENING)
	{
		result.m_errCode = -1;
		result.m_errMsg = "Not listening!";
		return result;
	}

	SOCKET clientSocket = INVALID_SOCKET;

	clientSocket = accept(m_socket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET)
	{
		result.m_errCode = WSAGetLastError();
		result.m_errMsg = "Failed to accept new connection";
		CloseConnection();
		return result;
	}

	if (nio)
	{
		u_long mode = 1; // non-blocking
		result.m_errCode = ioctlsocket(clientSocket, FIONBIO, &mode);
		if (result.m_errCode != 0)
		{
			result.m_errCode = WSAGetLastError();
			result.m_errMsg = "ioctlsocket failed";
			closesocket(clientSocket);
			return result;
		}
	}

	static int clientConnectionIdx = 12;

	{
		std::lock_guard<std::recursive_mutex> guard(m_lock);

		ClientConnection* connection = new ClientConnection();
		m_connections.push_back(connection);

		connection->m_socket = clientSocket;
		connection->m_idx = clientConnectionIdx++;
		connection->CreateConnection();
		return result;
	}
}

NetErrResult NetworkManagerServer::CloseConnection()
{
	NetErrResult result;

	if (m_socket == INVALID_SOCKET)
	{
		result.m_errCode = 1;
		result.m_errMsg = "Invalid socket";
		return result;
	}

	result.m_errCode = closesocket(m_socket);
	m_connections.clear();
	m_socket = INVALID_SOCKET;
	m_state = CONNECTION_STATE::DISCONNECTED;
	return result;
}

bool PacketBuffer::ReadPacket(Packet& packet)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);

	size_t pktSize = sizeof(PKT_HEADER);
	if (!IsReadable(pktSize))
		return false;
	PKT_HEADER* header = (PKT_HEADER*)m_data.data();

	pktSize += header->size;
	if (!IsReadable(pktSize))
		return false;

	Read(1, &packet.m_type);
	Read(1, &packet.m_writeIdx);
	packet.m_data.resize(packet.m_writeIdx);
	Read(packet.m_writeIdx, packet.m_data.data());
	return true;
}

void PacketBuffer::WritePacket(const Packet& packet)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);

	Write(1, &packet.m_type);
	Write(1, &packet.m_writeIdx);
	Write(packet.m_writeIdx, packet.m_data.data());
}

bool PacketBuffer::IsReadable(size_t size)
{
	return m_data.size() >= size;
}

size_t PacketBuffer::ReadBytes(size_t size, char* data)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);

	if (size > m_data.size())
		size = m_data.size();

	if (size == 0)
		return 0;

	Read(size, data);
	return size;
}

size_t PacketBuffer::WriteBytes(size_t size, const char* data)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);

	if (size == 0)
		return 0;

	Write(size, data);
	return size;
}

Packet::Packet()
{
}

Packet::Packet(const Packet& copyFrom)
	: m_type(copyFrom.m_type)
	, m_data(copyFrom.m_data)
	, m_writeIdx(copyFrom.m_writeIdx)
	, m_readIdx(copyFrom.m_readIdx)
{
}

Packet::~Packet()
{
}

void Packet::WriteString(const std::string& str)
{
	size_t size = str.size();
	Write((int)size);
	Write(size + 1, str.c_str());
}

std::string Packet::ReadString()
{
	int size;
	Read(1, &size);

	size_t shrinkSize = sizeof(char) * (size + 1L);

	if (m_readIdx + shrinkSize > m_writeIdx)
	{
		ERROR_RECOVERABLE("Data index out of bounds!");
		return "";
	}
	std::string str = std::string(&m_data[m_readIdx]);
	m_readIdx += (int)shrinkSize;

	return str;
}

