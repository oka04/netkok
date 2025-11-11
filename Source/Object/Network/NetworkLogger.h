#pragma once
#include <winsock2.h>    
#include <Ws2tcpip.h>   
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>

class NetworkLogger
{
public:
	static NetworkLogger& GetInstance()
	{
		static NetworkLogger instance;
		return instance;
	}

	// ログファイルを初期化（既存の内容を削除）
	void Initialize(const std::string& filename = "network_debug.txt")
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_filename = filename;

		// ファイルを空にして開く
		std::ofstream file(m_filename, std::ios::trunc);
		if (file.is_open())
		{
			file << "===========================================\n";
			file << "Network Debug Log\n";
			file << "===========================================\n\n";
			file.close();
		}
	}

	// ログを書き込む
	void Log(const std::string& message)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::ofstream file(m_filename, std::ios::app);
		if (file.is_open())
		{
			// タイムスタンプを追加
			auto now = std::time(nullptr);
			auto tm = *std::localtime(&now);

			file << "["
				<< std::put_time(&tm, "%H:%M:%S")
				<< "] "
				<< message
				<< std::endl;

			file.close();
		}
	}

	// フォーマット付きログ（printfスタイル）
	template<typename... Args>
	void LogFormat(const char* format, Args... args)
	{
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), format, args...);
		Log(std::string(buffer));
	}

private:
	NetworkLogger() : m_filename("network_debug.txt") {}
	~NetworkLogger() {}

	NetworkLogger(const NetworkLogger&) = delete;
	NetworkLogger& operator=(const NetworkLogger&) = delete;

	std::string m_filename;
	std::mutex m_mutex;
};

// 便利なマクロ
#define NET_LOG(msg) NetworkLogger::GetInstance().Log(msg)
#define NET_LOG_F(...) NetworkLogger::GetInstance().LogFormat(__VA_ARGS__)