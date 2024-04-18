// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "src/utils.h"

extern FilePath g_spirv_dir;

vkbench::Image::Image(const std::string& file_name) {
  load(g_spirv_dir.Append(FilePath(file_name)));
}

void vkbench::Image::Save(FilePath file_path) {
  if (data_ == nullptr) {
    RUNTIME_ERROR("No data to save");
  }
  std::string ext = "";

  std::string::size_type last_dot =
      file_path.value().find_last_of(".", std::string::npos);
  if (last_dot == std::string::npos) {
    RUNTIME_ERROR("No image extension found for file: {}", file_path);
  }
  ext = file_path.value().substr(last_dot + 1);

  if (ext == "png") {
    savePNG(file_path);
  } else if (ext == "ppm") {
    savePPM(file_path);
  } else {
    RUNTIME_ERROR("Unknown extension: {}", ext);
  }
  DEBUG("Image saved to {}", file_path);
  // Try to flush saved image to disk such that more data survives a hard crash.
  system("/bin/sync");
}

void vkbench::Image::load(FilePath file_path) {
  std::string ext = "";
  std::string::size_type last_dot =
      file_path.value().find_last_of(".", std::string::npos);
  if (last_dot == std::string::npos) {
    RUNTIME_ERROR("No image extension found for file: {}", file_path);
  }
  ext = file_path.value().substr(last_dot + 1);
  if (ext == "png") {
    loadPNG(file_path);
  } else {
    NOT_SUPPORT("Unknown extension: {}", ext);
  }
}

void vkbench::Image::savePPM(FilePath file_path) {
  FilePath directory = file_path.DirName();
  CreateDirectory(directory);

  std::ofstream file(file_path.value(), std::ios::binary | std::ios::out);
  DEFER(file.close());

  const unsigned char* row_ptr = data_ + resource_layout_.offset;
  file << "P6\n"
       << extent_.width << "\n"
       << extent_.height << "\n"
       << 255 << std::endl;
  for (uint32_t y = 0; y < extent_.height; y++) {
    unsigned int* row = (unsigned int*)row_ptr;
    for (uint32_t x = 0; x < extent_.width; x++) {
      file.write(reinterpret_cast<char*>(row), 3);
      row++;
    }
    row_ptr += resource_layout_.rowPitch;
  }
}

void vkbench::Image::savePNG(FilePath file_path) {
  FilePath directory = file_path.DirName();
  CreateDirectory(directory);

  /* create file */
  FILE* file = fopen(file_path.c_str(), "wb");
  if (!file)
    RUNTIME_ERROR("File {} couldn't be opened for writing", file_path);
  DEFER(fclose(file));

  png_bytep* row_pointers = new png_bytep[sizeof(png_bytep) * extent_.height];
  unsigned char* row_ptr = data_ + resource_layout_.offset;
  for (int y = extent_.height - 1; y >= 0; y--) {
    row_pointers[y] = new png_byte[4 * extent_.width];
    memcpy(row_pointers[y], row_ptr, 4 * extent_.width);
    row_ptr += resource_layout_.rowPitch;
  }

  /* initialize stuff */
  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    RUNTIME_ERROR("png_create_write_struct failed");
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    RUNTIME_ERROR("png_create_info_struct failed");
  if (setjmp(png_jmpbuf(png_ptr)))
    RUNTIME_ERROR("error during png_init_io");
  png_init_io(png_ptr, file);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr)))
    RUNTIME_ERROR("error during writing png header");
  png_byte bit_depth = 8;                     // 8 bits per channel RGBA
  png_byte color_type = PNG_COLOR_TYPE_RGBA;  // RGBA
  png_set_IHDR(png_ptr, info_ptr, extent_.width, extent_.height, bit_depth,
               color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);

  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr)))
    RUNTIME_ERROR("error during png_write_image");
  png_write_image(png_ptr, row_pointers);

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr)))
    RUNTIME_ERROR("error during png_write_end");
  png_write_end(png_ptr, NULL);

  for (int y = 0; y < extent_.height; y++)
    delete row_pointers[y];
  delete[] row_pointers;
}

void vkbench::Image::loadPNG(FilePath file_path) {
  FILE* fp = fopen(file_path.c_str(), "rb");
  if (!fp) {
    RUNTIME_ERROR("File {} could not be oppened", file_path.value());
  }
  DEFER(fclose(fp));

  unsigned char header[8];
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    RUNTIME_ERROR("Faile {} is not recognized as a PNG file",
                  file_path.value());
  }

  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    RUNTIME_ERROR("png_create_read_struct fails");
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    RUNTIME_ERROR("png_create_info_struct fails");
  }
  if (setjmp(png_jmpbuf(png))) {
    RUNTIME_ERROR("loadPNG fails to setjmp");
  }
  png_init_io(png, fp);
  png_set_sig_bytes(png, 8);
  png_read_info(png, info);

  extent_.width = png_get_image_width(png, info);
  extent_.height = png_get_image_height(png, info);
  resource_layout_.offset = 0;
  resource_layout_.rowPitch = png_get_rowbytes(png, info);
  resource_layout_.size = extent_.height * resource_layout_.rowPitch;
  png_byte color_type = png_get_color_type(png, info);
  if (color_type != PNG_COLOR_TYPE_RGBA) {
    RUNTIME_ERROR("Not recognized png color type: {}", color_type);
  }
  png_byte bit_depth = png_get_bit_depth(png, info);
  if (bit_depth != 8) {
    RUNTIME_ERROR("Not supported bit_depth: {}", bit_depth);
  }
  // read file
  if (setjmp(png_jmpbuf(png))) {
    RUNTIME_ERROR("PNG read image fails");
  }
  data_ = new unsigned char[extent_.height * resource_layout_.rowPitch];
  png_bytep *row_ptrs = new png_bytep[extent_.height];
  DEFER(delete row_ptrs);
  for (int y = 0; y < extent_.height; y++) {
    row_ptrs[y] = data_ + (extent_.height - 1 - y) * resource_layout_.rowPitch;
  }
  png_read_image(png, row_ptrs);
}

void PrintDateTime() {
  time_t timer;
  char buffer[26];
  time(&timer);
  struct tm tm_info;
  localtime_r(&timer, &tm_info);
  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", &tm_info);
  LOG("# DateTime: {}", buffer);
}

std::string readShaderFile(const std::string& filename) {
  FilePath file_path = g_spirv_dir.Append(FilePath(filename + ".spv"));
  std::ifstream file(file_path.value(), std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    RUNTIME_ERROR("Failed to open {}", file_path);
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
    int64_t size = ftell(fd);
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

uint32_t randi(uint32_t max) {
  return rand() % (max + 1);
}
