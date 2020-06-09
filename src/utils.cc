// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "utils.h"

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

bool IsItemInVector(const std::vector<std::string>& list,
                    const char* value,
                    bool empty_value = false) {
  if (list.empty())
    return empty_value;
  return !(find(list.begin(), list.end(), std::string(value)) == list.end());
}
