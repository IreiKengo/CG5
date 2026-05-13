#include "Framework.h"
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ImguiManager.h"
#include "Sound.h"

void Framework::Initialize()
{

#pragma region WindowsAPIの初期化

	//WindowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

#pragma endregion

#pragma region DirectXの初期化

	//DirectXの初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

#pragma endregion

#pragma region DirectInputの初期化

	//入力の初期化
	input = new Input();
	input->Initialize(winApp);

#pragma endregion

#pragma region Sound

	sound = new Sound();

	sound->Initialize("resources/fanfare.mp3");

#pragma endregion

#pragma region SRVマネージャー

	//SRVマネージャーの初期化
	srvManager = new SrvManager();
	srvManager->Initialize(dxCommon);

#pragma endregion


#pragma region Imguiの初期化

	imgui = new ImguiManager();

	imgui->Initialize(winApp, dxCommon, srvManager);

#pragma endregion

}

void Framework::Update()
{

	if (winApp->ProcessMessage())
	{
		endRequest_ = true;
	}

	//入力の更新
	input->Update();

	imgui->Begin();

	//ImGui::ShowDemoWindow();
	
	if (input->TriggerKey(DIK_W))
	{
		sound->SoundPlayWave();
	}

}

void Framework::Run()
{

	Initialize();

	while (true)
	{

		Update();

		if (IsEndRequest())
		{
			break;
		}

		Draw();
	}
	Finalize();
}

void Framework::Finalize()
{

	imgui->Finalize();
	delete imgui;
	imgui = nullptr;

	delete srvManager;
	srvManager = nullptr;

	sound->SoundUnload();
	sound->Finalize();
	delete sound;
	sound = nullptr;

	//入力解放
	delete input;
	input = nullptr;

	//DirectX解放
	delete dxCommon;
	dxCommon = nullptr;

	//WindowsAPIの終了処理
	winApp->Finalize();

	//WindowsAPIの解放
	delete winApp;
	winApp = nullptr;
}

