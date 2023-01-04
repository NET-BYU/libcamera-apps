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
Stream *stream;
StreamInfo info;

void camera_init()
{
	StillOptions *options = app.GetOptions();
	options->Parse(1, {NULL});

	unsigned int still_flags = LibcameraApp::FLAG_STILL_NONE;
	still_flags |= LibcameraApp::FLAG_STILL_RGB;

	app.OpenCamera();
	app.ConfigureStill(still_flags);
	app.StartCamera();

	stream = app.StillStream();
	info = app.GetStreamInfo(stream);
}

void save_to_bmp(uint8_t* mem, std::string const &filename)
{
	FILE *fp = filename == "-" ? stdout : fopen(filename.c_str(), "wb");

	if (fp == NULL)
		throw std::runtime_error("failed to open file " + filename);

	try
	{
		unsigned int line = info.width * 3;
		unsigned int pitch = (line + 3) & ~3; // lines are multiples of 4 bytes
		unsigned int pad = pitch - line;
		uint8_t padding[3] = {};
		uint8_t *ptr = mem;

		FileHeader file_header;
		ImageHeader image_header;
		file_header.filesize = file_header.offset + info.height * pitch;
		image_header.width = info.width;
		image_header.height = -info.height; // make image come out the right way up

		// Don't write the file header's 2 dummy bytes
		if (fwrite((uint8_t *)&file_header + 2, sizeof(file_header) - 2, 1, fp) != 1 ||
			fwrite(&image_header, sizeof(image_header), 1, fp) != 1)
			throw std::runtime_error("failed to write BMP file");

		for (unsigned int i = 0; i < info.height; i++, ptr += info.stride)
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

int camera_get_still(uint8_t* buf)
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

		return 0;
	}
}

unsigned int camera_get_width() {
	return info.width;
}

unsigned int camera_get_height() {
	return info.height;
}

void camera_exit()
{
	app.StopCamera();
}

int main(int argc, char *argv[])
{
	// Start camera
	camera_init();

	// Set up buffer for image data
	unsigned int buf_size = camera_get_width() * camera_get_height() * 3;
	uint8_t *buf = (uint8_t *)malloc(buf_size);

	// Get a still
	if(camera_get_still(buf) == -1) {
		fprintf(stderr, "Unable to get still image...\n");
		camera_exit();
		return 1;
	}

	// Save the still to an file
	save_to_bmp(buf, "foobar.bmp");

	// Exit camera
	camera_exit();

	return 0;
}
