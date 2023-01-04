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
// 	- Save to memory not file
// 	- Figure out how to compile this as a library

LibcameraJpegApp app;

static void camera_init()
{
	StillOptions *options = app.GetOptions();
	options->Parse(1, {NULL});

	app.OpenCamera();
	app.ConfigureStill();
	app.StartCamera();
}

static void camera_get_still(std::string filename)
{
	StillOptions const *options = app.GetOptions();

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
			return;
		}

		if (msg.type != LibcameraApp::MsgType::RequestComplete)
		{
			throw std::runtime_error("unrecognised message!");
		}

		LOG(1, "Still capture image received");

		Stream *stream = app.StillStream();
		StreamInfo info = app.GetStreamInfo(stream);
		CompletedRequestPtr &payload = std::get<CompletedRequestPtr>(msg.payload);
		const std::vector<libcamera::Span<uint8_t>> mem = app.Mmap(payload->buffers[stream]);
		jpeg_save(mem, info, payload->metadata, filename, app.CameraId(), options);
		return;
	}
}

static void camera_exit()
{
	app.StopCamera();
}

int main(int argc, char *argv[])
{
	try
	{
		std::chrono::milliseconds duration(500);

		camera_init();
		camera_get_still("frame1.jpg");
		std::this_thread::sleep_for(duration);
		camera_get_still("frame2.jpg");
		std::this_thread::sleep_for(duration);
		camera_get_still("frame3.jpg");
		std::this_thread::sleep_for(duration);
		camera_get_still("frame4.jpg");
		std::this_thread::sleep_for(duration);
		camera_get_still("frame5.jpg");
		camera_exit();
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
