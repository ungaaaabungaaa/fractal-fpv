using UnrealBuildTool;

public class FractalFPVEditorTarget : TargetRules
{
    public FractalFPVEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("FractalFPV");
    }
}
