#include "ProjectInfo.hpp"
#include "Debug/Exception.hpp"
#include "Debug/Logger.hpp"
#include "Graphics/GraphicsPipeline.hpp"
#include "Particles/Particle.hpp"
#include "Particles/ParticleSystem.hpp"
#include "Platform/Window.hpp"
#include "Simulation/BarnesHut/BarnesHutSimulation.hpp"
#include "Simulation/Direct/DirectSimulation.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "Vulkan/VulkanInstance.hpp"
#include "Vulkan/VulkanSurface.hpp"
#include "Vulkan/VulkanSwapChain.hpp"

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <exception>

const char* const ARGS_HELP = 
	"%s, version %u.%u.%u\n"
	"Available parameters:\n"
	"\t--log-file: The file to output all logs to. If unspecified, logs will not be outputted to a file.\n"
	"\t--particles-in: The input file containing the starting variables of the particles. If not specified, the program will use the given generation parameters.\n"
	"\t--particles-out: The optional output file in which the variables of the particles will be written once the simulation is finished.\n"
	"\t--particle-count: The number of particles to generate. All generation parameters are ignored if an input file is specified.\n"
	"\t--generate-type: The variant to use for the particle system generation. One of the following options:\n"
	"\t\trandom: Randomly distribures particles within the generation confines.\n"
	"\t\tgalaxy: Simulates the approximate structure of a spiral galaxy spanning the generation confines.\n"
	"\t\tgalaxy-collision: Simulates the collision of two sipral galaxies, each spanning a third of the generation confines.\n"
	"\t\tsymmetrical-galaxy-collision: Simulates the collision of two symmetrical spiral galaxies, each spanning a third of the generation confines.\n"
	"\t--generate-size: The radius of the resulting generation's size.\n"
	"\t--min-mass: The minimum mass of the generated particles.\n"
	"\t--max-mass: The maximum mass of the generated particles.\n"
	"\t--gravitational-const: The gravitational constant used for the simulation. Defaulted to 1.\n"
	"\t--simulation-time: The time interval length, in seconds, simulated in one instance. Defaulted to 1e-3.\n"
	"\t--simulation-speed: The speed factor at which the simulation is run. Defaulted to 1.\n"
	"\t--softening-len: The softening length used to soften the extreme forces that would usually result from close interactions. Defaulted to 0.2.\n"
	"\t--accuracy-parameter: The accuracy parameter used to calibrate force approximation. Only used for Barnes-Hut simulations. Defaulted to 1.\n"
	"\t--simulation-algorithm: The simulation algorithm used to calculate the gravitational forces. One of the following options:\n"
	"\t\tdirect-sum: The direct-sum method, calculating every interaction between particles.\n"
	"\t\tbarnes-hut: The Barnes-Hut algorithm, organizing all particles in a quadtree.\n"
	"\t--simulation-count: The number of simulations to run before closing the program. No limit will be used if this parameter isn't specified.\n"
	"Available options:\n"
	"\t--help: Displays the current message and exits the program.\n"
	"\t--log-detailed: Outputs non-crucial logs that might be useful for debugging or additional information.\n"
	"\t--no-graphics: Doesn't display the live positions of all particles, instead running the simulations in the background.\n"
	"\t--benchmark: Benchmarks the required runtime for all simulations. Ignored if --no-graphics isn't specified.\n";

struct ProgramInfo {
	const char* logFile = nullptr;
	const char* particlesInFile = nullptr;
	const char* particlesOutFile = nullptr;
	uint32_t particleCount = 0;
	gsim::ParticleSystem::GenerateType generateType = gsim::ParticleSystem::GENERATE_TYPE_COUNT;
	float generateSize = 0.0f;
	float minMass = 0.0f;
	float maxMass = 0.0f;
	float gravitationalConst = 1.0f;
	float simulationTime = 0.001f;
	float simulationSpeed = 1.0f;
	float softeningLen = 0.2f;
	float accuracyParameter = 1.0f;
	gsim::ParticleSystem::SimulationAlgorithm simulationAlgorithm = gsim::ParticleSystem::SIMULATION_ALGORITHM_COUNT;
	uint64_t maxSimulationCount = UINT64_MAX;

