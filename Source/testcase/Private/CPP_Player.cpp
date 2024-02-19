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

//�J�v�Z���R���W�����̔��a
#define CAPSULE_RADIUS 35.0f
//�J�v�Z���R���W�����̍���
#define CAPSULE_HALF_HEIGHT 90.0f

//�����ړ����x
#define WALK_SPEED 300.0f

//���Ⴊ�݂̈ړ����x
#define CROUCH_SPEED 50.0f

//�����Ă���Ƃ��̃J�����̍���
#define STAND_CAMERA_Z 60.0f

//���Ⴊ��ł���Ƃ��̃J�����̍���
#define CROUCH_CAMERA_Z 30.0f

// Sets default values
ACPP_Player::ACPP_Player()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//CapsuleComponent�̍쐬
	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
	//�R���W�����T�C�Y�ݒ�
	CapsuleCollision->InitCapsuleSize(CAPSULE_RADIUS, CAPSULE_HALF_HEIGHT);
	//�I�[�o�[���b�v�C�x���g�̗L����
	CapsuleCollision->SetGenerateOverlapEvents(true);
	//�R���W�����v���Z�b�g��Pawn�ɐݒ�
	CapsuleCollision->SetCollisionProfileName("Pawn");
	//�����𖳌���
	CapsuleCollision->SetSimulatePhysics(false);

	//RootComponent�ɐݒ�
	RootComponent = CapsuleCollision;

	// �J�����̍쐬
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	//RootComponent�ɃA�^�b�`
	Camera->SetupAttachment(RootComponent);
	//�����l�̐ݒ�
	Camera->SetRelativeLocation(FVector(0.f, 0.f, STAND_CAMERA_Z));
	//�R���g���[���[�̉�]���Q�Ƃ���
	Camera->bUsePawnControlRotation = true;

	//�A���[�R���|�[�l���g�쐬
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	//�V�[�����[�g�ɃA�^�b�`
	Arrow->SetupAttachment(Camera);
	//�v���C���[�̓���Ƀ��P�[�V������ݒ肷��
	Arrow->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	//�A���[���\���ɂ���
	Arrow->bHiddenInGame = true;

	/*Input Action�̓ǂݍ���*/
	MoveInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Move"), NULL, LOAD_None, NULL);
	CrouchInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Crouch"), NULL, LOAD_None, NULL);
	LookInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Look"), NULL, LOAD_None, NULL);
	
	/*Input Mapping Context�̓ǂݍ���*/
	ActionMappingContext = LoadObject<UInputMappingContext>
		(NULL, TEXT("/Game/TRAIN/Core/Input/IMC_Controll"), NULL, LOAD_None, NULL);


	FallVelocity = 0.0f;

	MoveVelocity = WALK_SPEED;


	// �J�[�u�A�Z�b�g�̎擾
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindCouchCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Crouch.CF_Crouch"));
	if (FindCouchCurve.Succeeded())
	{
		CrouchCurve = FindCouchCurve.Object;
	}

	//�^�C�����C���̍쐬
	CrouchTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeline"));
	if (CrouchTimeLine) 
	{
		// �^�C�����C���X�V���ɌĂяo����郁�\�b�h�̒�`
		FOnTimelineFloat CrouchStepFunc;
		CrouchStepFunc.BindUFunction(this, TEXT("Crouch"));
		CrouchTimeLine->AddInterpFloat(CrouchCurve, CrouchStepFunc);
	}

	//�^�C�����C���̍쐬
	StandTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("StandTimeline"));
	if (StandTimeLine)
	{
		// �^�C�����C���X�V���ɌĂяo����郁�\�b�h�̒�`
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

//��������
void ACPP_Player::Falling(const float& DeltaTime)
{
	//�����ʂ̉��Z
	FallVelocity += (GetWorld()->GetGravityZ() / 2) * DeltaTime;

	//�������̍��W
	FVector falled_location = GetActorLocation() + FVector(0.0f, 0.0f, FallVelocity);

	//�q�b�g���U���g
	FHitResult hit;

	//�ړ�
	SetActorLocation(falled_location, true, &hit, ETeleportType::None);

	if (hit.bBlockingHit) //�����Ƀq�b�g����
	{
		//�����ʂ̃��Z�b�g
		FallVelocity = 0.0f;
	}
}

//�ړ�����
void ACPP_Player::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MoveAxisVector = Value.Get<FVector2D>();

	UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_Vector2dToString(MoveAxisVector),
		true, true, FColor::Blue, 20.0, TEXT("None"));
	FVector MoveVector; //�ړ���

	//���E�ړ��̈ړ��ʂ̒ǉ�
	MoveVector = FVector(Camera->GetRightVector().X * MoveAxisVector.X * (MoveVelocity * FApp::GetDeltaTime()),
		Camera->GetRightVector().Y * MoveAxisVector.X * (MoveVelocity * FApp::GetDeltaTime()), 0.0f);

	//�O��ړ��̈ړ��ʂ̒ǉ�
	MoveVector += FVector(Camera->GetForwardVector().X * MoveAxisVector.Y * (MoveVelocity * FApp::GetDeltaTime()),
		Camera->GetForwardVector().Y * MoveAxisVector.Y * (MoveVelocity * FApp::GetDeltaTime()), 0.0f);

	//�q�b�g���U���g
	FHitResult hit;
	//�ړ�
	SetActorLocation((GetActorLocation() + MoveVector), true, &hit, ETeleportType::None);

	/*�⓹����鏈��*/
	if (hit.bBlockingHit) //�����Ƀq�b�g����
	{
		MoveVector = MoveVector - (hit.Normal * FVector::DotProduct(MoveVector, hit.Normal));
		SetActorLocation((GetActorLocation() + MoveVector), true, &hit, ETeleportType::None);
	}
	
}

