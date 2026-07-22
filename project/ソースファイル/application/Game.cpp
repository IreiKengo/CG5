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
#include "ImguiManager.h"
#include "PostEffect.h"
#include "Input.h"

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
	camera->SetRotate({ 0.1396f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,4.3f,-31.0f });
#pragma endregion 

#pragma region スプライト関連

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon);
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	//スプライト共通部の初期化
	spriteCommon = new SpriteCommon;
	spriteCommon->Initialize(dxCommon);


	sprite = new Sprite();

	std::string texturePath = "resources/uvChecker.png";
	sprite->Initialize(spriteCommon, texturePath);

#pragma endregion

#pragma region パーティクル
	ParticleManager::GetInstance()->Initialize(dxCommon);
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
	ModelManager::GetInstance()->LoadModel("terrain.obj");

	//3Dオブジェクト共通部の初期化
	object3dCommon = new Object3dCommon;
	object3dCommon->SetDefaultCamera(camera);
	object3dCommon->Initialize(dxCommon);

	for (int i = 0; i < 2; ++i)
	{

		object[i] = new Object3d();

		std::string modelPath;
		if (i % 2 == 0) {
			modelPath = "axis.obj";
		} else {

			modelPath = "terrain.obj";
		}

		object[i]->Initialize(object3dCommon);
		object[i]->SetModel(modelPath);

	}

#pragma endregion

	postEffect = new PostEffect();

	postEffect->Initialize(dxCommon,camera, "resources/noise0.png");

}

void Game::Finalize()
{

	delete particleChecker;
	delete particleCircle;
	particleChecker = nullptr;
	particleCircle = nullptr;
	ParticleManager::GetInstance()->Finalize();

	delete postEffect;
	postEffect = nullptr;

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

	//sprite->DebugUpdate();

	postEffect->DebugUpdate();


	//カメラの更新
	camera->Update();

	sprite->Update();

	object[0]->SetTranslate({ -1.0f,-1.0f,0.0f });
	object[1]->SetTranslate({ 1.0f,-1.0f,0.0f });
	for (uint32_t i = 0; i < 2; ++i)
	{
		object[i]->Update();
	}

	postEffect->Update();
	UpdateEffectInput();
	
	float deltaTime = 1.0f / 60.0f; // 本来は実時間計測



	//particleCircle->Update(deltaTime);
	//particleChecker->Update(deltaTime);
	ParticleManager::GetInstance()->Update();

}

void Game::Draw()

{

	//DirectXの描画基準。全ての描画に共通宇のグラッフィックスコマンドを積む
	dxCommon->PreDraw();
	
	
	//3Dオブジェクトの描画準備。3Dオブジェクトの描画に共通のグラフィックスコマンドを積む
	object3dCommon->ScreenCommon();

	//全てのObject3d個々の描画
	for (uint32_t i = 0; i < 2; ++i)
	{

		object[i]->Draw();

	}
	
	//Spriteの描画基準。Spriteの描画の共通のグラッフィックスコマンドを積む
	//spriteCommon->ScreenCommon();


	//Spriteの描画
	//sprite->Draw();


	//ParticleManager::GetInstance()->Draw();

	postEffect->Draw();

	imgui->End();    // ImGui終了
	imgui->Draw();   // 描画

	//描画後処理
	dxCommon->PostDraw();

	TextureManager::GetInstance()->ReleaseIntermediateResources();

}

void Game::UpdateEffectInput()
{

	if (input->TriggerKey(DIK_1))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kGrayscale);

	} else if (input->TriggerKey(DIK_2))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kVignetting);
	} else if (input->TriggerKey(DIK_3))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kSmoothing);
	} else if (input->TriggerKey(DIK_4))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kGaussian);
	} else if (input->TriggerKey(DIK_5))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kLumBasedOutline);
	} else if (input->TriggerKey(DIK_6))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kDepthBasedOutline);
	} else if (input->TriggerKey(DIK_7))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kRadialBlur);
	} else if (input->TriggerKey(DIK_8))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kDissolve);
	} else if (input->TriggerKey(DIK_9))
	{
		postEffect->SetEffectType(PostEffect::EffectType::kRandom);
	}



	// 1. Vignette (1:Scale, 2:Power)
	if (postEffect->GetEffectType() == PostEffect::EffectType::kVignetting)
	{
		if (input->PushKey(DIK_W))    postEffect->AddVignetteScale(0.1f);
		if (input->PushKey(DIK_S))  postEffect->AddVignetteScale(-0.1f);
		if (input->PushKey(DIK_D)) postEffect->AddVignettePower(0.01f);
		if (input->PushKey(DIK_A))  postEffect->AddVignettePower(-0.01f);
	}
	// 2. Gaussian (1:Kernel, 2:Sigma)
	else if (postEffect->GetEffectType() == PostEffect::EffectType::kGaussian)
	{
		if (input->TriggerKey(DIK_W))   postEffect->AddGaussianKernel(1);
		if (input->TriggerKey(DIK_S)) postEffect->AddGaussianKernel(-1);
		if (input->PushKey(DIK_D))   postEffect->AddGaussianSigma(0.05f);
		if (input->PushKey(DIK_A))    postEffect->AddGaussianSigma(-0.05f);
	}
	// 3. Luminance Based Outline (1:EdgeWeight)
	else if (postEffect->GetEffectType() == PostEffect::EffectType::kLumBasedOutline)
	{
		if (input->PushKey(DIK_W))   postEffect->AddLumOutlineEdgeWeight(0.1f);
		if (input->PushKey(DIK_S)) postEffect->AddLumOutlineEdgeWeight(-0.1f);
	}
	// 4. Depth Based Outline (1:EdgeWeight)
	else if (postEffect->GetEffectType() == PostEffect::EffectType::kDepthBasedOutline)
	{
		if (input->PushKey(DIK_W))   postEffect->AddDepthOutlineEdgeWeight(0.1f);
		if (input->PushKey(DIK_S)) postEffect->AddDepthOutlineEdgeWeight(-0.1f);
	}
	// 5. Radial Blur (1:BlurWidth, 2:Center Position)
	else if (postEffect->GetEffectType() == PostEffect::EffectType::kRadialBlur)
	{
		if (input->PushKey(DIK_UP))    postEffect->AddRadialBlurWidth(0.005f);
		if (input->PushKey(DIK_DOWN))  postEffect->AddRadialBlurWidth(-0.005f);
		float moveX = 0.0f;
		float moveY = 0.0f;

		if (input->PushKey(DIK_D)) moveX += 0.01f; // 右
		if (input->PushKey(DIK_A)) moveX -= 0.01f; // 左
		if (input->PushKey(DIK_S)) moveY += 0.01f; // 下（UV座標系では下がプラス）
		if (input->PushKey(DIK_W)) moveY -= 0.01f; // 上（UV座標系では上がマイナス）

		// X と Y の移動量を両方渡す！
		if (moveX != 0.0f || moveY != 0.0f) {
			postEffect->AddRadialBlurCenter(moveX, moveY);
		}
	}
	// 6. Dissolve (1:Threshold, 2:EdgeRange)
	else if (postEffect->GetEffectType() == PostEffect::EffectType::kDissolve)
	{
		if (input->PushKey(DIK_W))    postEffect->AddDissolveThreshold(0.01f);
		if (input->PushKey(DIK_S))  postEffect->AddDissolveThreshold(-0.01f);
		if (input->PushKey(DIK_D)) postEffect->AddDissolveEdgeRange(0.005f);
		if (input->PushKey(DIK_A))  postEffect->AddDissolveEdgeRange(-0.005f);
	}
	
}

