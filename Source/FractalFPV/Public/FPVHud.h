#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPVHud.generated.h"

UCLASS()
class FRACTALFPV_API AFPVHud : public AHUD
{
    GENERATED_BODY()
  public:
    virtual void DrawHUD() override;
};
