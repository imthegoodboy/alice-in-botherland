// Alice in Botherland

using UnrealBuildTool;
using System.Collections.Generic;

public class AliceInBotherlandEditorTarget : TargetRules
{
	public AliceInBotherlandEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("AliceInBotherland");
	}
}
