#include "Image.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
// ---------------------------------------------------------------
// DEFLATE (RFC 1951) decoder
// ---------------------------------------------------------------
struct BitReader
{
	const uint8_t* data;
	size_t size;
	size_t pos = 0;
	uint32_t bitBuf = 0;
	int bitCnt = 0;

	int bits(int need)
	{
		uint32_t val = bitBuf;
		while (bitCnt < need)
		{
			if (pos >= size) throw std::runtime_error("unexpected end of compressed data");
			val |= (uint32_t)data[pos++] << bitCnt;
			bitCnt += 8;
		}
		bitBuf = val >> need;
		bitCnt -= need;
		return (int)(val & ((1u << need) - 1u));
	}
};

struct Huffman
{
	short count[16];     // number of codes of each length
	short symbol[288];   // symbols ordered by code
};

void buildHuffman(Huffman& h, const short* lengths, int n)
{
	for (int i = 0; i < 16; i++) h.count[i] = 0;
	for (int s = 0; s < n; s++) h.count[lengths[s]]++;

	short offs[16];
	offs[1] = 0;
	for (int len = 1; len < 15; len++) offs[len + 1] = offs[len] + h.count[len];
	for (int s = 0; s < n; s++)
		if (lengths[s] != 0) h.symbol[offs[lengths[s]]++] = (short)s;
}

