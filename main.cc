#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>

#include "include/png.h"


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

  // Verify valid PNG file.
  if (!png::is_valid_png_file(cli_args.img_filepath)) {
    fmt::println("Please provide a valid PNG image. '{}' is not a PNG image!", cli_args.img_filepath);
    return 1;
  }

  png::ImagePNG img;
  png::parse_img(cli_args.img_filepath, &img);

  // Clean up.
  png::free_img_data(&img);

  return 0;
}

