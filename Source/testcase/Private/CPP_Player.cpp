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

//�C���^���N�g�\����
#define INTERACT_LENGTH 150

//�Y�[�����̃X�v�����O�A�[���̋���
#define ZOOM_SPRING_ARM_LENGHT -400.0

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


	//�X�v�����O�A�[���쐬
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	//�V�[�����[�g�ɃA�^�b�`
	SpringArm->SetupAttachment(RootComponent);
	//������ݒ�
	SpringArm->TargetArmLength = 0.0f;
	//�����l�̐ݒ�
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, STAND_CAMERA_Z));

	//�R���g���[���[�̉�]���Q�Ƃ���
	SpringArm->bUsePawnControlRotation = true;
	// �J�����̍쐬
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	//RootComponent�ɃA�^�b�`
	Camera->SetupAttachment(SpringArm);
	
	

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
	InteractInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Interact"), NULL, LOAD_None, NULL);
	ZoomInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Zoom"), NULL, LOAD_None, NULL);
	LookInput = LoadObject<UInputAction>
		(NULL, TEXT("/Game/TRAIN/Core/Input/Actions/IA_Look"), NULL, LOAD_None, NULL);

	
	/*Input Mapping Context�̓ǂݍ���*/
	ActionMappingContext = LoadObject<UInputMappingContext>
		(NULL, TEXT("/Game/TRAIN/Core/Input/IMC_Controll"), NULL, LOAD_None, NULL);

	/*���Ⴊ�ݏ����̃^�C�����C���̐ݒ�*/
	// �J�[�u�A�Z�b�g�̎擾
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindCouchCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Crouch.CF_Crouch"));
	if (FindCouchCurve.Succeeded())
	{
		CrouchCurve = FindCouchCurve.Object;
	}

	//�^�C�����C���̍쐬
	CrouchTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeLine"));
	if (CrouchTimeLine)
	{
		// �^�C�����C���X�V���ɌĂяo����郁�\�b�h�̒�`
		FOnTimelineFloat CrouchStepFunc;
		CrouchStepFunc.BindUFunction(this, TEXT("Crouch"));
		CrouchTimeLine->AddInterpFloat(CrouchCurve, CrouchStepFunc);
	}

	/*�Y�[�������̃^�C�����C���̐ݒ�*/
	// �J�[�u�A�Z�b�g�̎擾
	const ConstructorHelpers::FObjectFinder<UCurveFloat> FindZoomCurve(TEXT("/Game/TRAIN/Core/Characters/CF_Zoom.CF_Zoom"));
	if (FindCouchCurve.Succeeded())
	{
		ZoomCurve = FindZoomCurve.Object;
	}

	//�^�C�����C���̍쐬
	ZoomTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("ZoomTimeLine"));
	if (ZoomTimeLine)
	{
		// �^�C�����C���X�V���ɌĂяo����郁�\�b�h�̒�`
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
	if (CrouchTimeLine->IsPlaying())
	{
		CrouchTimeLine->Stop();
	}

	//���Ⴊ�ݏ�Ԃɐݒ�
	IsStand = false;

	//�^�C�����C���̍Đ�
	CrouchTimeLine->Play();

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

	//����Ԃɐݒ�
	IsStand = true;

	//�J�v�Z���̍�����ݒ�
	CapsuleHalfHeight = CapsuleCollision->GetScaledCapsuleHalfHeight();

	//�^�C�����C���̍Đ�
	CrouchTimeLine->Reverse();
	
	//�ړ����x��ʏ��Ԃ̈ړ����x�ɕύX
	MoveVelocity = WALK_SPEED;
}

//���Ⴊ��
void ACPP_Player::Crouch(const float& Value)
{
	float Height = FMath::Lerp(CAPSULE_HALF_HEIGHT, (CAPSULE_HALF_HEIGHT / 2), Value);
	//�J�v�Z���R���W�����̑傫���ɕύX
	CapsuleCollision->SetCapsuleHalfHeight(Height);

	//�J�����̈ʒu�̕ύX
	float CameraLocation = FMath::Lerp(STAND_CAMERA_Z, CROUCH_CAMERA_Z, Value);

	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, CameraLocation));

	UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_FloatToString(Height),
		true, true, FColor::Blue, 20.0, TEXT("None"));

	if (IsStand)
	{
		float VelocityZ = FMath::Lerp(CAPSULE_HALF_HEIGHT - CapsuleHalfHeight + 1,1.0f , Value);


		//���������̒n�ʂւ̖��܂�̏C��
		CapsuleCollision->SetRelativeLocation((StandLocation + FVector(0.f, 0.f, VelocityZ)), false);
	}
}
	
//�C���^���N�g
void ACPP_Player::Interact(const FInputActionValue& Value)
{
	FHitResult InteractHit;
	//�J�����̒��S����Ray���΂�
	GetWorld()->LineTraceSingleByChannel(InteractHit, Camera->GetComponentLocation(),
		(Camera->GetComponentLocation() + Camera->GetForwardVector() * INTERACT_LENGTH),
		ECollisionChannel::ECC_Visibility, FCollisionQueryParams::DefaultQueryParam);

	if (InteractHit.bBlockingHit)
	{
		UKismetSystemLibrary::PrintString(this, UKismetStringLibrary::Conv_ObjectToString(InteractHit.GetActor()),
			true, true, FColor::Blue, 20.0, TEXT("None"));
	}
}

//�Y�[���̊J�n
void ACPP_Player::BeginZoom(const FInputActionValue& Value)
{
	//ZoomTimeLine���Đ����Ȃ��~����
	if (ZoomTimeLine->IsPlaying())
	{
		ZoomTimeLine->Stop();
	}

	//TimeLine�̍Đ�
	ZoomTimeLine->Play();
}

//�Y�[���̏I��
void ACPP_Player::EndZoom(const FInputActionValue& Value)
{
	//ZoomTimeLine���Đ����Ȃ��~����
	if (ZoomTimeLine->IsPlaying())
	{
		ZoomTimeLine->Stop();
	}

	//TimeLine�̍Đ�
	ZoomTimeLine->Reverse();
}

//�Y�[��
UFUNCTION()
void ACPP_Player::Zoom(const float& Value)
{
	float Lenght = FMath::Lerp(0.0f, ZOOM_SPRING_ARM_LENGHT, Value);

	SpringArm->TargetArmLength = Lenght;
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
