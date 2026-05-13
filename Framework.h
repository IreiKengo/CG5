#pragma once

class WinApp;
class Input;
class DirectXCommon;
class SrvManager;
class Sound;
class ImguiManager;


//ゲーム全体
class Framework
{

public:

	//初期化
	virtual void Initialize();

	//終了
	virtual void Finalize();

	//更新
	virtual void Update();

	//描画
	virtual void Draw() = 0;

	//終了チェック
	virtual bool IsEndRequest() { return endRequest_; }

	virtual ~Framework() = default;

	//実行
	void Run();

private:

	bool endRequest_ = false;

protected:
	//ポインタ
	WinApp* winApp = nullptr;

	Input* input = nullptr;

	//ポインタ
	DirectXCommon* dxCommon = nullptr;

	SrvManager* srvManager = nullptr;

	Sound* sound = nullptr;

	ImguiManager* imgui = nullptr;


};
