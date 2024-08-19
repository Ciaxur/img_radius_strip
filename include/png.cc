#include <cstdint>
#include <cstring>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <netinet/in.h>
#include <string>

#include "png.h"


namespace png {
  int _parse_img_header(std::ifstream& input_filestream, ImageHeader *hdr) {
    input_filestream.seekg( input_filestream.beg );

    input_filestream.read( &hdr->eight_bit_data_support, 1 );
    input_filestream.read( hdr->ascii_png, 3 );
    input_filestream.read( hdr->is_dos_unix_line_ending, 2 );
    input_filestream.read( &hdr->dos_cmd, 1 );
    input_filestream.read( &hdr->unix_line_ending, 1 );

    return 0;
  }

  int _parse_img_chunk(std::ifstream& input_filestream, ImageChunk* chunk) {
    // Chunk starts after 8B file headers.
    input_filestream.seekg(8, input_filestream.beg);

    input_filestream.read( reinterpret_cast<char*>(&chunk->length), 4 );
    input_filestream.read( reinterpret_cast<char*>(&chunk->type), 4 );

    // Convert from big-endian to host order.
    chunk->length = ntohl(chunk->length);

    // Store chunk type as char array such that can be compared and printed.
    char chunk_type[4];
    strncpy(chunk_type, reinterpret_cast<char*>(&chunk->type), 4);

    // NOTE: only IHDR is supported for now!
    if ( strncmp( chunk_type, "IHDR", 4) != 0 ) {
      fmt::println("Header chunk type '{:s}' not supported!", chunk_type);
      return -1;
    }

    // For IHDR, 13B are expected.
    if (chunk->length != 13) {
      fmt::println("Invalid IHDR data size of '{}B'. Expected 13B", chunk->length);
      return -1;
    }

    // Now parse the data.
    input_filestream.read( reinterpret_cast<char*>(&chunk->width), 4 );
    input_filestream.read( reinterpret_cast<char*>(&chunk->height), 4 );
    input_filestream.read( &chunk->bit_depth, 1 );
    input_filestream.read( &chunk->color_type, 1 );
    input_filestream.read( &chunk->compression_method, 1 );
    input_filestream.read( &chunk->filter_method, 1 );
    input_filestream.read( &chunk->interlace_method, 1 );

    // Finally the CRC
    input_filestream.read( reinterpret_cast<char*>(&chunk->crc), 4 );

    // Fix endianness.
    chunk->width  = ntohl(chunk->width);
    chunk->height = ntohl(chunk->height);
    chunk->crc    = ntohl(chunk->crc);

    return 0;
  }

  bool is_idat_type(const IDAT& idat) {
    return strncmp(idat.ascii_type, "IDAT", 4) == 0;
  }

  bool is_iend_type(const IDAT& idat) {
    return strncmp(idat.ascii_type, "IEND", 4) == 0;
  }

  int _parse_img_data(std::ifstream& input_filestream, ImageData* img_data) {
    // Seek past the image headers and chunk.
    // Headers     = 8B
    // IHDR length = 4B
    // IHDR type   = 4B
    // IHDR chunk  = 13B
    // CRC         = 4B
    //             = 33B
    input_filestream.seekg(33, input_filestream.beg);

    while ( input_filestream.tellg() != input_filestream.end ) {
      // Start creating & parsing multiple IDAT frames.
      IDAT& idat = img_data->idat_frames.emplace_back();

      input_filestream.read( reinterpret_cast<char*>(&idat.length), 4 );
      input_filestream.read( reinterpret_cast<char*>(&idat.type), 4 );
      input_filestream.read( &idat.deflate_compression_method, 1 );
      input_filestream.read( &idat.zlib_fcheck, 1 );
      input_filestream.read( idat.compressed_block_huffman_code, 6 );
      input_filestream.read( reinterpret_cast<char*>(&idat.zlib_alder32_checksum), 4 );
      input_filestream.read( reinterpret_cast<char*>(&idat.crc), 4 );

      // Convert from big-endian to host order.
      idat.length = ntohl(idat.length);
      idat.zlib_alder32_checksum = ntohl(idat.zlib_alder32_checksum);
      idat.crc = ntohl(idat.crc);

      // Interpret idat type as human readable.
      strncpy(idat.ascii_type, reinterpret_cast<char*>(&idat.type), 4);
      idat.ascii_type[4] = '\0';

      // Reached the end of the PNG file.
      if (is_iend_type(idat)) {
        return 0;
      }

      if (!is_idat_type(idat)) {
        fmt::println("Header img_data type '{:s}' not supported!", idat.ascii_type);
        return -1;
      }

      idat.data_len_bytes = idat.length - IDAT_LENGTH_BYTES;

      // Consume raw image data.
      idat.data = new char[idat.data_len_bytes];
      input_filestream.read( idat.data, idat.data_len_bytes );
    }

    return 0;
  }

  int parse_img(std::string& filepath, ImagePNG *img) {
    std::ifstream img_if;
    img_if.open(filepath, std::ios::binary | std::ios::in | std::ios::ate);

    // Early return if failed to open.
    if (!img_if.is_open()) return -1;

    // Gather file size.
    img_if.seekg(0, img_if.end);
    img->size_bytes = img_if.tellg();

    // Doesn't even have any headers.
    if (img->size_bytes < 8) {
      fmt::println("Failed to parse header: File size of {}B not sufficient.", img->size_bytes);
      return -1;
    }

    // Parse headers.
    if (_parse_img_header(img_if, &img->header) != 0) {
      fmt::println("Failed to parse image header");
      return -1;
    }

    // Parse critical chunk data.
    if (_parse_img_chunk(img_if, &img->chunk) != 0) {
      fmt::println("Failed to parse image chunk");
      return -1;
    }

    // Parse image data chunk.
    if (_parse_img_data(img_if, &img->idat) != 0) {
      fmt::println("Failed to parse image data");
      return -1;
    }

    img_if.close();
    return 0;
  }

  int free_img_data(ImagePNG *img) {
    for ( auto& frame : img->idat.idat_frames ) {
      delete[] frame.data;
      frame.data = nullptr;
    }
    return 0;
  }

  bool is_valid_png_file(std::string& filepath) {
    std::ifstream img_if;
    img_if.open(filepath, std::ios::binary | std::ios::in | std::ios::ate);

    // Early return if failed to open.
    if (!img_if.is_open()) return false;

    ImageHeader header;
    if (_parse_img_header(img_if, &header) != 0) {
      fmt::println("Failed to parse image header");
      return -1;
    }

    // Simply verify that the header's 3B ASCII PNG indicates so.
    return strncmp(header.ascii_png, "PNG", 3) == 0;
  }
};
