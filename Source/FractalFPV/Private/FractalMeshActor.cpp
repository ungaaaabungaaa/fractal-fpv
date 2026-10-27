#include "FractalMeshActor.h"
#include "FractalCore.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"

namespace
{
struct FNativeOrder
{
    bool bValid = false;
    bool bReverse = false;
};

const FNativeOrder &NativeOrder()
{
    // Ask the engine's own primitive generator once, rather than guess its index convention.
    static const FNativeOrder Result = []()
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FVector2D> UV;
        TArray<FProcMeshTangent> Tangents;
        UKismetProceduralMeshLibrary::GenerateBoxMesh(FVector(1), Vertices, Triangles, Normals, UV,
                                                      Tangents);
        if (Triangles.Num() < 3)
            return FNativeOrder{};
        for (int32 I = 0; I < 3; ++I)
            if (!Vertices.IsValidIndex(Triangles[I]) || !Normals.IsValidIndex(Triangles[I]))
                return FNativeOrder{};
        const FVector Cross =
            FVector::CrossProduct(Vertices[Triangles[1]] - Vertices[Triangles[0]],
                                  Vertices[Triangles[2]] - Vertices[Triangles[0]]);
        const double Sign = FVector::DotProduct(Cross, Normals[Triangles[0]]);
        if (!FMath::IsFinite(Sign) || FMath::Abs(Sign) < 1.e-6)
            return FNativeOrder{};
        return FNativeOrder{true, Sign < 0};
    }();
    return Result;
}
} // namespace

AFractalMeshActor::AFractalMeshActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FractalMesh"));
    SetRootComponent(Mesh);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
}

void AFractalMeshActor::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    Rebuild();
}

void AFractalMeshActor::Rebuild()
{
    Mesh->ClearAllMeshSections();
    const auto Generated = fractal::generate(Depth, SideMetres);
    if (!Generated.ok())
    {
        UE_LOG(LogTemp, Error, TEXT("Fractal input rejected: %s"),
               UTF8_TO_TCHAR(Generated.error.c_str()));
        return;
    }
    const auto &Order = NativeOrder();
    if (!Order.bValid)
    {
        UE_LOG(LogTemp, Error,
               TEXT("Native box winding sample is invalid; no mesh was submitted."));
        return;
    }
    const auto &Data = Generated.mesh;
    const auto Indices = fractal::indexOrder(Data, Order.bReverse);
    TArray<FVector> Vertices, Normals;
    TArray<int32> Triangles;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colours;
    TArray<FProcMeshTangent> Tangents;
    Vertices.Reserve(static_cast<int32>(Data.vertices.size()));
    Normals.Reserve(static_cast<int32>(Data.normals.size()));
    for (std::size_t I = 0; I < Data.vertices.size(); ++I)
    {
        const auto P = fractal::toCentimetres(Data.vertices[I]);
        const auto &N = Data.normals[I];
        Vertices.Add(FVector(P.x, P.y, P.z));
        Normals.Add(FVector(N.x, N.y, N.z));
        UV.Add(FVector2D(Data.uv[I].u, Data.uv[I].v));
        const auto FaceStart = (I / 4) * 4;
        const auto U = Data.vertices[FaceStart + 1] - Data.vertices[FaceStart];
        Tangents.Add(FProcMeshTangent(FVector(U.x, U.y, U.z).GetSafeNormal(), false));
    }
    for (const auto Index : Indices)
        Triangles.Add(static_cast<int32>(Index));
    Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colours, Tangents,
                                        false, false);
    Mesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
    UE_LOG(LogTemp, Display,
           TEXT("Fractal depth %d: %d cells, %d exposed faces; metre positions converted to cm. "
                "Collision disabled."),
           Depth, Data.retainedCells, Data.exposedFaces);
}
