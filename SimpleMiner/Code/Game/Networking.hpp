// ================ BUFFER  UTILITIES SECTION =============== //
#include "Engine/Core/ErrorWarningAssert.hpp"

#pragma once

#include <vector>
#include <mutex>
#include <thread>

enum class TagType
{
	Char,
	Int,
	Long,
	Float,
	Double,
	String,
};

class Packet
{
public:
	int m_type = 0;
	std::vector<char> m_data;

	int m_readIdx = 0;
	int m_writeIdx = 0;

public:
	Packet();
	Packet(const Packet& copyFrom);
	~Packet();

	template<typename T>
	void Write(size_t arrLen, const T* arrPtr)
	{
		size_t appendSize = sizeof(T) * arrLen;

		if (m_data.size() < m_writeIdx + appendSize)
			m_data.resize(m_writeIdx + appendSize);

		memcpy(&m_data[m_writeIdx], arrPtr, appendSize);
		m_writeIdx += (int)appendSize;
	}

	template<typename T>
	void Read(size_t arrLen, T* arrPtr)
	{
		size_t shrinkSize = sizeof(T) * arrLen;

		if (m_readIdx + shrinkSize > m_writeIdx)
		{
			ERROR_RECOVERABLE("Data index out of bounds!");
			return;
		}
		memcpy(arrPtr, &m_data[m_readIdx], shrinkSize);
		m_readIdx += (int)shrinkSize;
	}

	template<typename T>
	void Write(const T& val)
	{
		Write(1, &val);
	}

	template<typename T>
	void Read(T& val)
	{
		Read(1, &val);
	}

	void WriteString(const std::string& str);
	std::string ReadString();
};

class PacketBuffer
{
private:
	struct PKT_HEADER
	{
		int type;
		int size;
	};

public:
	std::recursive_mutex m_lock;
	std::vector<char> m_data;

public:
	size_t ReadBytes(size_t size, char* data);
	size_t WriteBytes(size_t size, const char* data);

	bool ReadPacket(Packet& packet);
	void WritePacket(const Packet& packet);

private:
	template<typename T>
	void Write(size_t arrLen, const T* arrPtr)
	{
		size_t oldSize = m_data.size();
		size_t appendSize = sizeof(T) * arrLen;
		m_data.resize(m_data.size() + appendSize);
		memcpy(&m_data[oldSize], arrPtr, appendSize);
	}

	template<typename T>
	void Read(size_t arrLen, T* arrPtr)
	{
		size_t oldSize = m_data.size();
		size_t shrinkSize = sizeof(T) * arrLen;
		size_t newSize = oldSize - shrinkSize;
		memcpy(arrPtr, m_data.data(), shrinkSize);
		if (newSize > 0)
			memmove(m_data.data(), &m_data[shrinkSize], newSize);
		m_data.resize(newSize);
	}

	bool IsReadable(size_t size);
};


// ================ NETWORK UTILITIES SECTION ================ //


#pragma once

#include <string>
#include <map>
#include <atomic>

constexpr const char* DEFAULT_HOST = "127.0.0.1"; // First in local
constexpr const char* DEFAULT_PORT = "25564"; // I'm not minecraft
constexpr const int   DEFAULT_RETRY_TIME = 1500;
constexpr const int   DEFAULT_RETRY_COUNT = 3;

struct NetErrResult
{
	int m_errCode = 0;
	std::string m_errMsg = "success";
};

typedef unsigned long long UINT_PTR;
typedef UINT_PTR        SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)

enum class CONNECTION_STATE
{
	DISCONNECTED,
	ESTABLISHING,
	CONNECTED,
	LISTENING,
};

const char* GetNameFromType(CONNECTION_STATE type);

CONNECTION_STATE GetTypeByName(const char* name, CONNECTION_STATE defaultType);

NetErrResult StartupNetworking();
NetErrResult StopNetworking();

typedef void(*PacketHandler)(Packet& packet);
typedef void(*ServerPacketHandler)(int client, Packet& packet);

class NetworkManagerClient
{
public:
	std::thread* m_thread = nullptr;
	std::atomic_int m_disconnect = 0;
	std::atomic_int m_running = 0;
	std::atomic<CONNECTION_STATE> m_state = CONNECTION_STATE::DISCONNECTED;
	bool m_canRead = false;
	bool m_canWrite = false;

	SOCKET m_socket = INVALID_SOCKET;

	PacketBuffer m_bufferSend;
	PacketBuffer m_bufferRecv;

	std::vector<Packet> m_queue;
	std::map<int, PacketHandler> m_handlers;
	
	NetworkManagerClient();
	NetworkManagerClient(const NetworkManagerClient& copyFrom) = delete;
	~NetworkManagerClient();

	NetErrResult CreateClient(const char* host = DEFAULT_HOST, const char* port = DEFAULT_PORT, int retryTMillis = DEFAULT_RETRY_TIME, int maxRetry = DEFAULT_RETRY_COUNT);
	void ReleaseClient();
	void NetworkTickClient();

	void SendToServer(Packet& pkt);
	void HandlePacket(Packet& pkt);
	void RegisterHandler(int type, PacketHandler handler);

private:
	void RunNetworkThread(const char* host, const char* port, int retryTMillis, int maxRetry);
	NetErrResult Connect(const char* host, const char* port, bool nio);
	NetErrResult ClientRecvData();
	NetErrResult ClientSendData();
	NetErrResult ValidateConnection();
	NetErrResult CloseConnection();
};


class NetworkManagerServer
{
private:
	struct ClientConnection
	{
	public:
		std::thread* m_thread = nullptr;
		std::atomic_int m_disconnect = 0;
		std::atomic_int m_running = 0;
		std::atomic<CONNECTION_STATE> m_state = CONNECTION_STATE::DISCONNECTED;
		int m_idx = -1;
		SOCKET m_socket = INVALID_SOCKET;

		PacketBuffer m_bufferSend;
		PacketBuffer m_bufferRecv;

		std::vector<Packet> m_queue;

	public:
		ClientConnection();
		ClientConnection(const ClientConnection& copyFrom) = delete;
		~ClientConnection();

		void CreateConnection();
		void ReleaseConnection();
		void NetworkTickConnection(NetworkManagerServer* server);

		void Send(Packet& pkt);

	private:
		void RunNetworkThread();
		NetErrResult ClientRecvData();
		NetErrResult ClientSendData();
		NetErrResult CloseConnection();
	};

public:
	std::recursive_mutex m_lock;
	std::thread* m_thread = nullptr;
	std::atomic_int m_disconnect = 0;
	std::atomic_int m_running = 0;
	std::atomic<SOCKET> m_socket = INVALID_SOCKET;
	std::atomic<CONNECTION_STATE> m_state = CONNECTION_STATE::DISCONNECTED;

	std::vector<ClientConnection*> m_connections;

	std::map<int, ServerPacketHandler> m_handlers;

	NetworkManagerServer();
	~NetworkManagerServer();

	NetErrResult CreateServer(const char* port = DEFAULT_PORT);

	void ReleaseServer();
	void NetworkTickServer();

	int GetActiveConnections();
	void SendTo(int client, Packet& pkt);
	void Broadcast(Packet& pkt);
	void HandlePacket(ClientConnection* connection, Packet& pkt);
	void RegisterHandler(int type, ServerPacketHandler handler);

private:
	void RunNetworkThread(const char* port);
	NetErrResult Bind(const char* port);
	NetErrResult Accept(bool nio);
	NetErrResult CloseConnection();
};

