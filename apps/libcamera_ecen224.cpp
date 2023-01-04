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
	LibcameraJpegApp()
		: LibcameraApp(std::make_unique<StillOptions>())
	{
	}

	StillOptions *GetOptions() const
	{
		return static_cast<StillOptions *>(options_.get());
	}
};

// The main even loop for the application.

static void event_loop(LibcameraJpegApp &app)
{
	StillOptions const *options = app.GetOptions();
	app.OpenCamera();
	app.ConfigureStill();
	app.StartCamera();

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
			
		app.StopCamera();
		LOG(1, "Still capture image received");

		Stream *stream = app.StillStream();
		StreamInfo info = app.GetStreamInfo(stream);
		CompletedRequestPtr &payload = std::get<CompletedRequestPtr>(msg.payload);
		const std::vector<libcamera::Span<uint8_t>> mem = app.Mmap(payload->buffers[stream]);
		jpeg_save(mem, info, payload->metadata, "banana.jpg", app.CameraId(), options);
		return;
	}
}

int main(int argc, char *argv[])
{
	try
	{
		LibcameraJpegApp app;
		StillOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
