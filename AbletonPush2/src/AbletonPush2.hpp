#pragma once

#ifdef __linux__
#include <libusb-1.0/libusb.h>
#else
#include "libusb.h"
#endif

#define ABLETON_VENDOR_ID 0x2982
#define PUSH2_PRODUCT_ID  0x1967

#define PUSH2_BULK_EP_OUT 0x01
#define TRANSFER_TIMEOUT  1000 // milliseconds

#define TAP_TEMPO 3
#define METRONOME 9
#define DELETE 118
#define UNDO 119
#define DEVICE 110
#define BROWSE 111
#define MIX 112
#define CLIP 113
#define ADD_DEVICE 52
#define ADD_TRACK 53
#define MASTER 28
#define MUTE 60
#define SOLO 61
#define STOP_CLIP 29
#define SETUP 30
#define USER 59
#define LEFT 44
#define RIGHT 45
#define UP 46
#define DOWN 47
#define REPEAT 56
#define ACCENT 57
#define SCALE 58
#define LAYOUT 31
#define NOTE 50
#define SESSION 51
#define OCTAVE_UP 55
#define OCTAVE_DOWN 54
#define PAGE_LEFT 62
#define PAGE_RIGHT 63
#define SHIFT 49
#define SELECT 48
#define CONVERT 35
#define DOUBLE_LOOP 117
#define QUANTIZE 116
#define DUPLICATE 88
#define NEW 87
#define FIXED_LENGTH 90
#define AUTOMATE 89
#define REC 86
#define PLAY 85
#define TOP_BUTTON_ROW_START 102
#define BOT_BUTTON_ROW_START 20
#define BASE_NOTE 36
#define NOTES 64
#define TAPTEMPO_ENCODER 14
#define METRONOME_ENCODER 15

static const int NUM_GROUPS = 10;
static const int MAX_CHANNELS = 8;

int group_colors[NUM_GROUPS] = {
	0, 2, 3, 8, 11, 16, 19, 26, 29, 32
};