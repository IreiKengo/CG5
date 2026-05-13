#include "Logger.h"
#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <chrono>

#include <format>

namespace
{
	std::ofstream logStream;
}

namespace Logger
{

	std::wstring ToWString(const std::string& str)
	{
		if (str.empty()) return L"";

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
		std::wstring wstr(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
		return wstr;
	}


	void Log(const std::string& message)
	{
		OutputDebugStringW(ToWString(message).c_str());
	}

	void Log(std::ostream& os, const std::string& message)
	{
		os << message << std::endl;

		OutputDebugStringW(ToWString(message).c_str());
	}

	// ログファイル生成
	void Initialize()
	{
		//log出力用のフォルダ「logs」を作成
		std::filesystem::create_directory("logs");

		//ここからファイルを作成し、ofstreamを取得する
		//現時刻を取得
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		//ログファイルの名前にコンマ何秒はいらないので、削って秒にする
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
			nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		//日本時間に変換
		std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
		//formatを使って年月日_時分秒の文字列に変換
		std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
		//時刻を使ってファイル名を決定
		std::string logFilePath = std::format("logs/") + dateString + ".log";
		//ファイルを使って書き込み準備
		logStream.open(logFilePath);
	}

}
