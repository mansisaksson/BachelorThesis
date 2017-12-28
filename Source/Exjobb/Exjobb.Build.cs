// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Exjobb : ModuleRules
{
	public Exjobb(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "PhysX", "APEX", "Sockets", "Networking", "NVAPI" });
    }
}
