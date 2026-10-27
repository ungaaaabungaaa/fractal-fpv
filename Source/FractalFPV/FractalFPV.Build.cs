using UnrealBuildTool;
using System.IO;

public class FractalFPV : ModuleRules
{
    public FractalFPV(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent" });
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "FractalCore"));
    }
}
