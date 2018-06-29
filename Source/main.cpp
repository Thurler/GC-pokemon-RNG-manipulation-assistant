#include <QApplication>

#include <CL/cl.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include "GUI/MainWindow.h"
#include "PokemonRNGSystem/Colosseum/ColosseumRNGSystem.h"

int main(int argc, char** argv)
{
  ColosseumRNGSystem system;
  ColosseumRNGSystem::seedRange range = system.getRangeForSettings(false, 5);

  cl_int err;
  // get all platforms (drivers)
  std::vector<cl::Platform> all_platforms;
  cl::Platform::get(&all_platforms);
  if (all_platforms.size() == 0)
  {
    std::cout << " No platforms found. Check OpenCL installation!\n";
    exit(1);
  }
  cl::Platform default_platform = all_platforms[0];
  std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

  // get default device of the default platform
  std::vector<cl::Device> all_devices;
  default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
  if (all_devices.size() == 0)
  {
    std::cout << " No devices found. Check OpenCL installation!\n";
    exit(1);
  }
  cl::Device default_device = all_devices[0];
  std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>()
            << " with this amount of compute unit: "
            << default_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "\n";

  cl::Context context({default_device});

  cl::Program::Sources sources;

  std::ifstream t("main.cl");
  std::string kernel_code((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  sources.push_back({kernel_code.c_str(), kernel_code.length()});

  cl::Program program(context, sources);
  // const char* options = "-g -s \"/home/aldelaro5/vm-shared/GC-pokemon-RNG-manipulation-assistant
  // git/Sources/main.cl\"";
  if (program.build({default_device}) != CL_SUCCESS)
  {
    std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)
              << "\n";
    std::cin.ignore();
    exit(1);
  }

  const size_t max = range.max - range.min;
  // create queue to which we will push commands for the device.
  cl::CommandQueue queue(context, default_device);

  // create buffers on the device
  cl::Buffer buffer_criteria(context, CL_MEM_READ_ONLY, sizeof(int) * 2);
  cl::Buffer buffer_seeds(context, CL_MEM_WRITE_ONLY, sizeof(u32) * max);
  cl::Buffer buffer_rangeOffset(context, CL_MEM_READ_ONLY, sizeof(u32));

  int criteria[] = {0, 0};
  u32* things = new u32[max];
  for (int i = 0; i < max; i++)
    things[i] = 0;

  // write arrays A and B to the device
  err = queue.enqueueWriteBuffer(buffer_criteria, CL_TRUE, 0, sizeof(int) * 2, criteria);
  err = queue.enqueueWriteBuffer(buffer_seeds, CL_TRUE, 0, sizeof(u32) * max, things);
  err = queue.enqueueWriteBuffer(buffer_rangeOffset, CL_TRUE, 0, sizeof(u32), &range.min);
  delete[] things;
  // alternative way to run the kernel
  cl::Kernel kernel_add = cl::Kernel(program, "seedFinder");
  err = kernel_add.setArg(0, buffer_criteria);
  err = kernel_add.setArg(1, buffer_seeds);
  err = kernel_add.setArg(2, buffer_rangeOffset);

  err = queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(max), cl::NullRange);
  queue.finish();
  std::cout << "done\n";
  std::vector<u32> seeds;
  u32* result = new u32[max];
  // read result C from the device to array C
  err = queue.enqueueReadBuffer(buffer_seeds, CL_TRUE, 0, sizeof(u32) * max, result);
  for (int j = 0; j < max; j++)
  {
    if (result[j] != 0)
      seeds.push_back(result[j]);
  }
  delete[] result;
  while (seeds.size() > 1)
  {
    system.seedFinderPass({0, 0}, seeds, false, 5, false, [](long p) {}, []() { return false; });
  }
  /*QApplication app(argc, argv);
  MainWindow window;
  window.show();
  return app.exec();*/
  std::cin.ignore();
  return 0;
}
