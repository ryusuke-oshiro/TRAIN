// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_Player.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Kismet/KismetStringLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/TimelineComponent.h"
#include "Math/UnrealMathVectorCommon.h"

//カプセルコリジョンの半径
#define CAPSULE_RADIUS 35.0f
//カプセルコリジョンの高さ
#define CAPSULE_HALF_HEIGHT 90.0f

//歩く移動速度
#define WALK_SPEED 300.0f

//しゃがみの移動速度
#define CROUCH_SPEED 50.0f

//立っているときのカメラの高さ
#define STAND_CAMERA_Z 60.0f

//しゃがんでいるときのカメラの高さ
#define CROUCH_CAMERA_Z 30.0f

// Sets default values
ACPP_Player::ACPP_Player()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//CapsuleComponentの作成
	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
	//コリジョンサイズ設定
	CapsuleCollision->InitCapsuleSize(CAPSULE_RADIUS, CAPSULE_HALF_HEIGHT);
	//オーバーラップイベントの有効化
	CapsuleCollision->SetGenerateOverlapEvents(true);
	//コリジョンプリセットをPawnに設定
	CapsuleCollision->SetCollisionProfileName("Pawn");
	//物理を無効化
	CapsuleCollision->SetSimulatePhysics(false);

	//RootComponentに設定
	RootComponent = CapsuleCollision;

	// カメラの作成
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	//RootComponentにアタッチ
	Camera->SetupAttachment(RootComponent);
	//初期値の設定
	Camera->SetRelativeLocation(FVector(0.f, 0.f, STAND_CAMERA_Z));
	//コントローラーの回転を参照する
	Camera->bUsePawnControlRotation = true;

	//アローコンポーネント作成
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	//シーンルートにアタッチ
	Arrow->SetupAttachment(Camera);
	//プレイヤーの頭上にロケーションを設定する
	Arrow->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	//アローを非表示にする
	Arrow->bHiddenInGame = true;

	/*Input Actionの読み込み*/
	MoveInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Move"), NULL, LOAD_None, NULL);
	CrouchInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Crouch"), NULL, LOAD_None, NULL);
	LookInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Look"), NULL, LOAD_None, NULL);
	
	/*Input Mapping Contextの読み込み*/
	ActionMappingContext = LoadObject<UInputMappingContext>
		(NULL, TEXT("/Game/TRAIN/Core/Input/IMC_Controll"), NULL, LOAD_None, NULL);


	FallVelocity = 0.0f;

	MoveVelocity = WALK_SPEED;


	// カーブアセットの取得
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindCouchCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Crouch.CF_Crouch"));
	if (FindCouchCurve.Succeeded())
	{
		CrouchCurve = FindCouchCurve.Object;
	}

	//タイムラインの作成
	CrouchTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeline"));
	if (CrouchTimeLine) 
	{
		// タイムライン更新時に呼び出されるメソッドの定義
		FOnTimelineFloat CrouchStepFunc;
		CrouchStepFunc.BindUFunction(this, TEXT("Crouch"));
		CrouchTimeLine->AddInterpFloat(CrouchCurve, CrouchStepFunc);
	}

	//タイムラインの作成
	StandTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("StandTimeline"));
	if (StandTimeLine)
	{
		// タイムライン更新時に呼び出されるメソッドの定義
		FOnTimelineFloat StandStepFunc;
		StandStepFunc.BindUFunction(this, TEXT("Stand"));
		StandTimeLine->AddInterpFloat(CrouchCurve, StandStepFunc);
	}
}

// Called when the game starts or when spawned
void ACPP_Player::BeginPlay()
{
	Super::BeginPlay();
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			FModifyContextOptions option;
			option.bIgnoreAllPressedKeysUntilRelease = true;
			Subsystem->AddMappingContext(ActionMappingContext, 0, option);
		}
	}
}

// Called every frame
void ACPP_Player::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Falling(DeltaTime);
}

// Called to bind functionality to input
void ACPP_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//MoveInput
		EnhancedInputComponent->BindAction(MoveInput, ETriggerEvent::Triggered, this, &ACPP_Player::Move);

		//CrouchInput
		EnhancedInputComponent->BindAction(CrouchInput, ETriggerEvent::Started, this, &ACPP_Player::BeginCrouch);
		EnhancedInputComponent->BindAction(CrouchInput, ETriggerEvent::Completed, this, &ACPP_Player::EndCrouch);

		//LookInput
		EnhancedInputComponent->BindAction(LookInput, ETriggerEvent::Triggered, this, &ACPP_Player::Look);
	}

}