	bool logDetailed = false;
	bool noGraphics = false;
	bool benchmark = false;

	gsim::Logger* logger;
	gsim::Window* window;
	gsim::VulkanInstance* instance;
	gsim::VulkanSurface* surface;
	gsim::VulkanDevice* device;
	gsim::VulkanSwapChain* swapChain;
	gsim::ParticleSystem* particleSystem;
	gsim::GraphicsPipeline* graphicsPipeline;

	gsim::DirectSimulation* directSim = nullptr;
	gsim::BarnesHutSimulation* barnesHutSim = nullptr;

	gsim::Vec2 cameraPos;
	float cameraSize;
	float cameraZoom = 1.0f;
	gsim::Window::MousePos mousePos;

	clock_t clockStart;
	uint64_t simulationCount = 0;
	uint64_t targetSimulationCount = 0;
};

static void WindowDrawCallback(void* userData, void* args) {
	// Get the program info
	ProgramInfo* programInfo = (ProgramInfo*)userData;

	// Run the simulations
	if(programInfo->directSim) {
		programInfo->directSim->RunSimulations((uint32_t)(programInfo->targetSimulationCount - programInfo->simulationCount));
	} else {
		programInfo->barnesHutSim->RunSimulations((uint32_t)(programInfo->targetSimulationCount - programInfo->simulationCount));
	}
	programInfo->simulationCount = programInfo->targetSimulationCount;

	// Close the window and exit the function if all required simulations were run
	if(programInfo->simulationCount == programInfo->maxSimulationCount) {
		programInfo->window->CloseWindow();
		return;
	}

	// Calculate the screen's new position
	float aspectRatio = (float)programInfo->window->GetWindowInfo().width / programInfo->window->GetWindowInfo().height;
	gsim::Window::MousePos newMousePos = programInfo->window->GetMousePos();

	if(programInfo->window->IsMouseDown()) {
		// Calculate the mouse's relative pos
		gsim::Window::MousePos relativePos { newMousePos.x - programInfo->mousePos.x, newMousePos.y - programInfo->mousePos.y };

		// Calculate the relative world pos
		gsim::Vec2 relativeWorldPos { (float)relativePos.x / programInfo->window->GetWindowInfo().width * 2, -((float)relativePos.y / programInfo->window->GetWindowInfo().height * 2) };
		relativeWorldPos.x *= programInfo->cameraSize * aspectRatio / programInfo->cameraZoom;
		relativeWorldPos.y *= programInfo->cameraSize / programInfo->cameraZoom;

		// Set the new camera pos
		programInfo->cameraPos.x -= relativeWorldPos.x;
		programInfo->cameraPos.y -= relativeWorldPos.y;
	}

	// Set the new mouse pos
	programInfo->mousePos = newMousePos;

	// Render the particles
	programInfo->graphicsPipeline->RenderParticles(programInfo->cameraPos, { programInfo->cameraSize * aspectRatio / programInfo->cameraZoom, programInfo->cameraSize / programInfo->cameraZoom });

	// Store the clock end and calculate the target simulation count
	programInfo->targetSimulationCount = (uint64_t)((float)(clock() - programInfo->clockStart) / CLOCKS_PER_SEC / programInfo->simulationTime);
	if(programInfo->targetSimulationCount > programInfo->maxSimulationCount)
		programInfo->targetSimulationCount = programInfo->maxSimulationCount;
}
static void WindowKeyCallback(void* userData, void* args) {
	// Get the event's info
	gsim::Window::KeyEventInfo eventInfo = *(gsim::Window::KeyEventInfo*)args;

	// Get the program info
	ProgramInfo* programInfo = (ProgramInfo*)userData;

	// Check if the camera will be zoomed in or out
	if(eventInfo.key == '+') {
		programInfo->cameraZoom += 0.1f * eventInfo.repeatCount;
	} else if(eventInfo.key = '-') {
		programInfo->cameraZoom -= 0.1f * eventInfo.repeatCount;
		if(programInfo->cameraZoom < 0.2f)
			programInfo->cameraZoom = 0.2f;
	}
}

