#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPVGameMode.generated.h"

UCLASS()
class FRACTALFPV_API AFPVGameMode : public AGameModeBase
{
    GENERATED_BODY()
  public:
    AFPVGameMode();
    virtual void BeginPlay() override;
};
