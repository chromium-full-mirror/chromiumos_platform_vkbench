// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include "src/filepath.h"
#include "src/tests/clear.h"
#include "src/tests/copy.h"
#include "src/tests/draw.h"
#include "src/tests/submit.h"
#include "src/utils.h"

// g_list determines should we show the test list.
int g_list = false;
// g_iteration determines the total iteration to run for each test
int g_iteration = 1;
// g_verbose determines the logging level to print into the screen.
int g_verbose = false;
// g_vlayer enables the vulkan verification layer if it is set.
int g_vlayer = false;
// g_hasty enables the hasty mode. Tests would tries to reuse vulkan instance if
// possible.
int g_hasty = false;
// g_spirv_dir is the path to the folder contains spirv code for test.
FilePath g_spirv_dir = FilePath("shaders");
// g_out_dir is the path to the folder to store the output image.
FilePath g_out_dir = FilePath("");

const char RESULT_FMT[] = "@RESULT: {:<46s} = {:>10.2f} {:<15s}";
const char SKIP_RESULT_FMT[] = "@RESULT: {:<46s} = SKIP[{}]";
const char ERROR_RESULT_FMT[] = "@RESULT: {:<46s} = ERROR[{}]";

// kLongOptions defines the options for argument options.
static const struct option kLongOptions[] = {
    {"iterations", required_argument, nullptr, 'i'},
    {"tests", required_argument, nullptr, 't'},
    {"blacklist", required_argument, nullptr, 'b'},
    {"spirv_dir", required_argument, nullptr, 's'},
    {"out_dir", required_argument, nullptr, 'o'},
    {"help", no_argument, nullptr, 'h'},
    {"list", no_argument, &g_list, 1},
    {"vlayer", no_argument, &g_vlayer, 1},
    {"verbose", no_argument, &g_verbose, 1},
    {"hasty", no_argument, &g_hasty, 1},
    {0, 0, 0, 0}};

// TimeTest times the test by looping it iteration times.
// @param test: test to be excuted.
// @param iteration: how many times should the test be executed.
// @return time spent in nanoseconds.
inline uint64_t TimeTest(vkbench::testBase* test, uint64_t iteration) {
  test->vk->GetGFXQueue().waitIdle();
  try {
    test->Setup(iteration);
  } catch (const vk::SystemError& err) {
    LOG("Setup failed: {}", err.what());
    test->Cleanup();
    throw;
  }
  DEFER(test->Cleanup());

  test->vk->GetGFXQueue().waitIdle();
  uint64_t start = GetNTime();
  test->RunFunc(iteration);
  test->vk->GetGFXQueue().waitIdle();
  return GetNTime() - start;
}

// Run the test and pre/post processes.
// @param duration_us: The test would to iterate till duration_us is reached.
void Run(vkbench::testBase* test, const uint64_t duration_ns = 400000000) {
  try {
    test->Initialize();
  } catch (const vk::SystemError& err) {
    LOG("Test failed to initialize: {}", err.what());
    test->Destroy();
    throw;
  }
  DEFER(test->Destroy());

  // Do some iterations since initial timings may vary.
  TimeTest(test, 2);
  // Target minimum iteration is 1s for each test.
  uint64_t time = 0;
  uint64_t time_prev = 0;
  uint64_t iteration = 1;
  uint64_t iteration_prev = 0;
  double score = -1.f;
  do {
    time = TimeTest(test, iteration);
    DEBUG("iterations: {}, time: {} ns, time/iter: {} ns", iteration, time,
          time / iteration);
    if (time > duration_ns) {
      // We divide by 1000.0 to convert from nanoseconds to microseconds for use
      // by the caller to compute Mpixel/sec.
      score = (time + time_prev) / (iteration + iteration_prev) / 1000.0;
      break;
    }
    iteration_prev = iteration;
    time_prev = time;
    iteration = iteration * 2;
  } while ((1ull << 40) > iteration);

  // Returns 0.0 if it ran max iterations in less than test time.
  if (score <= 0.01f)
    LOG("{}: measurement may not be accurate.", test->Name());
  score = test->FormatMeasurement(score);
  LOG(RESULT_FMT, test->Name(), score, test->Unit());
  try {
    FilePath file_path =
        g_out_dir.Append(FilePath(fmt::format("{}.png", test->Name())));
    test->GetImage().Save(file_path);
  } catch (vkbench::not_supported_exception err) {
    // Catch the error here as not all tests would generate the image and we
    // don't want to mark the test SKIP.
    DEBUG("No image to save for test {}: {}", test->Name(), err.what());
  }
}

