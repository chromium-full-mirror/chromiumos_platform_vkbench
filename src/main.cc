// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include "./filepath.h"
#include "./utils.h"
#include "tests/clear.h"
#include "tests/copy.h"
#include "tests/draw.h"
#include "tests/submit.h"

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
inline uint64_t TimeTest(vkbench::testBase* test, uint64_t iteration) {
  uint64_t start = GetUTime();
  try {
    test->Setup();
  } catch (const vk::SystemError& err) {
    LOG("Setup failed: %s", err.what());
    test->Cleanup();
    throw;
  }
  DEFER(test->Cleanup());

  for (int i = 0; i < iteration; i++) {
    test->Run();
  }
  return GetUTime() - start;
}

// Run the test and pre/post processes.
// @param duration_us: The test would to iterate till duration_us is reached.
void Run(vkbench::testBase* test, const uint64_t duration_us = 1000000) {
  try {
    test->Initialize();
  } catch (const vk::SystemError& err) {
    LOG("Test failed to initialize: %s", err.what());
    test->Destroy();
    throw;
  }
  DEFER(test->Destroy());

  // Do some iterations since initial timings may vary.
  TimeTest(test, 2);
  // Target minimum iteration is 1s for each test.
  uint64_t time = 0;
  uint64_t iteration = 1;
  double score = -1.f;
  do {
    time = TimeTest(test, iteration);
    DEBUG("iterations: %llu, time: %llu us, time/iter: %llu us", iteration,
          time, time / iteration)
    if (time > duration_us) {
      score = time / iteration;
      break;
    }
    iteration = iteration * 2;
  } while ((1ull << 40) > iteration);
  // Returns 0.0 if it ran max iterations in less than test time.
  if (score <= 0.01f)
    LOG("%s: measurement may not be accurate.", test->Name())
  score = test->FormatMeasurement(score);
  LOG("@RESULT: %46s = %10.2f %-15s", test->Name(), score, test->Unit());
  try {
    char file_name[1024];
    snprintf(file_name, sizeof(file_name), "%s.png", test->Name());
    FilePath file_path = g_out_dir.Append(FilePath(file_name));

    vkbench::Image image = test->GetImage();
    image.Save(file_path);
  } catch (std::runtime_error err) {
    DEBUG("Get runtime_error while SaveImage: %s.", err.what());
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
  --out_dir              Path to the output directory.),")
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
    ERROR("Unknown argv: ")
    while (optind < argc)
      ERROR("%s ", argv[optind++])
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
  // Sort to bundle tests using same vulkan instance together.
  std::stable_sort(all_tests.begin(), all_tests.end(),
                   [](vkbench::testBase* a, vkbench::testBase* b) -> bool {
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
      LOG("%s: %s", test->Name(), test->Desp())
    }
    return false;
  }

  LOG("@TEST_BEGIN")
  PrintDateTime();
  for (auto i = 0; i < all_tests.size(); i++) {
    auto& test = all_tests[i];
    for (auto iter = 0; iter < g_iteration; iter++) {
      try {
        if (!test->vk->IsInitialized())
          test->vk->Initialize();
        Run(test, 1000000);
        if (!g_hasty)
          test->vk->Destroy();
      } catch (const std::runtime_error error) {
        LOG("@RESULT: %46s = ERROR[%s]", test->Name(), error.what());
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
