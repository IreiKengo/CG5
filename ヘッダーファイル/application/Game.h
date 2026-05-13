#pragma once
#include <Windows.h>
#include "Framework.h"


class Camera;
class SpriteCommon;
class Object3dCommon;
class Sprite;
class Object3d;
class ParticleEmitter;



class Game : public Framework
{

public:

	void Initialize() override;

	void Finalize() override;

	void Update() override;

	void Draw() override;
	
	

private:

	

	Camera* camera = nullptr;

	

	//SpriteCommonの初期化
	SpriteCommon* spriteCommon = nullptr;

	//3Dオブジェクト共通部
	Object3dCommon* object3dCommon = nullptr;

	Sprite* sprite = nullptr;	

	Object3d* object[2] = { nullptr };


	ParticleEmitter* particleCircle = nullptr;

	ParticleEmitter* particleChecker = nullptr;

};
