#include "FPVGameMode.h"
#include "FPVPawn.h"
#include "FPVHud.h"
#include "FractalMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "Components/LightComponent.h"

AFPVGameMode::AFPVGameMode()
{
    DefaultPawnClass = AFPVPawn::StaticClass();
    HUDClass = AFPVHud::StaticClass();
}

void AFPVGameMode::BeginPlay()
{
    Super::BeginPlay();
    auto *World = GetWorld();
    if (!World)
        return;
    FActorSpawnParameters Parameters;
    World->SpawnActor<AFractalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    const FRotator Rotations[] = {FRotator(-45, -35, 0), FRotator(-30, 145, 0)};
    for (int32 I = 0; I < 2; ++I)
    {
        if (auto *Light =
                World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), Rotations[I], Parameters))
        {
            auto *Component = Light->GetLightComponent();
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetIntensity(I == 0 ? 3.f : 1.2f);
        }
    }
}
