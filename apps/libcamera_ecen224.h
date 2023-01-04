#ifndef __LIBCAMERA_ECEN224_H
#define __LIBCAMERA_ECEN224_H

int cam_init();

int cam_exit();

int cam_get_still(uint8_t* buf);

unsigned int cam_get_width();

unsigned int cam_get_height();

void cam_save_to_bmp(uint8_t* mem, std::string const &filename);

#endif
