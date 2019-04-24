// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const std::string kTemperatureScript =
    "/usr/local/autotest/bin/temperature.py --maximum";

void PrintDateTime() {
  time_t timer;
  char buffer[26];
  time(&timer);
  struct tm* tm_info = localtime(&timer);
  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
  LOG("# DateTime: %s", buffer)
}

/* Execute a shell command and return its file descriptor for reading output. */
/* @param command: command to be run. */
/* @param result: the stdout of the command output. */
/* @return true if the command is executed successfully. */
bool ExecuteCommand(const std::string& kCommand,
                    std::string* result = nullptr) {
  FILE* fd = popen(kCommand.c_str(), "r");
  if (!fd) {
    return false;
  }

  if (result) {
    fseek(fd, 0, SEEK_END);
    long size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    fread(&result[0], sizeof(char), size, fd);
  }
  return pclose(fd) == 0;
}

std::vector<std::string> SplitString(const std::string& kInput,
                                     char delimiter) {
  std::vector<std::string> result;
  std::stringstream srcStream(kInput);

  std::string token;
  while (getline(srcStream, token, delimiter)) {
    result.push_back(token);
  }
  return result;
}

// Returns currently measured temperature.
double GetMachineTemperature() {
  std::string temp_str;
  double temperature;
  if (!ExecuteCommand(kTemperatureScript, &temp_str)) {
    return 10000;
  }
  temperature = strtod(temp_str.c_str(), nullptr);
  if (temperature < 10.0 || temperature > 150.0) {
    DEBUG("Warning: ignoring temperature reading of %f'C.", temperature)
  }
  return temperature;
}

// Waits up to timeout seconds to reach cold_temperature in Celsius.
// @param cold_temperature: the target temperature in celsius.
// @param timeout: the timeout to wait.
// @param temperature: the final temperature of the machine.
// @return: the total second of wait time.
double WaitForCoolMachine(double cold_temperature,
                          double timeout,
                          double* temperature) {
  uint64_t start, now, end;
  start = now = GetUTime();
  end = now + 1e6 * timeout;
  do {
    *temperature = GetMachineTemperature();
    if (*temperature < cold_temperature)
      break;
    sleep(1.0);
    now = GetUTime();
  } while (now < end);
  return 1.0e-6 * (now - start);
}

bool IsItemInVector(const std::vector<std::string>& list,
                    const char* value,
                    bool empty_value = false) {
  if (list.empty())
    return empty_value;
  return !(find(list.begin(), list.end(), std::string(value)) == list.end());
}