int main(int argc, char** args) {
	// Check if only the help message is requested
	if(argc == 2 && !strcmp(args[1], "--help")) {
		// Print the help string and exit the program
		printf(ARGS_HELP, GSIM_PROJECT_NAME, GSIM_PROJECT_VERSION_MAJOR, GSIM_PROJECT_VERSION_MINOR, GSIM_PROJECT_VERSION_PATCH);
	}

	// Parse all console args
	ProgramInfo programInfo;

	for(int32_t i = 1; i != argc; ++i) {
		if(!strncmp(args[i], "--log-file=", 11)) {
			programInfo.logFile = args[i] + 11;
		} else if(!strncmp(args[i], "--particles-in=", 15)) {
			programInfo.particlesInFile = args[i] + 15;
		} else if(!strncmp(args[i], "--particles-out=", 16)) {
			programInfo.particlesOutFile = args[i] + 16;
		} else if(!strncmp(args[i], "--particle-count=", 17)) {
			programInfo.particleCount = (uint32_t)strtoull(args[i] + 17, nullptr, 10);
		} else if(!strncmp(args[i], "--generate-type=", 16)) {
			if(!strcmp(args[i] + 16, "random")) {
				programInfo.generateType = gsim::ParticleSystem::GENERATE_TYPE_RANDOM;
			} else if(!strcmp(args[i] + 16, "galaxy")) {
				programInfo.generateType = gsim::ParticleSystem::GENERATE_TYPE_GALAXY;
			} else if(!strcmp(args[i] + 16, "galaxy-collision")) {
				programInfo.generateType = gsim::ParticleSystem::GENERATE_TYPE_GALAXY_COLLISION;
			} else if(!strcmp(args[i] + 16, "symmetrical-galaxy-collision")) {
				programInfo.generateType = gsim::ParticleSystem::GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION;
			}
		} else if(!strncmp(args[i], "--generate-size=", 16)) {
			programInfo.generateSize = strtof(args[i] + 16, nullptr);
		} else if(!strncmp(args[i], "--min-mass=", 11)) {
			programInfo.minMass = strtof(args[i] + 11, nullptr);
		} else if(!strncmp(args[i], "--max-mass=", 11)) {
			programInfo.maxMass = strtof(args[i] + 11, nullptr);
		} else if(!strncmp(args[i], "--gravitational-const=", 22)) {
			programInfo.gravitationalConst = strtof(args[i] + 22, nullptr);
		} else if(!strncmp(args[i], "--simulation-time=", 18)) {
			programInfo.simulationTime = strtof(args[i] + 18, nullptr);
		} else if(!strncmp(args[i], "--simulation-speed=", 19)) {
			programInfo.simulationSpeed = strtof(args[i] + 19, nullptr);
		} else if(!strncmp(args[i], "--softening-len=", 16)) {
			programInfo.softeningLen = strtof(args[i] + 16, nullptr);
		} else if(!strncmp(args[i], "--accuracy-parameter=", 21)) {
			programInfo.accuracyParameter = strtof(args[i] + 21, nullptr);
		} else if(!strncmp(args[i], "--simulation-algorithm=", 23)) {
			if(!strcmp(args[i] + 23, "direct-sum")) {
				programInfo.simulationAlgorithm = gsim::ParticleSystem::SIMULATION_ALGORITHM_DIRECT_SUM;
			} else if(!strcmp(args[i] + 23, "barnes-hut")) {
				programInfo.simulationAlgorithm = gsim::ParticleSystem::SIMULATION_ALGORITHM_BARNES_HUT;
			}
		} else if(!strncmp(args[i], "--simulation-count=", 19)) {
			programInfo.maxSimulationCount = (uint64_t)strtoull(args[i] + 19, nullptr, 10);
		} else if(!strcmp(args[i], "--log-detailed")) {
			programInfo.logDetailed = true;
		} else if(!strcmp(args[i], "--no-graphics")) {
			programInfo.noGraphics = true;
		} else if(!strcmp(args[i], "--benchmark")) {
			programInfo.benchmark = true;
		}
	}

	// Create the logger
	gsim::Logger::MessageLevelFlags messageLevelFlags = programInfo.logDetailed ? gsim::Logger::MESSAGE_LEVEL_ALL : gsim::Logger::MESSAGE_LEVEL_ESSENTIAL;
	programInfo.logger = new gsim::Logger(programInfo.logFile, messageLevelFlags);

	// Check if the given args are valid
	if(!programInfo.particlesInFile && !(programInfo.particleCount && programInfo.generateType != gsim::ParticleSystem::GENERATE_TYPE_COUNT && programInfo.generateSize && programInfo.minMass && programInfo.maxMass)) {
		programInfo.logger->LogMessage(gsim::Logger::MESSAGE_LEVEL_FATAL_ERROR, "If a particle input file is not provided, valid generation args must be provided!");
	}
	if(programInfo.particlesInFile && (programInfo.particleCount || programInfo.generateType != gsim::ParticleSystem::GENERATE_TYPE_COUNT || programInfo.generateSize || programInfo.minMass || programInfo.maxMass)) {
		programInfo.logger->LogMessage(gsim::Logger::MESSAGE_LEVEL_WARNING, "A particle input file was provided, therefore the given generation args will be ignored.");
	}
	if(programInfo.simulationAlgorithm == gsim::ParticleSystem::SIMULATION_ALGORITHM_COUNT) {
		programInfo.logger->LogMessage(gsim::Logger::MESSAGE_LEVEL_FATAL_ERROR, "A valid simulation algorithm must be given!");
	}
	if(programInfo.noGraphics && programInfo.simulationCount == UINT64_MAX) {
		programInfo.logger->LogMessage(gsim::Logger::MESSAGE_LEVEL_FATAL_ERROR, "If --no-graphics was specified, a valid simulation count must be given!");
	}
	if(!programInfo.noGraphics && programInfo.benchmark) {
		programInfo.logger->LogMessage(gsim::Logger::MESSAGE_LEVEL_WARNING, "The --benchmark option will be ignored, as --no-graphics wasn't specified.");
	}

	// Catch any exceptions thrown by the rest of the program
	try {
		if(programInfo.noGraphics) {
			// Create the Vulkan components
			programInfo.instance = new gsim::VulkanInstance(true, programInfo.logger);
			programInfo.device = new gsim::VulkanDevice(programInfo.instance, nullptr);

			// Log info about the Vulkan device
			programInfo.device->LogDeviceInfo(programInfo.logger);

			// Create the particle system
			if(programInfo.particlesInFile) {
				programInfo.particleSystem = new gsim::ParticleSystem(programInfo.device, programInfo.particlesInFile, programInfo.gravitationalConst, programInfo.simulationTime, programInfo.simulationSpeed, programInfo.softeningLen, programInfo.accuracyParameter, programInfo.simulationAlgorithm);
			} else {
				programInfo.particleSystem = new gsim::ParticleSystem(programInfo.device, programInfo.particleCount, programInfo.generateType, programInfo.generateSize, programInfo.minMass, programInfo.maxMass, programInfo.gravitationalConst, programInfo.simulationTime, programInfo.simulationSpeed, programInfo.softeningLen, programInfo.accuracyParameter, programInfo.simulationAlgorithm);
			}

			// Create the simulation
			if(programInfo.simulationAlgorithm == gsim::ParticleSystem::SIMULATION_ALGORITHM_DIRECT_SUM) {
				programInfo.directSim = new gsim::DirectSimulation(programInfo.device, programInfo.particleSystem);
			} else {
				programInfo.barnesHutSim = new gsim::BarnesHutSimulation(programInfo.device, programInfo.particleSystem);
			}

			// Store the clock start, for benchmarking
			programInfo.clockStart = clock();
			programInfo.simulationCount = 0;
			programInfo.targetSimulationCount = 0;

			// Run all the simulations
			while(programInfo.simulationCount != programInfo.maxSimulationCount) {
				// Run the simulations
				if(programInfo.directSim) {
					programInfo.directSim->RunSimulations((uint32_t)(programInfo.targetSimulationCount - programInfo.simulationCount));
				} else {
					programInfo.barnesHutSim->RunSimulations((uint32_t)(programInfo.targetSimulationCount - programInfo.simulationCount));
				}
				programInfo.simulationCount = programInfo.targetSimulationCount;

				// Set the target simulation count
				programInfo.targetSimulationCount = programInfo.simulationCount + 100;
				if(programInfo.targetSimulationCount > programInfo.maxSimulationCount)
					programInfo.targetSimulationCount = programInfo.maxSimulationCount;
			}

			// Wait for the device to idle
			vkDeviceWaitIdle(programInfo.device->GetDevice());

			// Output the benchmark info, if requested
			if(programInfo.benchmark) {
				// Calculate the total and average runtimes
				clock_t clockDif = clock() - programInfo.clockStart;
				float runtimeSec = (float)clockDif / CLOCKS_PER_SEC;
				float runtimeAvgMs = runtimeSec * 1000 / programInfo.maxSimulationCount;

				// Log the total runtime
				if(runtimeSec >= 1) {
					programInfo.logger->LogMessageForced(gsim::Logger::MESSAGE_LEVEL_INFO, "Total simulation runtime: %.3fs", runtimeSec);
				} else {
					programInfo.logger->LogMessageForced(gsim::Logger::MESSAGE_LEVEL_INFO, "Total simulation runtime: %.1fms", (runtimeSec * 1000));
				}

				// Log the average runtime
				if(runtimeAvgMs >= 5000) {
					programInfo.logger->LogMessageForced(gsim::Logger::MESSAGE_LEVEL_INFO, "Average runtime/simulation: %.3fs", (runtimeAvgMs * 0.001f));
				} else if(runtimeAvgMs >= 1) {
					programInfo.logger->LogMessageForced(gsim::Logger::MESSAGE_LEVEL_INFO, "Average runtime/simulation: %.3fms", runtimeAvgMs);
				} else {
					programInfo.logger->LogMessageForced(gsim::Logger::MESSAGE_LEVEL_INFO, "Average runtime/simulation: %.1fus", (runtimeAvgMs * 1000));
				}
			}

			// Destroy the simulation
			if(programInfo.directSim) {
				delete programInfo.directSim;
			} else {
				delete programInfo.barnesHutSim;
			}

			// Save the particle infos, if an output file was provided
			if(programInfo.particlesOutFile)
				programInfo.particleSystem->SaveParticles(programInfo.particlesOutFile);

			// Destroy the particle system
			delete programInfo.particleSystem;

			// Destroy the Vulkan components
			delete programInfo.device;
			delete programInfo.instance;
		} else {
			// Create the window
			programInfo.window = new gsim::Window(GSIM_PROJECT_NAME, 800, 800);

			// Create the Vulkan components
			programInfo.instance = new gsim::VulkanInstance(true, programInfo.logger);
			programInfo.surface = new gsim::VulkanSurface(programInfo.instance, programInfo.window);
			programInfo.device = new gsim::VulkanDevice(programInfo.instance, programInfo.surface);
			programInfo.swapChain = new gsim::VulkanSwapChain(programInfo.device, programInfo.surface);
		
			// Log info about the Vulkan objects
			programInfo.device->LogDeviceInfo(programInfo.logger);
			programInfo.swapChain->LogSwapChainInfo(programInfo.logger);

			// Create the particle system
			if(programInfo.particlesInFile) {
				programInfo.particleSystem = new gsim::ParticleSystem(programInfo.device, programInfo.particlesInFile, programInfo.gravitationalConst, programInfo.simulationTime, programInfo.simulationSpeed, programInfo.softeningLen, programInfo.accuracyParameter, programInfo.simulationAlgorithm);
			} else {
				programInfo.particleSystem = new gsim::ParticleSystem(programInfo.device, programInfo.particleCount, programInfo.generateType, programInfo.generateSize, programInfo.minMass, programInfo.maxMass, programInfo.gravitationalConst, programInfo.simulationTime, programInfo.simulationSpeed, programInfo.softeningLen, programInfo.accuracyParameter, programInfo.simulationAlgorithm);
			}

			// Set the camera's starting info
			programInfo.cameraPos = programInfo.particleSystem->GetCameraStartPos();
			programInfo.cameraSize = programInfo.particleSystem->GetCameraStartSize();

			// Create the pipelines
			programInfo.graphicsPipeline = new gsim::GraphicsPipeline(programInfo.device, programInfo.swapChain, programInfo.particleSystem);

			if(programInfo.simulationAlgorithm == gsim::ParticleSystem::SIMULATION_ALGORITHM_DIRECT_SUM) {
				programInfo.directSim = new gsim::DirectSimulation(programInfo.device, programInfo.particleSystem);
			} else {
				programInfo.barnesHutSim = new gsim::BarnesHutSimulation(programInfo.device, programInfo.particleSystem);
			}

			// Add the event listeners
			programInfo.window->GetDrawEvent().AddListener({ WindowDrawCallback, &programInfo });
			programInfo.window->GetKeyEvent().AddListener({ WindowKeyCallback, &programInfo });

			// Set all remaining program info
			programInfo.mousePos = programInfo.window->GetMousePos();
			programInfo.clockStart = clock();

			while(programInfo.window->GetWindowInfo().running) {
				// Parse the window's events
				programInfo.window->ParseEvents();

				// Run the simulations
				if(programInfo.directSim) {
					programInfo.directSim->RunSimulations((uint32_t)(programInfo.targetSimulationCount - programInfo.simulationCount));
				} else {
					programInfo.barnesHutSim->RunSimulations((uint32_t)(programInfo.targetSimulationCount - programInfo.simulationCount));
				}
				programInfo.simulationCount = programInfo.targetSimulationCount;

				// Close the window and exit the loop if all required simulations were run
				if(programInfo.simulationCount == programInfo.maxSimulationCount) {
					programInfo.window->CloseWindow();
					break;
				}

				// Store the clock end and calculate the target simulation count
				programInfo.targetSimulationCount = (uint64_t)((float)(clock() - programInfo.clockStart) / CLOCKS_PER_SEC / programInfo.simulationTime);
				if(programInfo.targetSimulationCount > programInfo.maxSimulationCount)
					programInfo.targetSimulationCount = programInfo.maxSimulationCount;
			}

			// Wait for the device to idle
			vkDeviceWaitIdle(programInfo.device->GetDevice());

			// Destroy the pipelines
			delete programInfo.graphicsPipeline;
			
			if(programInfo.directSim) {
				delete programInfo.directSim;
			} else {
				delete programInfo.barnesHutSim;
			}

			// Save the particle infos, if an output file was provided
			if(programInfo.particlesOutFile)
				programInfo.particleSystem->SaveParticles(programInfo.particlesOutFile);

			// Destroy the particle system
			delete programInfo.particleSystem;

			// Destroy the Vulkan components
			delete programInfo.swapChain;
			delete programInfo.device;
			delete programInfo.surface;
			delete programInfo.instance;

			// Destroy the window
			delete programInfo.window;
		}
	} catch(const gsim::Exception& exception) {
		// Log the exception
		programInfo.logger->LogException(exception);
	} catch(const std::exception& exception) {
		// Log the exception
		programInfo.logger->LogStdException(exception);
	}

	// Destroy the logger
	delete programInfo.logger;

	return 0;
}