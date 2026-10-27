#include "FPVHud.h"

void AFPVHud::DrawHUD()
{
    Super::DrawHUD();
    DrawText(TEXT("FRACTAL FPV / KINEMATIC CAMERA"), FLinearColor::White, 20, 20);
    DrawText(TEXT("WASD move | Space / Ctrl vertical | Mouse / arrows look | R reset"),
             FLinearColor::White, 20, 44);
    DrawText(TEXT("Collision OFF. Core metres are converted to Unreal centimetres."),
             FLinearColor::White, 20, 68);
}
