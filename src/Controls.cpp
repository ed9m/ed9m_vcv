#include "Controls.hpp"

void LED::lightOn(int color){
	midi::Message msg;
	msg.setNote(note);
	msg.setValue(color);
	msg.setChannel(1);
	msg.setStatus(on_status);
	out->sendMessage(msg);
}

void LED::lightOff(){
	midi::Message msg;
	msg.setNote(note);
	msg.setValue(0);
	msg.setChannel(1);
	msg.setStatus(off_status);
	out->sendMessage(msg);
}	

PushKey::PushKey(midi::Output * out_, int note_) {
	note = note_;
	group = 0;
	state = false;
	out = out_;
	on_status = 0x9;
	off_status = 0x8;
}

PushKnob::PushKnob (midi::Output * out_, int cc_) {
	note = cc_;
	out = out_;
	on_status = 0xB;
	off_status = 0xB;
}