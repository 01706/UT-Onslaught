namespace UnrealBuildTool.Rules
{
	public class UTOnslaught: ModuleRules
	{
		public UTOnslaught(TargetInfo Target)
        {
            PrivateIncludePaths.Add("UTOnslaught/Private");

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "UnrealTournament",
					"InputCore",
					"SlateCore",
                    "Json",
                    "JsonUtilities",
                }
				);
		}
	}
}