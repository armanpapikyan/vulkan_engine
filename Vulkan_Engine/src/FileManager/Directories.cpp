#include "pch.h"
#include "Directories.h"
#include "Loaders/Model/ModelLoaderOptions.h"

Path Directories::getAbsolutePath(const std::string& path) { return getApplicationPath().combine(path); }

Path Directories::getApplicationPath()
{
	return applicationPath;
}

bool Directories::isValidWorkingDirectory(const Path& path) { return path.combine(workingSceneDir_relative).fileExists(); }

Path Directories::getWorkingDirectory() { return getAbsolutePath(workingSceneDir_relative); }

Path Directories::getShaderLibraryPath() { return getAbsolutePath(libraryShaderPath_relative); }

std::vector<Loader::ModelLoaderOptions> Directories::getModels_IntelSponza()
{
	return 
	{ 
		Loader::ModelLoaderOptions(getAbsolutePath(workingModel_relative), 1.0f),
		Loader::ModelLoaderOptions(getAbsolutePath(additiveModel_relative), 1.0f)
	};
}
std::vector<Loader::ModelLoaderOptions> Directories::getModels_DebrovicSponza()
{
	return 
	{ 
		Loader::ModelLoaderOptions(getAbsolutePath(DEBROVIC_SPONZA_OBJ), 1.0f)
	};
}
std::vector<Loader::ModelLoaderOptions> Directories::getModels_CrytekSponza()
{
	return 
	{ 
		Loader::ModelLoaderOptions(getAbsolutePath(CRYTEK_SPONZA_OBJ), 0.01f)
	};
}

bool Directories::isBinary(const Path& scenePath) { return scenePath.matchesExtension(sceneFileExtension); }

Path Directories::getBinaryTargetPath(const Path& modelPath)
{
	auto fileName = modelPath.getFileName(false) + sceneFileExtension;
	return getAbsolutePath(workingSceneDir_relative).combine(fileName);
}

bool Directories::tryGetBinaryIfExists(Path& binaryPath, const Path& modelPath)
{
	binaryPath = getBinaryTargetPath(modelPath);
	return binaryPath.fileExists();
}

Path Directories::syscall_GetApplicationPath()
{
	char pBuf[MAX_PATH]{};
	DWORD len = sizeof(pBuf);
	int bytes = GetModuleFileNameA(nullptr, pBuf, len);
	return bytes ? Path(pBuf) : Path();
}

ApplicationParameters::ApplicationParameters(int argc, char* argv[])
{
	char* commandLineInput = argc > 0 ? argv[0] : nullptr;

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-resources") == 0 && i + 1 < argc)
		{
			commandLineInput = argv[i + 1];

			i += 1;
		}
	}

	applicationDirectory = Path(getApplicationPathFromCommandLine(commandLineInput));
	printf("Application directory '%s'.\n", applicationDirectory.c_str());
}

std::string ApplicationParameters::getApplicationPathFromCommandLine(char commandLineArgument[])
{
	// 1. The command line argument overrides the default fullPath, if its provided by user input and is valid.
	auto appDirectory = Path(std::string(commandLineArgument));
	if (commandLineArgument != nullptr && Directories::isValidWorkingDirectory(appDirectory))
	{
		return Path(commandLineArgument).getFileDirectory();
	}

	// 2. Try to see if the resources exist in the application directory.
	appDirectory = Path(std::move(Directories::syscall_GetApplicationPath().getFileDirectory()));
	if (Directories::isValidWorkingDirectory(appDirectory))
		return appDirectory;

	// 3. Try to exit the path and find the resources in the source directory (step out of /$(Architecture)/$(Configuration)/).
	appDirectory = appDirectory.combine("../../");
	assert(Directories::isValidWorkingDirectory(appDirectory) && "The resources could not be found near the executable, or at solution directory. Please make sure the resources folder is placed beside the executable, or provide an override path to resources via command line argument.");
	return appDirectory;
}