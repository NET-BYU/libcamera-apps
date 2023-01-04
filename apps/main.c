#include "camera.h"

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
	camera_save_to_bmp(buf, "foobar.bmp");

	// Exit camera
	camera_exit();

	return 0;
}
