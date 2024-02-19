// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CPP_Player.generated.h"

class UInputComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

UCLASS()
class TESTCASE_API ACPP_Player : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACPP_Player();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	//CapsuleCollision
	UPROPERTY(VisibleAnywhere, Category = "Reference|Components")
	class UCapsuleComponent* CapsuleCollision;

	//Camera
	UPROPERTY(VisibleAnywhere, Category = "Components/Camera")
	class UCameraComponent* Camera;

	//Arrow
	UPROPERTY(VisibleAnywhere, Category = Control, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* Arrow;

	//MoveInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* MoveInput;

	//CrouchInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* CrouchInput;

	//LookInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* LookInput;

	//MappingContext
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Reference|Input")
	class UInputMappingContext* ActionMappingContext;

	//���Ⴊ�ݗp��TimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* CrouchTimeLine;

	//���p��TimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* StandTimeLine;

	//���Ⴊ�ݗp��Curve
	UPROPERTY(EditAnywhere, Category = "Reference|Curve")
	class UCurveFloat* CrouchCurve;

	//FallVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float FallVelocity;

	//MoveVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float MoveVelocity;

	//���u�Ԃ̍��W
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	FVector StandLocation;

	//�J�v�Z���̍���
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float HalfHeight;

	//�J������Z���W
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float CameraLocationZ;
private:
	//��������
	void Falling(const float& DeltaTime);

	//�ړ�����
	void Move(const FInputActionValue& Value);

	//���Ⴊ�݊J�n
	void BeginCrouch(const FInputActionValue& Value);

	//���Ⴊ�ݏI��
	void EndCrouch(const FInputActionValue& Value);

	//���Ⴊ��
	UFUNCTION()
	void Crouch(const float& Value);

	//����
	UFUNCTION()
	void Stand(const float& Value);

	//���_�ړ�
	void Look(const FInputActionValue & Value);

};
