#include <vk_engine.h>
#include "FileManager/Directories.h"

int main(int argc, char* argv[])
{
	ApplicationParameters parameters(argc, argv);
	VulkanEngine engine(parameters.applicationDirectory);

	// By default disabled
	bool validationLayers = false;

#ifndef NDEBUG
	// Override if debug build
	validationLayers = true;
#endif
	// todo - override validation layers value on command line argument

	engine.init(validationLayers);

	engine.run();

	engine.cleanup();

	return 0;
}
