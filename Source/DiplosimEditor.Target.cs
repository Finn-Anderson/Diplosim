// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class DiplosimEditorTarget : TargetRules
{
	public DiplosimEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
        CppStandard = CppStandardVersion.Default;
        bValidateFormatStrings = true;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Diplosim");
	}
}
