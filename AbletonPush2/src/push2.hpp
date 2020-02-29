#ifdef __linux__
#include <libusb-1.0/libusb.h>
#else
#include "libusb.h"
#endif

#define ABLETON_VENDOR_ID 0x2982
#define PUSH2_PRODUCT_ID  0x1967

#define PUSH2_BULK_EP_OUT 0x01
#define TRANSFER_TIMEOUT  1000 // milliseconds

// void on_frame_header_transfer_finished(libusb_transfer* tr) {

// }

// void on_buffer_transfer_finished(libusb_transfer* tr) {

// }


class Push2 {

public:

  libusb_device_handle* device_handle;
  unsigned char* image;

  Push2() {
    device_handle = NULL;
    image = (unsigned char*)malloc(960*160*4);
  }

  int open_push2_device()
  {
    int result;

    if ((result = libusb_init(NULL)) < 0)
    {
      printf("error: [%d] could not initilialize usblib\n", result);
      return NULL;
    }

    libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);
    
    libusb_device** devices;
    ssize_t count;
    count = libusb_get_device_list(NULL, &devices);
    if (count < 0)
    {
      printf("error: [%ld] could not get usb device list\n", count);
      return NULL;
    }

    libusb_device* device;

    char ErrorMsg[128];

    // set message in case we get to the end of the list w/o finding a device
    sprintf(ErrorMsg, "error: Ableton Push 2 device not found\n");

    for (int i = 0; (device = devices[i]) != NULL; i++)
    {
      struct libusb_device_descriptor descriptor;
      if ((result = libusb_get_device_descriptor(device, &descriptor)) < 0)
      {
        sprintf(ErrorMsg,
          "error: [%d] could not get usb device descriptor\n", result);
        continue;
      }

      if (descriptor.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE
        && descriptor.idVendor == ABLETON_VENDOR_ID
        && descriptor.idProduct == PUSH2_PRODUCT_ID)
      {
        if ((result = libusb_open(device, &device_handle)) < 0)
        {
          sprintf(ErrorMsg,
            "error: [%d] could not open Ableton Push 2 device\n", result);
        }
        else if ((result = libusb_claim_interface(device_handle, 0)) < 0)
        {
          sprintf(ErrorMsg,
            "error: [%d] could not claim interface 0 of Push 2 device\n", result);
          libusb_release_interface(device_handle, 0);
          libusb_close(device_handle);
          device_handle = NULL;
        }
        else
        {
          break; // successfully opened
        }
      }
    }

    if (device_handle == NULL)
    {
      printf(ErrorMsg);
      return 0;
    }

    libusb_free_device_list(devices, 1);

    return 1;
  }

  void close_push2_device()
  {
    libusb_release_interface(device_handle, 0);
    libusb_close(device_handle);
  }

  void sendDisplay(unsigned char * image) {
      unsigned char buffer_to_be_transferred[2048]; //one line of blank pixels
      int actual_length;
      unsigned char frame_header[16] = {
          0xFF, 0xCC, 0xAA, 0x88,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00 };

      if (device_handle == nullptr) {
        printf("no device\n");
      }
      else { //print one frame
        libusb_bulk_transfer( //push out frame header
          device_handle,
          PUSH2_BULK_EP_OUT,
          frame_header,
          sizeof(frame_header),
          &actual_length,
          TRANSFER_TIMEOUT);
        for (int i = 0; i < 160; i++) { //print 160 lines of pixels
          libusb_bulk_transfer(
            device_handle,
            PUSH2_BULK_EP_OUT,
            &image[(159 - i)*1920],
            2048,
            &actual_length,
            TRANSFER_TIMEOUT);
          }
      }
  }

  void drawDisplay(NVGContext * vg) {
    glViewport(0, 0, 960, 160);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, 960,  160, 1);

    // nvgBeginPath(vg);
    // nvgFontSize(vg, 40.0f);
    // nvgFontFace(vg, "sans");
    // nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    // nvgText(vg, 300, 50, "Hello from VCV Rack", NULL);
    // nvgFillColor(vg, nvgRGB(255,255,255));
    // nvgFill(vg);
    // nvgClosePath(vg);


    nvgBeginPath(vg);
    nvgCircle(vg, counter, 100, 10);
    nvgFillColor(vg, nvgRGBA(255,50,50,255));
    nvgFill(vg);
    nvgClosePath(vg);

    counter = (counter + 15) % 960;
    
    nvgEndFrame(vg);

    glReadPixels(0, 0, 960, 160, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image); 

    for (int i = 0; i < 80*960; i ++){
      image[i * 4] ^= 0xE7;
      image[i * 4 + 1] ^= 0xF3;
      image[i * 4 + 2] ^= 0xE7;
      image[i * 4 + 3] ^= 0xFF;
    }

    push2->sendDisplay(image);

  }

};