#ifndef __CAMERA_H
#define __CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

int camera_init();

int camera_exit();

int camera_get_still(uint8_t* buf);

unsigned int camera_get_width();

unsigned int camera_get_height();

void camera_save_to_bmp(uint8_t* mem, char* filename);



#ifdef __cplusplus
}
#endif


#endif
