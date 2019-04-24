// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include "submitTest.h"
#include "utils.h"

// g_iteration determines the total iteration to run for each test
int g_iteration = 1;
// g_verbose determine the logging level to print into the screen.
int g_verbose = false;
// g_vlayer enables the vulkan verification layer if it is set.
int g_vlayer = false;
// g_notemp skips the temperature checking if it is set.
int g_notemp = false;

// all_tests list all the available tests.
std::vector<vkbench::testBase*> all_tests = {
    new vkbench::SubmitTest(10),
    new vkbench::SubmitTest(100),
    new vkbench::SubmitTest(1000),
    new vkbench::SubmitTest(10000),
};
// kLongOptions defines the options for argument options.
const static struct option kLongOptions[] = {
    {"iterations", required_argument, nullptr, 'i'},
    {"tests", required_argument, nullptr, 't'},
    {"blacklist", required_argument, nullptr, 'b'},
    {"list", no_argument, nullptr, 'l'},
    {"help", no_argument, nullptr, 'h'},
    {"notemp", no_argument, &g_notemp, 1},
    {"vlayer", no_argument, &g_vlayer, 1},
    {"verbose", no_argument, &g_verbose, 1},
    {0, 0, 0, 0}};

// TimeTest times the test by looping it iteration times.
inline uint64_t TimeTest(vkbench::testBase* test, uint64_t iteration) {
  uint64_t start = GetUTime();
  bool passed = true;
  try {
    test->TestSetup();
    for (int i = 0; i < iteration && passed; i++) {
      passed = test->TestRun();
    }
    test->TestCleanup();
  } catch (std::system_error e) {
    DEBUG("Error while timing test %s.", test->Name())
    throw e;
  }
  if (!passed) {
    return std::numeric_limits<uint64_t>::max();
  }
  return GetUTime() - start;
}

// Run the test and pre/post processes.
// @param target_temperature: The test would try to start until the machine get
//                            to this temperature unless notemp is set.
// @param duration_us: The test would to iterate till duration_us is reached.
// @return: true if the test passes.
bool Run(vkbench::testBase* test,
         const double init_temperature,
         const uint64_t duration_us = 1000000) {
  try {
    test->VkInitialization();
    test->TestInitialization();
    if (!g_notemp) {
      double temperature = 0;
      double cooldown_temperature = std::min(45.0, init_temperature + 6.0);
      double
          wait = WaitForCoolMachine(cooldown_temperature, 30.0, &temperature);
      LOG("Waited %f second for machine temperature to cool down to %f.", wait,
          temperature)
    }
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
        if (time == std::numeric_limits<uint64_t>::max()) {
          return false;
        } else {
          score = time / iteration;
          break;
        }
      }
      iteration = iteration * 2;
    } while ((1ull << 40) > iteration);
//     Returns 0.0 if it ran max iterations in less than test time.
    if (score <= 0.01f) LOG("%s: measurement may not be accurate.",
                            test->Name())
    score = test->FormatMeasurement(score);
    LOG("@RESULT: %46s = %10.2f %-15s", test->Name(), score, test->Unit())

  } catch (std::system_error e) {
    DEBUG("Error while running test: %s", e.what())
    return false;
  }
  test->TestDestroy();
  test->VkDestroy();
  return true;
}

void PrintTests() {
  for (const auto& test : all_tests) {
    LOG("%s: %s", test->Name(), test->Desp())
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
  --notemp               Do not wait for temperature to cool down.),")
}

bool ParseArgv(int argc, char** argv) {
  std::vector<std::string> enabled_tests;
  std::vector<std::string> disabled_tests;
  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "i:t:b:", kLongOptions, &option_index)) !=
      -1) {
    if (c == 'i') {
      g_iteration = atoi(optarg);
    } else if (c == 't') {
      enabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 'b') {
      disabled_tests = SplitString(std::string(optarg), ':');
    } else if (c == 'l') {
      PrintTests();
      return false;
    } else if (c == '?' || c == 'h') {
      PrintHelp();
      return false;
    }
  }

  if (optind < argc) {
    ERROR("Unknown argv: ")
    while (optind < argc) ERROR("%s ", argv[optind++])
    return false;
  }

  all_tests.erase(
      remove_if(
          all_tests.begin(), all_tests.end(),
          [enabled_tests, disabled_tests](const vkbench::testBase* test) {
            bool isDisabled =
                !IsItemInVector(enabled_tests, test->Name(), true) ||
                    IsItemInVector(disabled_tests, test->Name(), false);
            if (isDisabled)
              delete test;
            return isDisabled;
          }),
      all_tests.end());
  return true;
}

int main(int argc, char* argv[]) {
  if (!ParseArgv(argc, argv))
    return 0;

  double init_temperature = g_notemp ? 40 : GetMachineTemperature();
  std::map<const char*, int> failed_test;
  LOG("@TEST_BEGIN")
  PrintDateTime();
  for (auto i = 0; i<all_tests.size(); i++ ){
    auto &test = all_tests[i];
    for (auto iter = 0; iter < g_iteration; i++) {
      if (!Run(test, init_temperature, 1000000 )) {
        failed_test[test->Name()] += 1;
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

  return 0;
}