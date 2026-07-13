#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "../Constants.hpp"

#include "Video.hpp"

namespace
{
    /**
     * Compute the CRC-32 used to check each PNG chunk.
     */
    uint32_t crc32(const uint8_t* data, size_t length)
    {
        static uint32_t table[256];
        static bool tableComputed = false;
        if (!tableComputed)
        {
            for (uint32_t n = 0; n < 256; n++)
            {
                uint32_t c = n;
                for (int k = 0; k < 8; k++)
                {
                    c = (c & 1) ? (0xedb88320 ^ (c >> 1)) : (c >> 1);
                }
                table[n] = c;
            }
            tableComputed = true;
        }

        uint32_t crc = 0xffffffff;
        for (size_t i = 0; i < length; i++)
        {
            crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
        }
        return crc ^ 0xffffffff;
    }

    /**
     * Compute the Adler-32 checksum required at the end of a zlib stream.
     */
    uint32_t adler32(const uint8_t* data, size_t length)
    {
        uint32_t a = 1;
        uint32_t b = 0;
        for (size_t i = 0; i < length; i++)
        {
            a = (a + data[i]) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }

    void appendBigEndian32(std::vector<uint8_t>& out, uint32_t value)
    {
        out.push_back((value >> 24) & 0xff);
        out.push_back((value >> 16) & 0xff);
        out.push_back((value >> 8) & 0xff);
        out.push_back(value & 0xff);
    }

    void writeChunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data)
    {
        appendBigEndian32(out, static_cast<uint32_t>(data.size()));

        std::vector<uint8_t> typeAndData(type, type + 4);
        typeAndData.insert(typeAndData.end(), data.begin(), data.end());

        out.insert(out.end(), typeAndData.begin(), typeAndData.end());
        appendBigEndian32(out, crc32(typeAndData.data(), typeAndData.size()));
    }
}

void drawBox(uint32_t* buffer, int xOffset, int yOffset, int width, int height, uint32_t palette)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int tile;
            if (y == 0)
            {
                if (x == 0)
                {
                    tile = TILE_BOX_NW;
                }
                else if (x == width - 1)
                {
                    tile = TILE_BOX_NE;
                }
                else
                {
                    tile = TILE_BOX_N;
                }
            }
            else if (y == height - 1)
            {
                if (x == 0)
                {
                    tile = TILE_BOX_SW;
                }
                else if (x == width - 1)
                {
                    tile = TILE_BOX_SE;
                }
                else
                {
                    tile = TILE_BOX_S;
                }
            }
            else
            {
                if (x == 0)
                {
                    tile = TILE_BOX_W;
                }
                else if (x == width - 1)
                {
                    tile = TILE_BOX_E;
                }
                else
                {
                    tile = TILE_BOX_CENTER;
                }
            }
            drawCHRTile(buffer, xOffset + x * 8, yOffset + y * 8, tile, palette);
        }
    }
}

void drawCHRTile(uint32_t* buffer, int xOffset, int yOffset, int tile, uint32_t palette)
{
    // Read the pixels of the tile
    for( int row = 0; row < 8; row++ )
    {
        uint8_t plane1 = romImage[16 + 2 * 16384 + tile * 16 + row];
        uint8_t plane2 = romImage[16 + 2 * 16384 + tile * 16 + row + 8];

        for( int column = 0; column < 8; column++ )
        {
            uint8_t paletteIndex = (((plane1 & (1 << column)) ? 1 : 0) + ((plane2 & (1 << column)) ? 2 : 0));
            if( paletteIndex == 0 )
            {
                // skip transparent pixels
                continue;
            }
            uint32_t pixel;
            if (palette == 0)
            {
                // Grayscale
                pixel = 0xff000000 | (paletteIndex * 0x555555);
            }
            else
            {
                uint32_t colorIndex = (palette >> (8 * (3 - paletteIndex))) & 0xff;
                pixel = 0xff000000 | paletteRGB[colorIndex];
            }

            int x = (xOffset + (7 - column));
            int y = (yOffset + row);
            if (x < 0 || x >= 256 || y < 0 || y >= 240)
            {
                continue;
            }
            buffer[y * 256 + x] = pixel;
        }
    }
}