//落下処理
void ACPP_Player::Falling(const float& DeltaTime)
{
	//落下量の加算
	FallVelocity += (GetWorld()->GetGravityZ() / 2) * DeltaTime;

	//落下時の座標
	FVector falled_location = GetActorLocation() + FVector(0.0f, 0.0f, FallVelocity);

	//ヒットリザルト
	FHitResult hit;

	//移動
	SetActorLocation(falled_location, true, &hit, ETeleportType::None);

	if (hit.bBlockingHit) //何かにヒットした
	{
		//落下量のリセット
		FallVelocity = 0.0f;
	}
}

//移動処理
void ACPP_Player::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MoveAxisVector = Value.Get<FVector2D>();

	UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_Vector2dToString(MoveAxisVector),
		true, true, FColor::Blue, 20.0, TEXT("None"));
	FVector MoveVector; //移動量

	//左右移動の移動量の追加
	MoveVector = FVector(Camera->GetRightVector().X * MoveAxisVector.X * (MoveVelocity * FApp::GetDeltaTime()),
		Camera->GetRightVector().Y * MoveAxisVector.X * (MoveVelocity * FApp::GetDeltaTime()), 0.0f);

	//前後移動の移動量の追加
	MoveVector += FVector(Camera->GetForwardVector().X * MoveAxisVector.Y * (MoveVelocity * FApp::GetDeltaTime()),
		Camera->GetForwardVector().Y * MoveAxisVector.Y * (MoveVelocity * FApp::GetDeltaTime()), 0.0f);

	//ヒットリザルト
	FHitResult hit;
	//移動
	SetActorLocation((GetActorLocation() + MoveVector), true, &hit, ETeleportType::None);

	/*坂道を上る処理*/
	if (hit.bBlockingHit) //何かにヒットした
	{
		MoveVector = MoveVector - (hit.Normal * FVector::DotProduct(MoveVector, hit.Normal));
		SetActorLocation((GetActorLocation() + MoveVector), true, &hit, ETeleportType::None);
	}
	
}

//しゃがみ開始
void ACPP_Player::BeginCrouch(const FInputActionValue& Value)
{
	//立つTimeLineが再生中なら停止する
	if (StandTimeLine->IsPlaying())
	{
		StandTimeLine->Stop();
	}

	//しゃがむときのカプセルコリジョンの高さ
	HalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();
	//しゃがむときのカメラのZ座標
	CameraLocationZ = Camera->GetRelativeLocation().Z;

	//タイムラインの再生
	CrouchTimeLine->PlayFromStart();

	//移動速度をしゃがみ状態の移動速度に変更
	MoveVelocity = CROUCH_SPEED;
}

//しゃがみ終了
void ACPP_Player::EndCrouch(const FInputActionValue& Value)
{
	//しゃがむTimeLineが再生中なら停止する
	if (CrouchTimeLine->IsPlaying())
	{
		CrouchTimeLine->Stop();
	}

	//立つときの座標の設定
	StandLocation = CapsuleCollision->GetRelativeLocation();
	//立つときのカプセルコリジョンの高さ
	HalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();
	//立つときのカメラのZ座標
	CameraLocationZ = Camera->GetRelativeLocation().Z;

	//タイムラインの再生
	StandTimeLine->PlayFromStart();
	
	//移動速度を通常状態の移動速度に変更
	MoveVelocity = WALK_SPEED;
}

//しゃがみ状態に移行
void ACPP_Player::Crouch(const float& Value)
{
	float Height = FMath::Lerp(HalfHeight, (CAPSULE_HALF_HEIGHT / 2), Value);
	//カプセルコリジョンの大きさに変更
	CapsuleCollision->SetCapsuleHalfHeight(HalfHeight);

	//カメラの位置の変更
	float CameraLocation = FMath::Lerp(CameraLocationZ, CROUCH_CAMERA_Z, Value);

	Camera->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));
}

//立つ
void ACPP_Player::Stand(const float& Value)
{
	float Height = FMath::Lerp(HalfHeight, CAPSULE_HALF_HEIGHT, Value);
	//カプセルコリジョンをもとの大きさに変更
	CapsuleCollision->SetCapsuleHalfHeight(Height);

	float CameraLocation = FMath::Lerp(CameraLocationZ, STAND_CAMERA_Z, Value);
	//カメラの位置の変更
	Camera->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));

	float VelocityZ = FMath::Lerp(1.0f, CAPSULE_HALF_HEIGHT - HalfHeight + 1, Value);
	//立った時の地面への埋まりの修正
	CapsuleCollision->SetRelativeLocation((StandLocation + FVector(0.f, 0.f, VelocityZ)), false);
}

//視点移動
void ACPP_Player::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if ((Controller != nullptr))
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