//���Ⴊ�݊J�n
void ACPP_Player::BeginCrouch(const FInputActionValue& Value)
{
	//����TimeLine���Đ����Ȃ��~����
	if (StandTimeLine->IsPlaying())
	{
		StandTimeLine->Stop();
	}

	//���Ⴊ�ނƂ��̃J�v�Z���R���W�����̍���
	HalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();
	//���Ⴊ�ނƂ��̃J������Z���W
	CameraLocationZ = Camera->GetRelativeLocation().Z;

	//�^�C�����C���̍Đ�
	CrouchTimeLine->PlayFromStart();

	//�ړ����x�����Ⴊ�ݏ�Ԃ̈ړ����x�ɕύX
	MoveVelocity = CROUCH_SPEED;
}

//���Ⴊ�ݏI��
void ACPP_Player::EndCrouch(const FInputActionValue& Value)
{
	//���Ⴊ��TimeLine���Đ����Ȃ��~����
	if (CrouchTimeLine->IsPlaying())
	{
		CrouchTimeLine->Stop();
	}

	//���Ƃ��̍��W�̐ݒ�
	StandLocation = CapsuleCollision->GetRelativeLocation();
	//���Ƃ��̃J�v�Z���R���W�����̍���
	HalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();
	//���Ƃ��̃J������Z���W
	CameraLocationZ = Camera->GetRelativeLocation().Z;

	//�^�C�����C���̍Đ�
	StandTimeLine->PlayFromStart();
	
	//�ړ����x��ʏ��Ԃ̈ړ����x�ɕύX
	MoveVelocity = WALK_SPEED;
}

//���Ⴊ�ݏ�ԂɈڍs
void ACPP_Player::Crouch(const float& Value)
{
	float Height = FMath::Lerp(HalfHeight, (CAPSULE_HALF_HEIGHT / 2), Value);
	//�J�v�Z���R���W�����̑傫���ɕύX
	CapsuleCollision->SetCapsuleHalfHeight(HalfHeight);

	//�J�����̈ʒu�̕ύX
	float CameraLocation = FMath::Lerp(CameraLocationZ, CROUCH_CAMERA_Z, Value);

	Camera->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));
}

//����
void ACPP_Player::Stand(const float& Value)
{
	float Height = FMath::Lerp(HalfHeight, CAPSULE_HALF_HEIGHT, Value);
	//�J�v�Z���R���W���������Ƃ̑傫���ɕύX
	CapsuleCollision->SetCapsuleHalfHeight(Height);

	float CameraLocation = FMath::Lerp(CameraLocationZ, STAND_CAMERA_Z, Value);
	//�J�����̈ʒu�̕ύX
	Camera->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));

	float VelocityZ = FMath::Lerp(1.0f, CAPSULE_HALF_HEIGHT - HalfHeight + 1, Value);
	//���������̒n�ʂւ̖��܂�̏C��
	CapsuleCollision->SetRelativeLocation((StandLocation + FVector(0.f, 0.f, VelocityZ)), false);
}

//���_�ړ�
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
