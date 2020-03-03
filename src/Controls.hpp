#pragma once

#include "plugin.hpp"

class LED {

public:

	int note;
	int on_status;
	int off_status;
	midi::Output * out;

	void lightOn(int color);
	void lightOff();
};

class PushKey : public LED {
 	
public:

 	int group;
	bool state;

 	PushKey (midi::Output * out_, int note_);

}; 

class PushKeyGroup {
	
	int mode;

public:

	int color;

	PushKeyGroup(int color_) {
		color = color_;
	}

};

class PushKnob : public LED {

public:

 	PushKnob (midi::Output * out_, int cc_);

}; 