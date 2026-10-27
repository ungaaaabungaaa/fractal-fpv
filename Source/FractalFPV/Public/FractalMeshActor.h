#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FractalMeshActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class FRACTALFPV_API AFractalMeshActor : public AActor
{
    GENERATED_BODY()
  public:
    AFractalMeshActor();
    virtual void OnConstruction(const FTransform &Transform) override;
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Fractal")
    void Rebuild();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fractal",
              meta = (ClampMin = "0", ClampMax = "3"))
    int32 Depth = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fractal",
              meta = (ClampMin = "1", ClampMax = "100", Units = "m"))
    float SideMetres = 20.f;

  private:
    UPROPERTY(VisibleAnywhere, Category = "Fractal")
    TObjectPtr<UProceduralMeshComponent> Mesh;
};
