#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>

#include <libpng16/png.h>
#include <png.h>
#include "pngconf.h"


struct CommandLineArgs {
  // Filepath to a valid PNG image.
  std::string img_filepath;
};

void print_help() {
  fmt::println("USAGE:");
  fmt::println("  app [OPTIONS] FILEPATH");
}

int parse_args(int argc, char** argv, CommandLineArgs *cli_args) {
  // Ensure required args are available.
  if (argc < 2) {
    fmt::println("Insufficient number of arguments");
    print_help();
    return 1;
  }

  // Assuming the only argument available is the filepath.
  cli_args->img_filepath = std::string{argv[1]};
  return 0;
}

int read_png_file(const char* filepath, png_structp& png_ptr, png_infop& info_ptr, png_bytepp& row_pointers) {
  FILE* fp = fopen(filepath, "rb");
  if (!fp) {
    fmt::println("Failed to open image '{}'", filepath);
    return -1;
  }

  // Read PNG image.
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  png_init_io(png_ptr, fp);

  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  row_pointers = png_get_rows(png_ptr, info_ptr);

  fclose(fp);
  return 0;
}

int write_png_file(const char* filepath, png_infop& info_ptr, png_bytepp& row_pointers) {
  FILE* fp = fopen(filepath, "wb");
  if (!fp) {
    fmt::println("Failed write image to '{}': Failed to open file", filepath);
    return 1;
  }

  // Create IO to write PNG to the opened file.
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_init_io(png_ptr, fp);

  // Write that PNG!
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  // Clean up!
  png_destroy_write_struct(&png_ptr, NULL);
  fclose(fp);
  return 0;
}


int main(int argc, char** argv) {
  CommandLineArgs cli_args;
  if (parse_args(argc, argv, &cli_args) != 0) {
    return 1;
  }

  // Verify valid filepath.
  if (!std::filesystem::exists(cli_args.img_filepath)) {
    fmt::println("Please provide a valid filepath to a PNG image. '{}' does not exist!", cli_args.img_filepath);
    return 1;
  }

  // Shared PNG structs between read/write contexts.
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytepp row_pointers;

  // Read image.
  if (read_png_file(cli_args.img_filepath.c_str(), png_ptr, info_ptr, row_pointers) != 0) {
    fmt::println("Failed to read PNG image");
    return 1;
  }

  // Do stuff with image.
  size_t width      = png_get_image_width(png_ptr, info_ptr);
  size_t height     = png_get_image_height(png_ptr, info_ptr);
  png_byte channels = png_get_channels(png_ptr, info_ptr);
  fmt::println("Parsed PNG image: {}x{} | Channels={}", width, height, channels);

  for (size_t y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for (size_t x = 0; x < width; x++) {
      png_bytep px = &(row[x * channels]);
      px[0] = 255 - px[0];
      px[1] = 255 - px[1];
      px[2] = 255 - px[2];
    }
  }


  // Write image.
  std::filesystem::path out_filename_path{cli_args.img_filepath};
  out_filename_path.replace_filename("out");
  out_filename_path.replace_extension(".png");

  if (write_png_file(out_filename_path.c_str(), info_ptr, row_pointers) != 0) {
    fmt::println("Failed to write image");
  }
  else {
    fmt::println("Wrote new image to '{}'", out_filename_path.c_str());
  }


  // When all is done, clean up shared info ptr.
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return 0;
}

