#pragma once
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <vector>

// Docs:
//  - http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
//  - https://en.wikipedia.org/wiki/PNG
namespace png {
  // IDAT frames are offset by 12B each
  #define IDAT_LENGTH_BYTES 12
  #define IDAT_CHUNK_LENGTH_BYTES 13

  // 8 Byte PNG header.
  struct ImageHeader {
    // Has the high bit set to detect transmission systems that do not support 8-bit data.
    char eight_bit_data_support;

    // 3B in ASCII of the letters PNG
    char ascii_png[3];

    // A DOS-style line ending (CRLF) to detect DOS-Unix line ending conversion of the data.
    char is_dos_unix_line_ending[2];

    // A byte that stops display of the file under DOS when the command type has been used—the end-of-file character.
    char dos_cmd;

    // A Unix-style line ending (LF) to detect Unix-DOS line ending conversion.
    char unix_line_ending;
  };

  // Chunk metadata.
  // https://en.wikipedia.org/wiki/PNG#%22Chunks%22_within_the_file
  struct ImageChunk {
    // 4B length in big endian.
    uint32_t length;

    // 4B ASCII type.
    uint32_t type;

    // Chunk data | IHDR expected 13B.
    // width & height are in big endian order.
    uint32_t width;
    uint32_t height;

    char     bit_depth;
    char     color_type;
    char     compression_method;
    char     filter_method;
    char     interlace_method;

    // 4B CRC in network byte order.
    uint32_t crc;
  };

  // Image data.
  struct IDAT {
    // 4B length in big endian.
    uint32_t length;

    // 4B ASCII type.
    uint32_t type;
    char     ascii_type[5]; // Human readable (extra byte for \0)

    char     deflate_compression_method;
    char     zlib_fcheck;
    char     compressed_block_huffman_code[6];

    uint32_t zlib_alder32_checksum;
    uint32_t crc;


    // Image data.
    size_t   data_len_bytes;
    char*    data = nullptr;
  };

  struct ImageData {
    std::vector<IDAT> idat_frames;
  };

  // Parsed PNG image.
  struct ImagePNG {
    ImageHeader   header;
    ImageChunk    chunk;
    ImageData     idat;
    size_t        size_bytes;
  };

  /**
   * Checks whether the type matches IDAT.
   *
   * @param idat Reference to the IDAT instance being checked.
   * @returns boolean indicating IDAT type match.
   */
  bool is_idat_type(const IDAT& idat);

  /**
   * Checks whether the type matches IEND.
   *
   * @param idat Reference to the IDAT instance being checked.
   * @returns boolean indicating IEND type match.
   */
  bool is_iend_type(const IDAT& idat);

  /**
   * Parses the PNG file's header.
   *
   * @param input_filestream The open image's file stream.
   * @param hdr ImageHeader pointer for which to populate.
   *
   * @returns Status code, where non-zero means failure.
   */
  int _parse_img_header(std::ifstream& input_filestream, ImageHeader *hdr);

  /**
   * Parses the PNG file's chunk data.
   *
   * @param input_filestream The open image's file stream.
   * @param chunk ImageChunk pointer for which to populate.
   *
   * @returns Status code, where non-zero means failure.
   */
  int _parse_img_chunk(std::ifstream& input_filestream, ImageChunk* chunk);

  /**
   * Parses the PNG file's data chunk.
   *
   * @param input_filestream The open image's file stream.
   * @param chunk ImageData pointer for which to populate.
   *
   * @returns Status code, where non-zero means failure.
   */
  int _parse_img_data(std::ifstream& input_filestream, ImageData* img_data);

  /**
   * Parses the PNG file.
   *
   * @param filepath Path to PNG file for which to parse.
   * @param img Image pointer for which to populate.
   *
   * @returns Status code, where non-zero means failure.
   */
  int parse_img(std::string& filepath, ImagePNG *img);

  /**
   * Frees PNG data from heap.
   *
   * @param img Image pointer for which to free data of.
   *
   * @returns Status code, where non-zero means failure.
   */
  int free_img_data(ImagePNG *img);

  /**
  * Validates whether the given filepath is a valid PNG image or not.
  * Using https://en.wikipedia.org/wiki/PNG#File_header
  *
  * @param filepath Filepath to the image
  *
  * @returns Boolean indicating the validity
  */
  bool is_valid_png_file(std::string& filepath);
};


