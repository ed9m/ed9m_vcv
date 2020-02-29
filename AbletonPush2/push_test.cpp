#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#ifdef _WIN32

// see following link for a discussion of the
// warning suppression:
// http://sourceforge.net/mailarchive/forum.php?
// thread_name=50F6011C.2020000%40akeo.ie&forum_name=libusbx-devel

// Disable: warning C4200: nonstandard extension used:
// zero-sized array in struct/union
#pragma warning(disable:4200)

#include <windows.h>
#endif

#ifdef __linux__
#include <libusb-1.0/libusb.h>
#else
#include "libusb.h"
#endif

#define ABLETON_VENDOR_ID 0x2982
#define PUSH2_PRODUCT_ID  0x1967

#define PUSH2_BULK_EP_OUT 0x01
#define TRANSFER_TIMEOUT  1000 // milliseconds

unsigned char buffer[160*2048];

#define BUFFER_SIZE 160*2048

static libusb_device_handle* open_push2_device()
{
  int result;

  if ((result = libusb_init(NULL)) < 0)
  {
    printf("error: [%d] could not initilialize usblib\n", result);
    return NULL;
  }

  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);

  libusb_device_handle* device_handle = NULL;
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
        libusb_close(device_handle);
        device_handle = NULL;
      }
      else
      {
        printf("opened\n");
        break; // successfully opened
      }
    }
  }

  if (device_handle == NULL)
  {
    printf(ErrorMsg);
  }

  libusb_free_device_list(devices, 1);
  return device_handle;
}

static void close_push2_device(libusb_device_handle* device_handle)
{
  libusb_release_interface(device_handle, 0);
  libusb_close(device_handle);
}

bool flag = false;

void on_frame_header_transfer_finished(struct libusb_transfer* tr) {
    printf("header transfer finished\n");
}

void on_buffer_transfer_finished(struct libusb_transfer* tr) {
    flag = true;
}

int main() {
    libusb_device_handle* device_handle = open_push2_device();

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
          buffer_to_be_transferred,
          2048,
          &actual_length,
          TRANSFER_TIMEOUT);
        }
    }
    

    return 0;
}