#pragma once
#include "pch.h"
#include "Path.h"

namespace Loader
{
	struct ModelLoaderOptions;
}

class Directories
{
public:
	inline static Path applicationPath;
	static Path getApplicationPath();
	static Path getAbsolutePath(const std::string& path);
	
	static bool isValidWorkingDirectory(const Path& path);
	static Path getWorkingDirectory();

	static std::vector<Loader::ModelLoaderOptions> getModels_IntelSponza();
	static std::vector<Loader::ModelLoaderOptions> getModels_DebrovicSponza();
	static std::vector<Loader::ModelLoaderOptions> getModels_CrytekSponza();

	static Path getShaderLibraryPath();

	static bool isBinary(const Path& scenePath);
	static Path getBinaryTargetPath(const Path& modelPath);
	static bool tryGetBinaryIfExists(Path& binaryPath, const Path& modelPath);

	static Path syscall_GetApplicationPath();

private:
	inline static std::string sceneFileExtension = ".binary";
	inline static std::string library_relative = "Resources/Library/";
	inline static std::string libraryShaderPath_relative = "Resources/Library/outputSPV/";

	inline static std::string CUBE_GLTF = "Resources/Other/gltf/Cube_khr.gltf";

	inline static std::string CRYTEK_SPONZA_OBJ = "Resources/classic_sponza/crytekSponza.obj";
	inline static std::string CRYTEK_SPONZA_FBX = "Resources/classic_sponza/crytekSponza.fbx";
	inline static std::string DEBROVIC_SPONZA_OBJ = "Resources/classic_sponza/debrovic_sponza/debrovicSponza.obj";

	inline static std::string INTEL_SPONZA_FBX = "Resources/intel_sponza/NewSponza_Main_Yup_002.fbx";
	inline static std::string INTEL_SPONZA_GLTF = "Resources/intel_sponza/NewSponza_Main_glTF_002.gltf";
	inline static std::string INTEL_SPONZA_CURTAINS_GLTF = "Resources/intel_sponza/NewSponza_Curtains_glTF.gltf";

	inline static std::string workingSceneName = "Scene.binary";
	inline static std::string workingSceneDir_relative = "Resources/Serialized/";
	inline static std::string workingModel_relative = INTEL_SPONZA_GLTF;
	inline static std::string additiveModel_relative = INTEL_SPONZA_CURTAINS_GLTF;
};

struct ApplicationParameters
{
	Path applicationPath;
	Path applicationDirectory;

	ApplicationParameters(int argc, char* argv[]);

	std::string getApplicationPathFromCommandLine(char commandLineArgument[]);
};