int decodeSymbol(BitReader& br, const Huffman& h)
{
	int code = 0, first = 0, index = 0;
	for (int len = 1; len <= 15; len++)
	{
		code |= br.bits(1);
		int count = h.count[len];
		if (code - count < first) return h.symbol[index + (code - first)];
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	throw std::runtime_error("invalid Huffman code");
}

const short LEN_BASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
const short LEN_EXTRA[29] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
const short DIST_BASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
const short DIST_EXTRA[30] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

void inflateBlock(BitReader& br, std::vector<uint8_t>& out, const Huffman& lit, const Huffman& dist)
{
	for (;;)
	{
		int sym = decodeSymbol(br, lit);
		if (sym < 256) { out.push_back((uint8_t)sym); continue; }
		if (sym == 256) return;

		sym -= 257;
		if (sym >= 29) throw std::runtime_error("bad length symbol");
		int len = LEN_BASE[sym] + br.bits(LEN_EXTRA[sym]);

		int ds = decodeSymbol(br, dist);
		if (ds >= 30) throw std::runtime_error("bad distance symbol");
		size_t d = (size_t)DIST_BASE[ds] + br.bits(DIST_EXTRA[ds]);
		if (d > out.size()) throw std::runtime_error("distance too far back");

		size_t from = out.size() - d;
		for (int i = 0; i < len; i++) out.push_back(out[from + i]);
	}
}

// zlib stream (2 byte header + deflate data)
std::vector<uint8_t> zlibInflate(const std::vector<uint8_t>& in, size_t expected)
{
	if (in.size() < 2) throw std::runtime_error("zlib stream too short");
	BitReader br{ in.data() + 2, in.size() - 2 };
	std::vector<uint8_t> out;
	out.reserve(expected);

	int last;
	do
	{
		last = br.bits(1);
		int type = br.bits(2);

		if (type == 0)                                  // stored
		{
			br.bitBuf = 0; br.bitCnt = 0;               // skip to byte boundary
			if (br.pos + 4 > br.size) throw std::runtime_error("bad stored block");
			unsigned len = br.data[br.pos] | (br.data[br.pos + 1] << 8);
			br.pos += 4;
			if (br.pos + len > br.size) throw std::runtime_error("bad stored block");
			out.insert(out.end(), br.data + br.pos, br.data + br.pos + len);
			br.pos += len;
		}
		else if (type == 1)                             // fixed Huffman
		{
			short lengths[288];
			for (int i = 0; i < 144; i++) lengths[i] = 8;
			for (int i = 144; i < 256; i++) lengths[i] = 9;
			for (int i = 256; i < 280; i++) lengths[i] = 7;
			for (int i = 280; i < 288; i++) lengths[i] = 8;
			Huffman lit, dist;
			buildHuffman(lit, lengths, 288);
			short dl[30];
			for (int i = 0; i < 30; i++) dl[i] = 5;
			buildHuffman(dist, dl, 30);
			inflateBlock(br, out, lit, dist);
		}
		else if (type == 2)                             // dynamic Huffman
		{
			int nlen = br.bits(5) + 257;
			int ndist = br.bits(5) + 1;
			int ncode = br.bits(4) + 4;
			if (nlen > 286 || ndist > 30) throw std::runtime_error("bad dynamic block");

			static const short ORDER[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
			short lengths[320];
			for (int i = 0; i < 19; i++) lengths[i] = 0;
			for (int i = 0; i < ncode; i++) lengths[ORDER[i]] = (short)br.bits(3);
			Huffman lencode;
			buildHuffman(lencode, lengths, 19);

			int idx = 0;
			while (idx < nlen + ndist)
			{
				int sym = decodeSymbol(br, lencode);
				if (sym < 16) lengths[idx++] = (short)sym;
				else
				{
					short prev = 0; int rep;
					if (sym == 16)
					{
						if (idx == 0) throw std::runtime_error("bad repeat");
						prev = lengths[idx - 1];
						rep = 3 + br.bits(2);
					}
					else if (sym == 17) rep = 3 + br.bits(3);
					else rep = 11 + br.bits(7);
					if (idx + rep > nlen + ndist) throw std::runtime_error("too many lengths");
					while (rep--) lengths[idx++] = prev;
				}
			}
			Huffman lit, dist;
			buildHuffman(lit, lengths, nlen);
			buildHuffman(dist, lengths + nlen, ndist);
			inflateBlock(br, out, lit, dist);
		}
		else throw std::runtime_error("invalid block type");
	} while (!last);

	return out;
}

// ---------------------------------------------------------------
// PNG
// ---------------------------------------------------------------
uint32_t be32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

int paeth(int a, int b, int c)
{
	int p = a + b - c;
	int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
	if (pa <= pb && pa <= pc) return a;
	return (pb <= pc) ? b : c;
}

bool decodePNG(const std::vector<uint8_t>& file, Image& img)
{
	static const uint8_t SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	if (file.size() < 8 || std::memcmp(file.data(), SIG, 8) != 0) return false;

	uint32_t w = 0, h = 0;
	int bitDepth = 0, colorType = 0, interlace = 0;
	std::vector<uint8_t> idat, palette;

	size_t p = 8;
	while (p + 12 <= file.size())
	{
		uint32_t len = be32(&file[p]);
		std::string type((const char*)&file[p + 4], 4);
		const uint8_t* d = &file[p + 8];
		if (p + 12 + len > file.size()) throw std::runtime_error("truncated PNG chunk");

		if (type == "IHDR")
		{
			w = be32(d); h = be32(d + 4);
			bitDepth = d[8]; colorType = d[9]; interlace = d[12];
		}
		else if (type == "PLTE") palette.assign(d, d + len);
		else if (type == "IDAT") idat.insert(idat.end(), d, d + len);
		else if (type == "IEND") break;
		p += 12 + len;
	}

	if (w == 0 || h == 0) throw std::runtime_error("PNG without IHDR");
	if (interlace != 0) throw std::runtime_error("interlaced PNG is not supported");
	if (bitDepth != 8 && bitDepth != 16) throw std::runtime_error("only 8 and 16 bit PNG are supported");

	int channels;
	switch (colorType)
	{
	case 0: channels = 1; break;   // grey
	case 2: channels = 3; break;   // RGB
	case 3: channels = 1; break;   // palette
	case 4: channels = 2; break;   // grey + alpha
	case 6: channels = 4; break;   // RGBA
	default: throw std::runtime_error("unknown PNG colour type");
	}
	if (colorType == 3 && bitDepth != 8) throw std::runtime_error("only 8 bit palette PNG is supported");

	int bytesPer = channels * (bitDepth / 8);
	size_t stride = (size_t)w * bytesPer;
	std::vector<uint8_t> raw = zlibInflate(idat, (stride + 1) * h);
	if (raw.size() < (stride + 1) * h) throw std::runtime_error("PNG image data too short");

	// undo the scanline filters in place
	std::vector<uint8_t> pix(stride * h);
	for (uint32_t y = 0; y < h; y++)
	{
		int ft = raw[y * (stride + 1)];
		const uint8_t* src = &raw[y * (stride + 1) + 1];
		uint8_t* cur = &pix[y * stride];
		const uint8_t* up = (y > 0) ? &pix[(y - 1) * stride] : nullptr;

		for (size_t x = 0; x < stride; x++)
		{
			int a = (x >= (size_t)bytesPer) ? cur[x - bytesPer] : 0;
			int b = up ? up[x] : 0;
			int c = (up && x >= (size_t)bytesPer) ? up[x - bytesPer] : 0;
			int v = src[x];
			switch (ft)
			{
			case 0: break;
			case 1: v += a; break;
			case 2: v += b; break;
			case 3: v += (a + b) / 2; break;
			case 4: v += paeth(a, b, c); break;
			default: throw std::runtime_error("bad PNG filter");
			}
			cur[x] = (uint8_t)v;
		}
	}

	// convert to RGB8 (alpha is dropped, 16 bit -> high byte)
	img.width = (int)w; img.height = (int)h;
	img.rgb.resize((size_t)w * h * 3);
	int step = bitDepth / 8;                       // bytes per sample
	for (uint32_t y = 0; y < h; y++)
		for (uint32_t x = 0; x < w; x++)
		{
			const uint8_t* s = &pix[y * stride + (size_t)x * bytesPer];
			uint8_t* o = &img.rgb[((size_t)y * w + x) * 3];
			switch (colorType)
			{
			case 0: case 4: o[0] = o[1] = o[2] = s[0]; break;
			case 2: case 6: o[0] = s[0]; o[1] = s[step]; o[2] = s[2 * step]; break;
			case 3:
			{
				size_t i = s[0];
				if (i * 3 + 2 >= palette.size()) throw std::runtime_error("bad palette index");
				o[0] = palette[i * 3]; o[1] = palette[i * 3 + 1]; o[2] = palette[i * 3 + 2];
				break;
			}
			}
		}
	return true;
}

bool decodePPM(const std::vector<uint8_t>& file, Image& img)
{
	if (file.size() < 3 || file[0] != 'P' || file[1] != '6') return false;
	size_t p = 2;
	int vals[3], got = 0;
	while (got < 3 && p < file.size())
	{
		if (file[p] == '#') { while (p < file.size() && file[p] != '\n') p++; continue; }
		if (file[p] >= '0' && file[p] <= '9')
		{
			int v = 0;
			while (p < file.size() && file[p] >= '0' && file[p] <= '9') v = v * 10 + (file[p++] - '0');
			vals[got++] = v;
		}
		else p++;
	}
	p++;                                            // single whitespace after maxval
	if (got < 3 || vals[2] != 255) return false;
	size_t n = (size_t)vals[0] * vals[1] * 3;
	if (p + n > file.size()) return false;
	img.width = vals[0]; img.height = vals[1];
	img.rgb.assign(file.begin() + p, file.begin() + p + n);
	return true;
}
}

bool loadImage(const std::string& path, Image& img)
{
	std::ifstream in(path, std::ios::binary);
	if (!in) { std::cerr << "Cannot open image: " << path << std::endl; return false; }
	std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	try
	{
		if (decodePNG(file, img) || decodePPM(file, img))
		{
			std::cout << "Loaded image " << path << " (" << img.width << "x" << img.height << ")" << std::endl;
			return true;
		}
		std::cerr << "Unknown image format (use PNG or PPM): " << path << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Cannot decode " << path << ": " << e.what() << std::endl;
	}
	return false;
}

Image makeProceduralImage(int width, int height)
{
	Image img;
	img.width = width; img.height = height;
	img.rgb.resize((size_t)width * height * 3);

	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			float u = (float)x / (width - 1), v = (float)y / (height - 1);
			float cx = u - 0.5f, cy = v - 0.5f;
			float r = std::sqrt(cx * cx + cy * cy);
			float ang = std::atan2(cy, cx);

			// swirl: the phase depends on radius and angle
			float t = 6.0f * r + 0.9f * std::sin(5.0f * ang + 14.0f * r) + 0.6f * std::sin(9.0f * u + 4.0f * std::sin(7.0f * v));

			float c[3];
			for (int k = 0; k < 3; k++)
				c[k] = 0.5f + 0.5f * std::cos(6.2831853f * (0.18f * t + 0.33f * k + 0.1f));

			float ring = 0.75f + 0.25f * std::cos(24.0f * r);       // soft concentric shading
			uint8_t* o = &img.rgb[((size_t)y * width + x) * 3];
			for (int k = 0; k < 3; k++)
				o[k] = (uint8_t)std::lround(255.0f * std::min(1.0f, std::max(0.0f, c[k] * ring)));
		}
	return img;
}
