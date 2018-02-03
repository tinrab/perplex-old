// Copyright 2018 Tin Rabzelj. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PerplexEditorTarget : TargetRules
{
	public PerplexEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "Perplex" } );
	}
}
