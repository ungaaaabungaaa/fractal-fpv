#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FractalCore.h"
#include "FPVPawn.generated.h"

class UCameraComponent;

UCLASS()
class FRACTALFPV_API AFPVPawn : public APawn
{
    GENERATED_BODY()
  public:
    AFPVPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent *Input) override;

    UPROPERTY(EditAnywhere, Category = "FPV", meta = (ClampMin = "0.1", ClampMax = "100"))
    float SpeedMetresPerSecond = 5.f;

  private:
    UPROPERTY(VisibleAnywhere, Category = "FPV")
    TObjectPtr<UCameraComponent> Camera;
    fractal::Camera State;
    fractal::Controls Controls;
    void ApplyPose();
    void Forward(float Value);
    void Right(float Value);
    void Up(float Value);
    void Turn(float Value);
    void Pitch(float Value);
    void MouseYaw(float Value);
    void MousePitch(float Value);
    void ResetView();
};
