// Copyright 2018 Tin Rabzelj. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PerplexTarget : TargetRules
{
	public PerplexTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "Perplex" } );
	}
}
