#include "Game.h"
#include "Logger.h"
#include "StringUtility.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "ModelCommon.h"
#include "Model.h"
#include"ModelManager.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include <dbghelp.h>
#include <strsafe.h>
#include "D3DResourceLeakChecker.h"
#include <filesystem>
#include "SrvManager.h"
#include "ImguiManager.h"

#pragma comment(lib,"Dbghelp.lib")

using namespace StringUtility;
using namespace Logger;


D3DResourceLeakChecker leakCheck;

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{

	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	//processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	//Dumpを入力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	//他に関連づけられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する

	return EXCEPTION_EXECUTE_HANDLER;

}

void Game::Initialize()
{


	Logger::Initialize();
	Log("Hello DirectX!\n");
	Log(
		ConvertString(
			std::format(
				L"clientSize:{},{}\n",
				WinApp::kClientWidth,
				WinApp::kClientHeight
			)
		)
	);


	SetUnhandledExceptionFilter(ExportDump);

	//基底クラスの初期化処理
	Framework::Initialize();

#pragma region カメラの初期化

	camera = new Camera();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-10.0f });
#pragma endregion 

#pragma region スプライト関連

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon, srvManager);
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	//スプライト共通部の初期化
	spriteCommon = new SpriteCommon;
	spriteCommon->Initialize(dxCommon);


	sprite = new Sprite();

	std::string texturePath = "resources/uvChecker.png";
	sprite->Initialize(spriteCommon, srvManager, texturePath);

#pragma endregion

#pragma region パーティクル
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager);
	ParticleManager::GetInstance()->SetCamera(camera);

	ParticleManager::GetInstance()->CreateParticleGroup(
		"circle",
		"resources/circle.png"
	);

	particleCircle = new ParticleEmitter
	(
		"circle",
		Vector3{ 0, 0, 0 },
		5,
		0.1f
	);

	ParticleManager::GetInstance()->CreateParticleGroup(
		"uvChecker",              //新しい名前にする
		"resources/uvChecker.png" //使いたい画像のパス
	);


	particleChecker = new ParticleEmitter(
		"uvChecker",
		Vector3{ 2.0f, 0, 0 },    // 位置を少しずらすと見やすいです
		5,                        // 発生数
		0.1f                      // 発生頻度
	);

#pragma endregion

#pragma region オブジェクト関係

	//3Dモデルマネージャの初期化
	ModelManager::GetInstance()->Initialize(dxCommon);

	//.objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("multiMesh.obj");
	ModelManager::GetInstance()->LoadModel("multiMaterial.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	//3Dオブジェクト共通部の初期化
	object3dCommon = new Object3dCommon;
	object3dCommon->SetDefaultCamera(camera);
	object3dCommon->Initialize(dxCommon);

	for (int i = 0; i < 2; ++i)
	{

		object[i] = new Object3d();

		std::string modelPath;
		if (i % 2 == 0) {
			modelPath = "plane.obj";
		} else {

			modelPath = "axis.obj";
		}

		object[i]->Initialize(object3dCommon);
		object[i]->SetModel(modelPath);

	}

#pragma endregion



}

void Game::Finalize()
{

	delete particleChecker;
	delete particleCircle;
	particleChecker = nullptr;
	particleCircle = nullptr;
	ParticleManager::GetInstance()->Finalize();

	for (uint32_t i = 0; i < 2; ++i)
	{
		delete object[i];
		object[i] = nullptr;
	}

	//Sprite解放

	//CloseHandle(dxCommon->GetFenceEvent());

	delete sprite;
	sprite = nullptr;


	//Dモデルマネージャの終了
	ModelManager::GetInstance()->Finalize();

	//TextureManager解放
	TextureManager::GetInstance()->Finalize();

	

	delete object3dCommon;

	delete camera;

	//SpriteCommon解放
	delete spriteCommon;

	//基底クラスの終了処理
	Framework::Finalize();
}

void Game::Update()
{

	//基底クラスの更新処理
	Framework::Update();

	camera->DebugUpdate();

	sprite->DebugUpdate();

	//カメラの更新
	camera->Update();

	sprite->Update();

	object[0]->SetTranslate({ -1.0f,-1.0f,0.0f });
	object[1]->SetTranslate({ 1.0f,-1.0f,0.0f });
	for (uint32_t i = 0; i < 2; ++i)
	{
		object[i]->Update();
	}

	float deltaTime = 1.0f / 60.0f; // 本来は実時間計測



	//particleCircle->Update(deltaTime);
	//particleChecker->Update(deltaTime);
	ParticleManager::GetInstance()->Update();

}

void Game::Draw()

{

	//DirectXの描画基準。全ての描画に共通宇のグラッフィックスコマンドを積む
	dxCommon->PreDraw();
	srvManager->PreDraw();

	//3Dオブジェクトの描画準備。3Dオブジェクトの描画に共通のグラフィックスコマンドを積む
	object3dCommon->ScreenCommon();


	//全てのObject3d個々の描画
	for (uint32_t i = 0; i < 2; ++i)
	{
		object[i]->Draw();

	}

	//Spriteの描画基準。Spriteの描画の共通のグラッフィックスコマンドを積む
	spriteCommon->ScreenCommon();


	//Spriteの描画
	sprite->Draw();


	//ParticleManager::GetInstance()->Draw();

	imgui->End();    // ImGui終了
	imgui->Draw();   // 描画

	//描画後処理
	dxCommon->PostDraw();

	TextureManager::GetInstance()->ReleaseIntermediateResources();

}