void drawText(uint32_t* buffer, int xOffset, int yOffset, const std::string& text, uint32_t palette)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        int tile = 256 + 36; // SPACE
        char c = text[i];
        if (c >= '0' && c <= '9')
        {
            tile = 256 + (int)(c - '0');
        }
        else if (c >= 'a' && c <= 'z')
        {
            tile = 256 + 10 + (int)(c - 'a');
        }
        else if (c >= 'A' && c <= 'Z')
        {
            tile = 256 + 10 + (int)(c - 'A');
        }
        else if (c == '-')
        {
            tile = 256 + 40;
        }
        else if (c == '!')
        {
            tile = 256 + 43;
        }
        else if (c == '*')
        {
            tile = 256 + 41;
        }
        else if (c == '$')
        {
            tile = 256 + 46;
        }
        drawCHRTile(buffer, xOffset + i * 8, yOffset, tile, palette);
    }
}

SDL_Texture* generateScanlineTexture(SDL_Renderer* renderer)
{
    // Create a scanline texture for 3x rendering
    SDL_Texture* scanlineTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, RENDER_WIDTH * 3, RENDER_HEIGHT * 3);
    uint32_t* scanlineTextureBuffer = new uint32_t[RENDER_WIDTH * RENDER_HEIGHT * 3 * 3];
    for (int y = 0; y < RENDER_HEIGHT; y++)
    {
        for (int x = 0; x < RENDER_WIDTH; x++)
        {
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    uint32_t color = 0xff000000;
                    switch (j)
                    {
                    case 0:
                        color |= 0xfdd6c7;
                        break;
                    case 1:
                        color |= 0xbef5e1;
                        break;
                    case 2:
                        color |= 0xcfe2ff;
                        break;
                    }
                    scanlineTextureBuffer[((y * 3) + i) * (RENDER_WIDTH * 3) + (x * 3) + j] = color;
                }
            }
        }
    }
    SDL_SetTextureBlendMode(scanlineTexture, SDL_BLENDMODE_MOD);
    SDL_UpdateTexture(scanlineTexture, NULL, scanlineTextureBuffer, sizeof(uint32_t) * RENDER_WIDTH * 3);
    delete [] scanlineTextureBuffer;

    return scanlineTexture;
}

bool writeScreenshot(const std::string& fileName, const uint32_t* buffer, int width, int height)
{
    // Build the raw, filtered image data: one filter-type byte (0 = none)
    // followed by width * 3 bytes of RGB per row.
    //
    std::vector<uint8_t> raw;
    raw.reserve(height * (1 + width * 3));
    for (int y = 0; y < height; y++)
    {
        raw.push_back(0); // filter type: none
        for (int x = 0; x < width; x++)
        {
            uint32_t pixel = buffer[y * width + x];
            raw.push_back((pixel >> 16) & 0xff); // R
            raw.push_back((pixel >> 8) & 0xff);  // G
            raw.push_back(pixel & 0xff);         // B
        }
    }

    // Deflate the image data using uncompressed ("stored") blocks. This
    // produces a valid zlib/PNG stream without depending on zlib, which isn't
    // otherwise a dependency of this project.
    //
    std::vector<uint8_t> zlibStream;
    zlibStream.push_back(0x78);
    zlibStream.push_back(0x01);

    size_t offset = 0;
    do
    {
        size_t blockSize = std::min<size_t>(raw.size() - offset, 65535);
        bool isFinalBlock = (offset + blockSize) >= raw.size();

        zlibStream.push_back(isFinalBlock ? 1 : 0);
        zlibStream.push_back(blockSize & 0xff);
        zlibStream.push_back((blockSize >> 8) & 0xff);
        uint16_t blockSizeComplement = ~static_cast<uint16_t>(blockSize);
        zlibStream.push_back(blockSizeComplement & 0xff);
        zlibStream.push_back((blockSizeComplement >> 8) & 0xff);
        zlibStream.insert(zlibStream.end(), raw.begin() + offset, raw.begin() + offset + blockSize);

        offset += blockSize;
    } while (offset < raw.size());

    appendBigEndian32(zlibStream, adler32(raw.data(), raw.size()));

    // Assemble the PNG file
    //
    std::vector<uint8_t> png;
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    png.insert(png.end(), signature, signature + 8);

    std::vector<uint8_t> ihdr;
    appendBigEndian32(ihdr, static_cast<uint32_t>(width));
    appendBigEndian32(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8); // bit depth
    ihdr.push_back(2); // color type: truecolor (RGB)
    ihdr.push_back(0); // compression method
    ihdr.push_back(0); // filter method
    ihdr.push_back(0); // interlace method
    writeChunk(png, "IHDR", ihdr);

    writeChunk(png, "IDAT", zlibStream);

    writeChunk(png, "IEND", std::vector<uint8_t>());

    std::ofstream file(fileName, std::ios::binary);
    if (!file)
    {
        std::cout << "Failed to open \"" << fileName << "\" for writing." << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()), png.size());

    return true;
}
