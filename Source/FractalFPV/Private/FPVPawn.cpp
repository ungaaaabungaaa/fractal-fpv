#include "FPVPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"

AFPVPawn::AFPVPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    auto *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Root);
    Camera->SetFieldOfView(70.f);
    AutoPossessPlayer = EAutoReceiveInput::Player0;
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    SetActorEnableCollision(false);
}

void AFPVPawn::BeginPlay()
{
    Super::BeginPlay();
    ResetView(); // Overrides an origin PlayerStart with a known outside camera.
}

void AFPVPawn::ApplyPose()
{
    const auto P = fractal::toCentimetres(State.position);
    SetActorLocationAndRotation(FVector(P.x, P.y, P.z), FRotator(State.pitchDeg, State.yawDeg, 0));
}

void AFPVPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto Next = State;
    Next.speedMps = SpeedMetresPerSecond;
    std::string Error;
    // Long frame hitches drop excess simulation time instead of causing a large jump.
    if (fractal::step(Next, Controls, FMath::Clamp(static_cast<double>(DeltaSeconds), 0.0, 0.1),
                      Error))
    {
        State = Next;
        ApplyPose();
    }
}

void AFPVPawn::SetupPlayerInputComponent(UInputComponent *Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("Forward"), this, &AFPVPawn::Forward);
    Input->BindAxis(TEXT("Right"), this, &AFPVPawn::Right);
    Input->BindAxis(TEXT("Up"), this, &AFPVPawn::Up);
    Input->BindAxis(TEXT("Turn"), this, &AFPVPawn::Turn);
    Input->BindAxis(TEXT("Pitch"), this, &AFPVPawn::Pitch);
    Input->BindAxis(TEXT("MouseYaw"), this, &AFPVPawn::MouseYaw);
    Input->BindAxis(TEXT("MousePitch"), this, &AFPVPawn::MousePitch);
    Input->BindAction(TEXT("Reset"), IE_Pressed, this, &AFPVPawn::ResetView);
    if (auto *Player = Cast<APlayerController>(GetController()))
    {
        Player->bShowMouseCursor = false;
        Player->SetInputMode(FInputModeGameOnly());
    }
}

void AFPVPawn::Forward(float Value) { Controls.forward = FMath::Clamp(Value, -1.f, 1.f); }
void AFPVPawn::Right(float Value) { Controls.right = FMath::Clamp(Value, -1.f, 1.f); }
void AFPVPawn::Up(float Value) { Controls.up = FMath::Clamp(Value, -1.f, 1.f); }
void AFPVPawn::Turn(float Value) { Controls.yaw = FMath::Clamp(Value, -1.f, 1.f); }
void AFPVPawn::Pitch(float Value) { Controls.pitch = FMath::Clamp(Value, -1.f, 1.f); }
void AFPVPawn::MouseYaw(float Value)
{
    std::string Error;
    if (fractal::look(State, FMath::Clamp(static_cast<double>(Value) * .15, -180.0, 180.0), 0,
                      Error))
        ApplyPose();
}
void AFPVPawn::MousePitch(float Value)
{
    std::string Error;
    if (fractal::look(State, 0, FMath::Clamp(static_cast<double>(Value) * .15, -180.0, 180.0),
                      Error))
        ApplyPose();
}
void AFPVPawn::ResetView()
{
    State = fractal::Camera{};
    Controls = fractal::Controls{};
    ApplyPose();
}
