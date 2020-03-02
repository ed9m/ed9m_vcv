#include "AbletonPush2.hpp"

struct Push2Display : FramebufferWidget {

	unsigned char frame_header[16] = {
          0xFF, 0xCC, 0xAA, 0x88,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00 };

	libusb_device_handle* device_handle;

public:

	bool display_connected;

  	unsigned char* image;

  	ParamHandle ** paramHandles;

  	int * len;
  	char ** names;
  	float * values;
  	int * ccs;

  	int open() {
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
	          printf("Opened\n");
	          display_connected = true;
	          break; // successfully opened
	        }
	      }
	  	}
    }

    void close() {
	    libusb_release_interface(device_handle, 0);
	    libusb_close(device_handle);
	}

	void draw(NVGcontext * vg) {
		if (len == nullptr) return;
		int l = *len - 1;
		for (int i = 0; i < l; i ++) {
			ParamHandle* paramHandle = paramHandles[i];
			if (paramHandle->moduleId < 0) continue;
			ModuleWidget* mw = APP->scene->rack->getModule(paramHandle->moduleId);
			if (!mw) continue;
			
			Module* m = mw->module;
			if (!m) continue;
			int paramId = paramHandle->paramId;
			if (paramId >= (int) m->params.size()) continue;
			ParamQuantity* paramQuantity = m->paramQuantities[paramId];
			std::string s;
			s += paramQuantity->label;

			nvgBeginPath(vg);
			nvgFontSize(vg, 17.f);		
			char param_name[20];
			nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			// sprintf(param_name, "param%d", i);
			nvgText(vg, 98*i + (2*i + 1)*11 + 50, 20, s.c_str(), NULL);
			nvgFillColor(vg, nvgRGBA(255,255,255,120));
			nvgFill(vg);
			nvgClosePath(vg);


			nvgBeginPath(vg);
	        nvgArc(vg, 98*i + (2*i + 1)*11 + 50, 100, 40, M_PI * (0.5f + 2 * values[ccs[i]] / 127.f), M_PI * 0.5f, NVG_CCW);
	        nvgStrokeWidth(vg, 5.f);
	        nvgStrokeColor(vg, nvgRGBA(255,255,255,120));
	        nvgStroke(vg);
	        nvgClosePath(vg);


	        nvgBeginPath(vg);
			nvgFontSize(vg, 17.f);		
			char val[20];
			nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			sprintf(val, "%d", (int)values[ccs[i]]);
			nvgText(vg, 98*i + (2*i + 1)*11 + 50, 100, val, NULL);
			nvgFillColor(vg, nvgRGBA(255,255,255,120));
			nvgFill(vg);
			nvgClosePath(vg);
		}
	}

	void setLabelsAndValues(int * len, ParamHandle ** paramHandles, float * values, int * ccs) {
		this->len = len;
		this->paramHandles = paramHandles;
		this->values = values;
		this->ccs = ccs;
	}

	Push2Display() {
		display_connected = false;
		device_handle = NULL;
		len = nullptr;
		image = (unsigned char*)malloc(960*160*4);
	}

	~Push2Display() {
		printf("Delete push\n");
		if (display_connected) close();
	}

	void sendDisplay(unsigned char * image) {
      int actual_length;
      
      if (device_handle == nullptr) {
        if (display_connected) printf("no device\n"), display_connected = false;
      }
      else { //print one frame
        if (!display_connected) printf("Push connected\n"), display_connected = true;
        libusb_bulk_transfer(device_handle, PUSH2_BULK_EP_OUT, frame_header, sizeof(frame_header), &actual_length, TRANSFER_TIMEOUT);
        for (int i = 0; i < 160; i++)
          libusb_bulk_transfer(device_handle, PUSH2_BULK_EP_OUT, &image[(159 - i)*1920], 2048, &actual_length, TRANSFER_TIMEOUT);
      }
  	}

	void drawFramebuffer() override {

		NVGcontext* vg = APP->window->vg;


	    if (display_connected) {

	    	nvgSave(vg);
			glViewport(0, 0, 960, 160);
		    glClearColor(0, 0, 0, 0);
		    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	    	nvgBeginFrame(vg, 960,  160, 1);

		    draw(vg);

		    nvgEndFrame(vg);

		    glReadPixels(0, 0, 960, 160, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image); 

		    for (int i = 0; i < 80*960; i ++){
		      image[i * 4] ^= 0xE7;
		      image[i * 4 + 1] ^= 0xF3;
		      image[i * 4 + 2] ^= 0xE7;
		      image[i * 4 + 3] ^= 0xFF;
		    }
	    	
	    	sendDisplay(image);

	    	nvgRestore(vg);
	    }

	    dirty = true;
	}

};