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

	UPROPERTY(VisibleAnywhere, Category = "Reference|Components")
	class USpringArmComponent* SpringArm;

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

	//InteractInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* InteractInput;

	//ZoomInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* ZoomInput;

	//LookInput
	UPROPERTY(EditAnywhere, Category = "Reference|Input")
	class UInputAction* LookInput;

	//MappingContext
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Reference|Input")
	class UInputMappingContext* ActionMappingContext;

	//しゃがみ用のTimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* CrouchTimeLine;

	//しゃがみ用のCurve
	UPROPERTY(EditAnywhere, Category = "Reference|Curve")
	class UCurveFloat* CrouchCurve;

	//Zoom用のTimeLine
	UPROPERTY(EditAnywhere, Category = "Reference|TimeLine")
	class UTimelineComponent* ZoomTimeLine;

	//Zoom用のCurve
	UPROPERTY(EditAnywhere, Category = "Reference|Curve")
	class UCurveFloat* ZoomCurve;

	//FallVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float FallVelocity;

	//MoveVelocity
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float MoveVelocity;

	//立つ瞬間の座標
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	FVector StandLocation;

	//立つ状態か
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	bool IsStand;

	//カプセルの高さ
	UPROPERTY(EditAnywhere, Category = "Reference|Variable")
	float CapsuleHalfHeight;
private:
	//落下処理
	void Falling(const float& DeltaTime);

	//移動処理
	void Move(const FInputActionValue& Value);

	//しゃがみ開始
	void BeginCrouch(const FInputActionValue& Value);

	//しゃがみ終了
	void EndCrouch(const FInputActionValue& Value);

	//しゃがみ
	UFUNCTION()
	void Crouch(const float& Value);

	//インタラクト
	void Interact(const FInputActionValue& Value);

	//ズームの開始
	void BeginZoom(const FInputActionValue& Value);

	//ズームの終了
	void EndZoom(const FInputActionValue& Value);

	//ズーム
	UFUNCTION()
	void Zoom(const float& Value);


	//視点移動
	void Look(const FInputActionValue & Value);

};