void PrintHelp() {
  LOG(R",(
Usage: vkbench [OPTIONS]
  -i, --iterations=N     Specify the iterations to run the tests.
  -t, --tests=TESTS      Tests to run in colon separated form.
  -b, --blacklist=TESTS  Tests to not run in colon separated form.
  --list                 List the tests available.
  --verbose              Show verbose messages.
  --vlayer               Enable vulkan verification layer.
  --hasty                Enable hasty mode.
                         In this mode, tests will try to reuse existing vulkan
                         instance if possible. It also runs a reduced number
                         of tests and checks the validity of each tests.
  --spirv_dir            Path to SPIRV code for test.(default: shaders/)
  --out_dir              Path to the output directory.),");
}

bool prefixFind(std::vector<std::string> list, std::string name) {
  for (const std::string item : list) {
    if (name.rfind(item, 0) == 0) {
      return true;
    }
  }
  return false;
}

bool ParseArgv(int argc,
               char** argv,
               std::vector<std::string>* enabled_tests,
               std::vector<std::string>* disabled_tests) {
  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "i:t:b:", kLongOptions, &option_index)) !=
         -1) {
    if (c == 'i') {
      g_iteration = atoi(optarg);
    } else if (c == 't') {
      *enabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 'b') {
      *disabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 's') {
      g_spirv_dir = FilePath(optarg);
    } else if (c == 'o') {
      g_out_dir = FilePath(optarg);
    } else if (c == '?' || c == 'h') {
      PrintHelp();
      return false;
    }
  }

  if (optind < argc) {
    ERROR("Unknown argv: ");
    while (optind < argc)
      ERROR("{} ", argv[optind++]);
    return false;
  }
  return true;
}

std::vector<vkbench::testBase*> genTests(
    std::vector<std::string> enabled_tests,
    std::vector<std::string> disabled_tests) {
  std::vector<vkbench::testBase*> all_tests;
  auto appendList = [](std::vector<vkbench::testBase*>& a,
                       const std::vector<vkbench::testBase*>& b) {
    if (b.size() == 0)
      return;
    if (g_hasty)
      a.push_back(b.front());
    else
      a.insert(a.end(), b.begin(), b.end());
  };
  appendList(all_tests, vkbench::submit::GenTests());
  appendList(all_tests, vkbench::draw::GenTests());
  appendList(all_tests, vkbench::clear::GenTests());
  appendList(all_tests, vkbench::copy::GenTests());

  auto filterTests = [enabled_tests,
                      disabled_tests](const vkbench::testBase* test) {
    bool should_run =
        enabled_tests.empty() || prefixFind(enabled_tests, test->Name());
    should_run &= !prefixFind(disabled_tests, test->Name());
    if (!should_run)
      delete test;
    return !should_run;
  };
  all_tests.erase(remove_if(all_tests.begin(), all_tests.end(), filterTests),
                  all_tests.end());
  // Sort to bundle tests based on its vulkan instance and test name.
  std::stable_sort(all_tests.begin(), all_tests.end(),
                   [](vkbench::testBase* a, vkbench::testBase* b) -> bool {
                     if (a->vk == b->vk) {
                       return strcmp(a->Name().c_str(), b->Name().c_str()) < 0;
                     }
                     return a->vk < b->vk;
                   });
  return all_tests;
}

int main(int argc, char* argv[]) {
  std::vector<std::string> enabled_tests, disabled_tests;
  if (!ParseArgv(argc, argv, &enabled_tests, &disabled_tests))
    return -1;
  std::vector<vkbench::testBase*> all_tests =
      genTests(enabled_tests, disabled_tests);

  if (g_list) {
    for (const auto& test : all_tests) {
      LOG("{}: {}", test->Name(), test->Desp());
    }
    return 0;
  }

  LOG("@TEST_BEGIN");
  PrintDateTime();
  for (auto i = 0; i < all_tests.size(); i++) {
    auto& test = all_tests[i];
    for (auto iter = 0; iter < g_iteration; iter++) {
      // Use fixed random seed for reproducibility.
      // This ensures for each iteration, each tests got same random numbers.
      srand(0);
      try {
        if (!test->vk->IsInitialized())
          test->vk->Initialize();
        Run(test);
        if (!g_hasty)
          test->vk->Destroy();
      } catch (const vkbench::not_supported_exception error) {
        // This catch should be checked before runtime_error as
        // not_supported_exception is derived from runtime_error.
        LOG(SKIP_RESULT_FMT, test->Name(), error.what());
      } catch (const std::runtime_error error) {
        LOG(ERROR_RESULT_FMT, test->Name(), error.what());
      }
    }

    // keep test->vk initialized for the next test.
    if (g_hasty && test->vk->IsInitialized()) {
      if (i + 1 >= all_tests.size() || test->vk != all_tests[i + 1]->vk) {
        test->vk->Destroy();
      }
    }
  }
  PrintDateTime();
  LOG("@TEST_END");
  for (auto& test : all_tests) {
    delete test;
  }
}
