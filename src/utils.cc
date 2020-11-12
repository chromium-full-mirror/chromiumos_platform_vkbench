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

extern FilePath g_spirv_dir;

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

std::string readShaderFile(const std::string& filename) {
  FilePath file_path = g_spirv_dir.Append(FilePath(filename));
  std::ifstream file(file_path.value(), std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open " + file_path.value());
  }
  DEFER(file.close());

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  return std::string(buffer.begin(), buffer.end());
}

// CreateShaderModule creates a shader module from a loaded SPIR-V code.
vk::ShaderModule CreateShaderModule(const vk::Device& device,
                                    std::string code) {
  vk::ShaderModuleCreateInfo create_info;
  create_info.setCodeSize(code.length());
  create_info.setPCode(reinterpret_cast<const uint32_t*>(code.c_str()));
  return device.createShaderModule(create_info);
}

// SavePPM saves data into file_path in PPM format. It assumes data is in
// R8G8B8A8_UNORM format.
void SavePPM(FilePath file_path,
             const char* data,
             uint32_t width,
             uint32_t height,
             vk::DeviceSize row_pitch) {
  FilePath directory = file_path.DirName();
  CreateDirectory(directory);

  std::ofstream file(file_path.value(), std::ios::binary | std::ios::out);
  DEFER(file.close());

  file << "P6\n" << width << "\n" << height << "\n" << 255 << std::endl;
  for (uint32_t y = 0; y < height; y++) {
    unsigned int* row = (unsigned int*)data;
    for (uint32_t x = 0; x < width; x++) {
      file.write((char*)row, 3);
      row++;
    }
    data += row_pitch;
  }
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

bool check_file_existence(const char* file_path, struct stat* buffer) {
    struct stat local_buf;
    bool exist = stat(file_path, &local_buf) == 0;
    if (buffer && exist)
        memcpy(buffer, &local_buf, sizeof(local_buf));
    return exist;
}

bool check_dir_existence(const char* file_path) {
    struct stat buffer;
    bool exist = check_file_existence(file_path, &buffer);
    if (!exist)
        return false;
    return S_ISDIR(buffer.st_mode);
}
