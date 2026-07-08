#pragma once
#include <string>

//ログ出力
namespace Logger
{
	void Initialize();

	void Log(const std::string& message);
	void Log(std::ostream& os, const std::string& message);
}
