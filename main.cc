#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>

#include <libpng16/png.h>
#include <png.h>
#include "pngconf.h"

template<typename T>
struct Vector2D {
  T x;
  T y;
};

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

void print_png_info(png_structp& png_ptr, png_infop& info_ptr) {
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth  = png_get_bit_depth(png_ptr, info_ptr);
  png_byte channels   = png_get_channels(png_ptr, info_ptr);
  png_uint_32 width   = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 height  = png_get_image_height(png_ptr, info_ptr);

  fmt::println("Image Parsed:");
  fmt::println("  - Color type = {}", color_type);
  fmt::println("  - Bit depth  = {}", bit_depth);
  fmt::println("  - Channels   = {}", channels);
  fmt::println("  - Height     = {}", height);
  fmt::println("  - Width      = {}", width);
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
  if(!info_ptr) return 1;

  if(setjmp(png_jmpbuf(png_ptr))) return 1;

  png_init_io(png_ptr, fp);
  png_read_info(png_ptr, info_ptr);

  // Configure color types.
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth  = png_get_bit_depth(png_ptr, info_ptr);
  png_uint_32 height  = png_get_image_height(png_ptr, info_ptr);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png_ptr);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  // TODO: make more C++ like.
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(png_uint_32 y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));
  }
  png_read_image(png_ptr, row_pointers);

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

  png_uint_32 height  = png_get_image_height(png_ptr, info_ptr);
  png_uint_32 width  = png_get_image_width(png_ptr, info_ptr);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png_ptr,
    info_ptr,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );

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

  // Alright now we're cookin.
  png_uint_32 height  = png_get_image_height(png_ptr, info_ptr);
  print_png_info(png_ptr, info_ptr);

  // Do stuff with image.
  apply_radius(100, png_ptr, info_ptr, row_pointers);

  // Write image.
  std::filesystem::path out_filename_path{"out.png"};

  if (write_png_file(out_filename_path.c_str(), info_ptr, row_pointers) != 0) {
    fmt::println("Failed to write image");
  }
  else {
    fmt::println("Wrote new image to '{}'", out_filename_path.c_str());
  }


  // When all is done, clean up shared info ptr.
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  for (png_uint_32 y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);
  return 0;
}

