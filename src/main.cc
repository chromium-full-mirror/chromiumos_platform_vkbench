// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include "clearTest.h"
#include "constant.h"
#include "drawSizeTest.h"
#include "submitTest.h"
#include "utils.h"

// g_iteration determines the total iteration to run for each test
int g_iteration = 1;
// g_verbose determine the logging level to print into the screen.
int g_verbose = false;
// g_vlayer enables the vulkan verification layer if it is set.
int g_vlayer = false;
// g_hasty enables the hasty mode. Tests would tries to reuse vulkan instance if
// possible.
int g_hasty = false;

// kLongOptions defines the options for argument options.
const static struct option kLongOptions[] = {
    {"iterations", required_argument, nullptr, 'i'},
    {"tests", required_argument, nullptr, 't'},
    {"blacklist", required_argument, nullptr, 'b'},
    {"list", no_argument, nullptr, 'l'},
    {"help", no_argument, nullptr, 'h'},
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

  try {
    for (int i = 0; i < iteration; i++) {
      test->Run();
    }
  } catch (const vk::SystemError& err) {
    LOG("TestRun failed: %s", err.what());
    throw TEST_FAIL;
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
    test->SaveImage(((std::string)test->Name()));
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
  --hasty                Enable hasty mode.),")
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
               std::vector<vkbench::testBase*>& all_tests) {
  std::vector<std::string> enabled_tests;
  std::vector<std::string> disabled_tests;
  int c;
  int option_index = 0;
  bool print_test = false;
  while ((c = getopt_long(argc, argv, "i:t:b:", kLongOptions, &option_index)) !=
         -1) {
    if (c == 'i') {
      g_iteration = atoi(optarg);
    } else if (c == 't') {
      enabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 'b') {
      disabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 'l') {
      print_test = true;
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

  all_tests.erase(
      remove_if(all_tests.begin(), all_tests.end(),
                [enabled_tests, disabled_tests](const vkbench::testBase* test) {
                  bool should_run = enabled_tests.empty() ||
                                    prefixFind(enabled_tests, test->Name());
                  should_run &= !prefixFind(disabled_tests, test->Name());
                  if (!should_run)
                    delete test;
                  return !should_run;
                }),
      all_tests.end());

  if (print_test) {
    for (const auto& test : all_tests) {
      LOG("%s: %s", test->Name(), test->Desp())
    }
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  vkbench::vkBase simpleVulkan;
  // all_tests list all the available tests.
  std::vector<vkbench::testBase*> all_tests = {
      new vkbench::SubmitTest(10, &simpleVulkan),
      new vkbench::SubmitTest(100, &simpleVulkan),
      new vkbench::SubmitTest(1000, &simpleVulkan),
      new vkbench::SubmitTest(10000, &simpleVulkan),
      new vkbench::SubmitTest(100000, &simpleVulkan),
      new vkbench::DrawSizeTest(16, &simpleVulkan),
      new vkbench::DrawSizeTest(64, &simpleVulkan),
      new vkbench::DrawSizeTest(128, &simpleVulkan),
      new vkbench::DrawSizeTest(512, &simpleVulkan),
      new vkbench::DrawSizeTest(1024, &simpleVulkan),
      new vkbench::ClearTest(&simpleVulkan),
  };

  // Sort to bundle tests using same vulkan instance together.
  std::stable_sort(all_tests.begin(), all_tests.end(),
                   [](vkbench::testBase* a, vkbench::testBase* b) -> bool {
                     return a->vk < b->vk;
                   });

  if (!ParseArgv(argc, argv, all_tests)) {
    return 0;
  }

  std::map<const char*, int> failed_test;
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
      } catch (const ERROR_TYPE& type) {
        switch (type) {
          case TEST_PASS:
            break;
          case TEST_FAIL:
            failed_test[test->Name()] += 1;
            break;
          default:
            ERROR("Unimplemented error type");
            throw;
        }
      } catch (const std::runtime_error error) {
        failed_test[test->Name()] += 1;
        LOG("Runtime Error: %s", error.what());
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
  LOG("@TEST_END")

  for (auto& keyval : failed_test) {
    LOG("%s failed %d times.", keyval.first, keyval.second)
  }

  for (auto& test : all_tests) {
    delete test;
  }
}
