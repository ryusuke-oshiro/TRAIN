// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_Player.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Kismet/KismetStringLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/TimelineComponent.h"
#include "Math/UnrealMathVectorCommon.h"
#include "Kismet/GameplayStatics.h"


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

//インタラクト可能距離
#define INTERACT_LENGTH 150

//ズーム時のスプリングアームの距離
#define ZOOM_SPRING_ARM_LENGHT -400.0

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


	//スプリングアーム作成
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	//シーンルートにアタッチ
	SpringArm->SetupAttachment(RootComponent);
	//長さを設定
	SpringArm->TargetArmLength = 0.0f;
	//初期値の設定
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, STAND_CAMERA_Z));

	//コントローラーの回転を参照する
	SpringArm->bUsePawnControlRotation = true;
	// カメラの作成
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	//RootComponentにアタッチ
	Camera->SetupAttachment(SpringArm);
	
	

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
	InteractInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Interact"), NULL, LOAD_None, NULL);
	ZoomInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Zoom"), NULL, LOAD_None, NULL);
	LookInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Look"), NULL, LOAD_None, NULL);

	
	/*Input Mapping Contextの読み込み*/
	ActionMappingContext = LoadObject<UInputMappingContext>
		(NULL, TEXT("/Game/TRAIN/Core/Input/IMC_Controll"), NULL, LOAD_None, NULL);

	/*しゃがみ処理のタイムラインの設定*/
	// カーブアセットの取得
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindCouchCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Crouch.CF_Crouch"));
	if (FindCouchCurve.Succeeded())
	{
		CrouchCurve = FindCouchCurve.Object;
	}

	//タイムラインの作成
	CrouchTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeLine"));
	if (CrouchTimeLine)
	{
		// タイムライン更新時に呼び出されるメソッドの定義
		FOnTimelineFloat CrouchStepFunc;
		CrouchStepFunc.BindUFunction(this, TEXT("Crouch"));
		CrouchTimeLine->AddInterpFloat(CrouchCurve, CrouchStepFunc);
	}

	/*ズーム処理のタイムラインの設定*/
	// カーブアセットの取得
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindZoomCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Zoom.CF_Zoom"));
	if (FindCouchCurve.Succeeded())
	{
		ZoomCurve = FindZoomCurve.Object;
	}

	//タイムラインの作成
	ZoomTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("ZoomTimeLine"));
	if (ZoomTimeLine)
	{
		// タイムライン更新時に呼び出されるメソッドの定義
		FOnTimelineFloat ZoomStepFunc;
		ZoomStepFunc.BindUFunction(this, TEXT("Zoom"));
		ZoomTimeLine->AddInterpFloat(ZoomCurve, ZoomStepFunc);
	}

	FallVelocity = 0.0f;

	MoveVelocity = WALK_SPEED;

	IsStand = false;
	CapsuleHalfHeight = CAPSULE_HALF_HEIGHT;
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

		//InteractInput
		EnhancedInputComponent->BindAction(InteractInput, ETriggerEvent::Started, this, &ACPP_Player::Interact);

		//ZoomInput
		EnhancedInputComponent->BindAction(ZoomInput, ETriggerEvent::Started, this, &ACPP_Player::BeginZoom);
		EnhancedInputComponent->BindAction(ZoomInput, ETriggerEvent::Completed, this, &ACPP_Player::EndZoom);

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
	if (CrouchTimeLine->IsPlaying())
	{
		CrouchTimeLine->Stop();
	}

	//しゃがみ状態に設定
	IsStand = false;

	//タイムラインの再生
	CrouchTimeLine->Play();

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

	//立つ状態に設定
	IsStand = true;

	//カプセルの高さを設定
	CapsuleHalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();

	//タイムラインの再生
	CrouchTimeLine->Reverse();
	
	//移動速度を通常状態の移動速度に変更
	MoveVelocity = WALK_SPEED;
}

//しゃがみ
void ACPP_Player::Crouch(const float& Value)
{
	float Height = FMath::Lerp(CAPSULE_HALF_HEIGHT, (CAPSULE_HALF_HEIGHT / 2), Value);
	//カプセルコリジョンの大きさに変更
	CapsuleCollision->SetCapsuleHalfHeight(Height);

	//カメラの位置の変更
	float CameraLocation = FMath::Lerp(STAND_CAMERA_Z, CROUCH_CAMERA_Z, Value);

	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));

	UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_FloatToString(Height),
		true, true, FColor::Blue, 20.0, TEXT("None"));

	if (IsStand)
	{
		float VelocityZ = FMath::Lerp(CAPSULE_HALF_HEIGHT - CapsuleHalfHeight + 1,1.0f , Value);


		//立った時の地面への埋まりの修正
		CapsuleCollision->SetRelativeLocation((StandLocation + FVector(0.f, 0.f, VelocityZ)), false);
	}
}
	
//インタラクト
void ACPP_Player::Interact(const FInputActionValue& Value)
{
	FHitResult InteractHit;
	//カメラの中心からRayを飛ばす
	GetWorld()->LineTraceSingleByChannel(InteractHit, Camera->GetComponentLocation(),
		(Camera->GetComponentLocation() + Camera->GetForwardVector() * INTERACT_LENGTH),
		ECollisionChannel::ECC_Visibility, FCollisionQueryParams::DefaultQueryParam);

	if (InteractHit.bBlockingHit)
	{
		UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_ObjectToString(InteractHit.GetActor()),
			true, true, FColor::Blue, 20.0, TEXT("None"));
	}
}

//ズームの開始
void ACPP_Player::BeginZoom(const FInputActionValue& Value)
{
	//ZoomTimeLineが再生中なら停止する
	if (ZoomTimeLine->IsPlaying())
	{
		ZoomTimeLine->Stop();
	}

	//TimeLineの再生
	ZoomTimeLine->Play();
}

//ズームの終了
void ACPP_Player::EndZoom(const FInputActionValue& Value)
{
	//ZoomTimeLineが再生中なら停止する
	if (ZoomTimeLine->IsPlaying())
	{
		ZoomTimeLine->Stop();
	}

	//TimeLineの再生
	ZoomTimeLine->Reverse();
}

//ズーム
UFUNCTION()
void ACPP_Player::Zoom(const float& Value)
{
	float Lenght = FMath::Lerp(0.0f, ZOOM_SPRING_ARM_LENGHT, Value);

	SpringArm->TargetArmLength = Lenght;
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
