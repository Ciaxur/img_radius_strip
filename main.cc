#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <stdexcept>
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

  // Radius value.
  size_t radius = 0;
};

void print_help() {
  fmt::println("USAGE:");
  fmt::println("  app [OPTIONS] FILEPATH");

  fmt::println("\nDESCRIPTION\n");
  fmt::println("  Creates a transparent corder radius around a given PNG image\n");

  fmt::println("  -r RADIUS");
  fmt::println("    radius to apply on the given image");
}

int parse_args(int argc, char** argv, CommandLineArgs *cli_args) {
  // Ensure required args are available.
  if (argc < 2) {
    fmt::println("Insufficient number of arguments");
    print_help();
    return 1;
  }

  // Parse required positional and flags.
  for ( int i = 1; i < argc; i++ ) {
    if ( std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0 ) {
      print_help();
      return -1;
    }

    else if ( std::strncmp(argv[i], "-r", 2) == 0 ) {
      // Make sure there's a follow up argument for the value.
      if ( i + 1 == argc ) {
        fmt::println("Invalid raidius argument. Expected radius value after flag");
        print_help();
        return 1;
      }

      // Parse radius value.
      try {
        cli_args->radius = std::stoull(argv[i + 1]);
      } catch( std::invalid_argument& ) {
        fmt::println("Invalid radius value! Expected integer value but got '{}'", argv[i + 1]);
        return -1;
      }

      // Shift argv.
      ++i;
    }

    // Positional argument for filepath.
    else {
      cli_args->img_filepath = std::string{argv[i]};
    }
  }

  // Ensure required args are passed in.
  if (cli_args->img_filepath == "") {
    fmt::println("No required image filepath was given!");
    print_help();
    return 1;
  } else if (cli_args->radius == 0) {
    fmt::println("No required radius was given!");
    print_help();
    return 1;
  }

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

/**
* Checks whether the given point is inside the circle.
*
* @param point The current point being checked
* @param circle_midpoint Point in the center of the circle
* @param radius Circle's radius
*
* @returns Boolean indicating whether the point is in the circle
*/
bool is_inside_circle(Vector2D<ssize_t> point, Vector2D<ssize_t> circle_midpoint, ssize_t radius) {
  ssize_t a = std::labs( point.x - circle_midpoint.x );
  ssize_t b = std::labs( point.y - circle_midpoint.y );
  ssize_t c = std::sqrt( (a*a) + (b*b) );
  return c < radius;
}

/**
* Helper function for drawing a simple circle at a given midpoint.
*
* @param circle_midpoint Vector2D mindpoint for which to draw the circle
* @param radius Circle radius
* @param channels Number of channels the png has
* @param point_percision Circle point point_percision | number of pooints
* @param row_pointers PNG pixels
*/
void draw_circle(Vector2D<ssize_t> circle_midpoint, int radius, png_byte channels, ssize_t point_percision, png_bytepp& row_pointers) {
  double angle_inc = 2 * M_PI / point_percision;

  for (ssize_t i = 0; i < point_percision; i++) {
    double angle = i * angle_inc;
    double x = radius * std::cos(angle);
    double y = radius * std::sin(angle);

    // Translate onto image.
    ssize_t dx = circle_midpoint.x + static_cast<ssize_t>(std::floor(x));
    ssize_t dy = circle_midpoint.y + static_cast<ssize_t>(std::floor(y));
    png_bytep row = row_pointers[dy];
    png_bytep px = &(row[dx * channels]);

    px[0] = 0;   // Red
    px[1] = 0;   // Green
    px[2] = 0;   // Blue
    px[3] = 255; // Alpha
  }
}

/**
* Applies a transparent radius around a given image.
*
* @param radius_px Radius to apply to image
* @param png_ptr Pointer to the PNG image struct
* @param info_ptr Pointer to the PNG image info struct
* @param row_pointers Pointer to the PNG image pixels.
*
* @returns Status of applying the radius to the image.
*/
int apply_radius(int radius_px, png_structp& png_ptr, png_infop& info_ptr, png_bytepp& row_pointers) {
  ssize_t width     = png_get_image_width(png_ptr, info_ptr);
  ssize_t height    = png_get_image_height(png_ptr, info_ptr);
  png_byte channels = png_get_channels(png_ptr, info_ptr);

  // This only works with RGBA images.
  if (channels != 4) {
    fmt::println("Failed to apply radius around image. Image has {} channels, expected 4 channels for RGBA", channels);
    return -1;
  }

  // Create a circle on each edge.
  // Top left.
  {
    Vector2D<ssize_t> midpoint = { radius_px, radius_px };

    for (int y = 0; y < radius_px; y++) {
      png_bytep row = row_pointers[y];
      for (int x = 0; x < radius_px; x++) {
        // Only update the pixel within the circle radius.
        if (!is_inside_circle({ x, y }, midpoint, radius_px)) {
          png_bytep px = &(row[x * channels]);

          // Make transparent!
          px[0] = 0; // Red
          px[1] = 0; // Green
          px[2] = 0; // Blue
          px[3] = 0; // Alpha
        }
      }
    }
  }

  // Top right.
  {
    ssize_t x0 = width - 1 - radius_px;
    Vector2D<ssize_t> midpoint = { x0, radius_px };

    for (int y = 0; y < radius_px; y++) {
      png_bytep row = row_pointers[y];
      for (int x = x0; x < width; x++) {
        // Only update the pixel within the circle radius.
        if (!is_inside_circle({ x, y }, midpoint, radius_px)) {
          png_bytep px = &(row[x * channels]);

          // Make transparent!
          px[0] = 0; // Red
          px[1] = 0; // Green
          px[2] = 0; // Blue
          px[3] = 0; // Alpha
        }
      }
    }
  }

  // Bottom right.
  {
    ssize_t x0 = width - 1 - radius_px;
    ssize_t y0 = height - 1 - radius_px;
    Vector2D<ssize_t> midpoint = { x0, y0 };

    for (int y = y0; y < height; y++) {
      png_bytep row = row_pointers[y];
      for (int x = x0; x < width; x++) {
        // Only update the pixel within the circle radius.
        if (!is_inside_circle({ x, y }, midpoint, radius_px)) {
          png_bytep px = &(row[x * channels]);

          // Make transparent!
          px[0] = 0; // Red
          px[1] = 0; // Green
          px[2] = 0; // Blue
          px[3] = 0; // Alpha
        }
      }
    }
  }

  // Bottom left.
  {
    ssize_t y0 = height - 1 - radius_px;
    Vector2D<ssize_t> midpoint = { radius_px, y0 };

    for (int y = y0; y < height; y++) {
      png_bytep row = row_pointers[y];
      for (int x = 0; x < radius_px; x++) {
        // Only update the pixel within the circle radius.
        if (!is_inside_circle({ x, y }, midpoint, radius_px)) {
          png_bytep px = &(row[x * channels]);

          // Make transparent!
          px[0] = 0; // Red
          px[1] = 0; // Green
          px[2] = 0; // Blue
          px[3] = 0; // Alpha
        }
      }
    }
  }

  return 0;
}


// TODO: add some more checks.
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
  if (apply_radius(100, png_ptr, info_ptr, row_pointers) == 0) {
    // Write image.
    std::filesystem::path out_filename_path{"out.png"};

    if (write_png_file(out_filename_path.c_str(), info_ptr, row_pointers) != 0) {
      fmt::println("Failed to write image");
    }
    else {
      fmt::println("Wrote new image to '{}'", out_filename_path.c_str());
    }
  }

  // When all is done, clean up shared info ptr.
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  for (png_uint_32 y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);
  return 0;
}

