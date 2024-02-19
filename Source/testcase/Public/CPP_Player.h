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

	//しゃがみ用のTimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* CrouchTimeLine;

	//立つ用のTimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* StandTimeLine;

	//しゃがみ用のCurve
	UPROPERTY(EditAnywhere, Category = "Reference|Curve")
	class UCurveFloat* CrouchCurve;

	//FallVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float FallVelocity;

	//MoveVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float MoveVelocity;

	//立つ瞬間の座標
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	FVector StandLocation;

	//カプセルの高さ
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float HalfHeight;

	//カメラのZ座標
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float CameraLocationZ;
private:
	//落下処理
	void Falling(const float& DeltaTime);

	//移動処理
	void Move(const FInputActionValue& Value);

	//しゃがみ開始
	void BeginCrouch(const FInputActionValue& Value);

	//しゃがみ終了
	void EndCrouch(const FInputActionValue& Value);

	//しゃがむ
	UFUNCTION()
	void Crouch(const float& Value);

	//立つ
	UFUNCTION()
	void Stand(const float& Value);

	//視点移動
	void Look(const FInputActionValue & Value);

};
