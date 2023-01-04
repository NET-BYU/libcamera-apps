#include "libcamera_ecen224.h"
#include "camera.h"

extern "C" {
    int camera_init()
    {
        return cam_init();
    }

    int camera_exit()
    {
        return cam_exit();
    }

    int camera_get_still(uint8_t* buf)
    {
        return cam_get_still(buf);
    }

    unsigned int camera_get_width()
    {
        return cam_get_width();
    }

    unsigned int camera_get_height()
    {
        return cam_get_height();
    }

    void camera_save_to_bmp(uint8_t* mem, char* filename) {
        cam_save_to_bmp(mem, filename);
    }
}
