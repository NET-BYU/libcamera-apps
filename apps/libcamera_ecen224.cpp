/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_jpeg.cpp - minimal libcamera jpeg capture app.
 */

#include <chrono>

#include "core/libcamera_app.hpp"
#include "core/still_options.hpp"

#include "image/image.hpp"

using namespace std::placeholders;
using libcamera::Stream;

class LibcameraJpegApp : public LibcameraApp
{
public:
	LibcameraJpegApp() : LibcameraApp(std::make_unique<StillOptions>()) {}

	StillOptions *GetOptions() const { return static_cast<StillOptions *>(options_.get()); }
};

// TODO:
// 	x Remove app
//	x How not to parse options
// 	x Save to memory not file
//	x Figure out how to save data to a image
// 	- Figure out how to compile this as a library

struct ImageHeader
{
	uint32_t size = sizeof(ImageHeader);
	uint32_t width;
	int32_t height;
	uint16_t planes = 1;
	uint16_t bitcount = 24;
	uint32_t compression = 0;
	uint32_t imagesize = 0;
	uint32_t xpels = 100000;
	uint32_t ypels = 100000;
	uint32_t clrused = 0;
	uint32_t clrimportant = 0;
};
static_assert(sizeof(ImageHeader) == 40, "ImageHeader size wrong");

struct FileHeader
{
	uint16_t dummy; // 2 dummy bytes so that our uint32_ts line up
	uint8_t type1 = 'B';
	uint8_t type2 = 'M';
	uint32_t filesize;
	uint16_t reserved1 = 0;
	uint16_t reserved2 = 0;
	uint32_t offset = sizeof(FileHeader) - 2 + sizeof(ImageHeader);
};
static_assert(sizeof(FileHeader) == 16, "FileHeader size wrong");

LibcameraJpegApp app;

static void camera_init()
{
	StillOptions *options = app.GetOptions();
	options->Parse(1, {NULL});

	unsigned int still_flags = LibcameraApp::FLAG_STILL_NONE;
	still_flags |= LibcameraApp::FLAG_STILL_RGB;

	app.OpenCamera();
	app.ConfigureStill(still_flags);
	app.StartCamera();
}

void save_to_bmp(uint8_t* mem, unsigned int width, unsigned int height, unsigned int stride,
			  std::string const &filename)
{
	FILE *fp = filename == "-" ? stdout : fopen(filename.c_str(), "wb");

	if (fp == NULL)
		throw std::runtime_error("failed to open file " + filename);

	try
	{
		unsigned int line = width * 3;
		unsigned int pitch = (line + 3) & ~3; // lines are multiples of 4 bytes
		unsigned int pad = pitch - line;
		uint8_t padding[3] = {};
		uint8_t *ptr = mem;

		FileHeader file_header;
		ImageHeader image_header;
		file_header.filesize = file_header.offset + height * pitch;
		image_header.width = width;
		image_header.height = -height; // make image come out the right way up

		// Don't write the file header's 2 dummy bytes
		if (fwrite((uint8_t *)&file_header + 2, sizeof(file_header) - 2, 1, fp) != 1 ||
			fwrite(&image_header, sizeof(image_header), 1, fp) != 1)
			throw std::runtime_error("failed to write BMP file");

		for (unsigned int i = 0; i < height; i++, ptr += stride)
		{
			if (fwrite(ptr, line, 1, fp) != 1 || (pad != 0 && fwrite(padding, pad, 1, fp) != 1))
				throw std::runtime_error("failed to write BMP file, row " + std::to_string(i));
		}

		LOG(2, "Wrote " << file_header.filesize << " bytes to BMP file");

		if (fp != stdout)
			fclose(fp);
	}
	catch (std::exception const &e)
	{
		if (fp && fp != stdout)
			fclose(fp);
		throw;
	}
}

static int camera_get_still(uint8_t* buf, unsigned int* width, unsigned int* height, unsigned int* stride)
{
	for (;;)
	{
		LibcameraApp::Msg msg = app.Wait();
		if (msg.type == LibcameraApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}

		if (msg.type == LibcameraApp::MsgType::Quit)
		{
			return -1;
		}

		if (msg.type != LibcameraApp::MsgType::RequestComplete)
		{
			return -1;
		}

		Stream *stream = app.StillStream();
		StreamInfo info = app.GetStreamInfo(stream);

		if (info.pixel_format != libcamera::formats::RGB888)
		{
			fprintf(stderr, "Pixel format should be RGB\n");
			return -1;
		}

		CompletedRequestPtr &payload = std::get<CompletedRequestPtr>(msg.payload);
		const std::vector<libcamera::Span<uint8_t>> mem = app.Mmap(payload->buffers[stream]);
		libcamera::Span<uint8_t> image_data = mem[0];

		// Update parameters
		memcpy(buf, image_data.data(), image_data.size());
		*width = info.width;
		*height = info.height;
		*stride = info.stride;

		return 0;
	}
}

static void camera_exit()
{
	app.StopCamera();
}

int main(int argc, char *argv[])
{
	uint8_t *buf = (uint8_t *)malloc(15116544);
	unsigned int width;
	unsigned int height;
	unsigned int stride;

	// Start camera
	camera_init();

	// Get a still
	if(camera_get_still(buf, &width, &height, &stride) == -1) {
		fprintf(stderr, "Unable to get still image...\n");
		exit(1);
	}

	// Save the still to an file
	save_to_bmp(buf, width, height, stride, "foobar.bmp");

	// Exit camera
	camera_exit();

	return 0;
}
