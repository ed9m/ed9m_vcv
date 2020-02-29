#include "plugin.hpp"
#include "Controls.hpp"
#include "Display.hpp"
#include "AbletonPush2.hpp"

struct AbletonPush2 : Module {

	const float updateFrequency = 400;
	int sampleCounter = 0;

	PushKey * keyboard[128];
	PushKnob * knobs[128];

	PushKeyGroup * groups[10];

	Push2Display * display;

	// state variables

	bool shiftMode = false;

	int focusNote = BASE_NOTE;
	bool focusPressed = false;
	int focusGroup = 0;

	bool isplaying;

public:

	midi::Output midiOutput;
	midi::InputQueue midiInput;

	bool inputConnected;
	bool outputConnected;

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		GATEOUT_OUTPUT,
		CVOUT_OUTPUT,
		VELOUT_OUTPUT,

		G1G_OUTPUT,
		CV1G_OUTPUT,
		V1G_OUTPUT,
		G2G_OUTPUT,
		CV2G_OUTPUT,
		V2G_OUTPUT,
		G3G_OUTPUT,
		CV3G_OUTPUT,
		V3G_OUTPUT,
		G4G_OUTPUT,
		CV4G_OUTPUT,
		V4G_OUTPUT,

		GR_OUTPUT,
		GRT_OUTPUT,

		NUM_OUTPUTS
	};
	enum LightIds {	
		NUM_LIGHTS
	};

	/** Number of maps */
	int mapLen[NUM_GROUPS];
	/** The mapped CC number of each channel */
	int ccs[NUM_GROUPS][MAX_CHANNELS];
	/** The mapped param handle of each channel */
	ParamHandle * paramHandles[NUM_GROUPS][MAX_CHANNELS];

	/** Channel ID of the learning session */
	int learningId;
	/** Whether the CC has been set during the learning session */
	bool learnedCc;
	/** Whether the param has been set during the learning session */
	bool learnedParam;

	/** The value of each CC number */
	float values[NUM_GROUPS][128];
	/** The smoothing processor (normalized between 0 and 1) of each channel */
	dsp::ExponentialFilter valueFilters[NUM_GROUPS][MAX_CHANNELS];
	bool filterInitialized[NUM_GROUPS][MAX_CHANNELS] = {};
	dsp::ClockDivider divider;

	void sendMidi(int cmd, int note, int val) {
		midi::Message msg;
		msg.setNote(note);
		msg.setValue(val);
		msg.setChannel(1);
		msg.setStatus(cmd);
		midiOutput.sendMessage(msg);
	}

	void connectPush() {
		auto in_devs = midiInput.getDeviceIds();
		for (int i = 0; i < in_devs.size(); i++){
			if (midiInput.getDeviceName(in_devs[i]).find("Ableton") != std::string::npos) {
				midiInput.setDeviceId(in_devs[i]);
				inputConnected = true;
				break;
			}
		}
		
		auto out_devs = midiOutput.getDeviceIds();
		for (int i = 0; i < out_devs.size(); i++){
			if (midiOutput.getDeviceName(out_devs[i]).find("Ableton") != std::string::npos) {
				midiOutput.setDeviceId(out_devs[i]);
				outputConnected = true;
				break;
			}
		}
	}

	void attachDisplay(Push2Display * display_) {
		display = display_;
	}

	AbletonPush2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		inputConnected = false;
		outputConnected = false;
		display = nullptr;
		isplaying = false;
		for(int i = 0; i < 128; i ++) {
			keyboard[i] = new PushKey(&midiOutput, i);
			knobs[i] = new PushKnob(&midiOutput, i);
		}

		for(int i = 0; i < NUM_GROUPS; i ++) {
			groups[i] = new PushKeyGroup(group_colors[i]);
		}

		for (int id = 0; id < MAX_CHANNELS; id++) {
			for (int g = 0; g < NUM_GROUPS; g++) {
				paramHandles[g][id] = new ParamHandle();
				paramHandles[g][id]->color = nvgRGB(0xff, 0xff, 0x40);
				APP->engine->addParamHandle(paramHandles[g][id]);
			}
		}
		// for (int i = 0; i < MAX_CHANNELS; i++) {
		// 	valueFilters[i].setTau(1 / 30.f);
		// }

		onReset();
		// connectPush();
	}

	~AbletonPush2() {
		for(int i = 0; i < 128; i ++) {
			keyboard[i]->lightOff();
			knobs[i]->lightOff();
		}

		// for (int g = 0; g < NUM_GROUPS; g++) {
		// 	for (int id = 0; id < MAX_CHANNELS; id++) {
		// 		APP->engine->removeParamHandle(paramHandles[focusGroup][id]);
		// 	}
		// }
	}

	void onReset() override {
		learningId = -1;
		learnedCc = false;
		learnedParam = false;
		clearMaps();
		for (int g = 0; g < NUM_GROUPS; g++) mapLen[g] = 1;
		for (int i = 0; i < 128; i++) {
			for (int g = 0; g < NUM_GROUPS; g++) {
				values[g][i] = -1;
			}
		}
		midiInput.reset();
	}

	void lightUp() {
		for (int i = BASE_NOTE; i < BASE_NOTE + NOTES; i++) {
			if (i == focusNote && focusPressed) {
				keyboard[focusNote]->lightOn(126);
				// focusPressed = false;
				continue;
			}
			auto color = groups[keyboard[i]->group]->color;
			// color = (i - BASE_NOTE);
			keyboard[i]->lightOn(color);
		}

		if (shiftMode) knobs[SHIFT]->lightOn(127);
		else knobs[SHIFT]->lightOff();

		// PushKnob KTAPTEMPO = PushKnob(0xB, TAP_TEMPO);
		// KTAPTEMPO.lightOn(&midiOutput, 127);

		// sendMidi(0xB, TAP_TEMPO, 127);
		// sendMidi(0xB, METRONOME, 127);
		// sendMidi(0xB, DELETE, 127);
		// sendMidi(0xB, UNDO, 127);
		// sendMidi(0xB, MUTE, 127);
		// sendMidi(0xB, SOLO, 127);
		// sendMidi(0xB, STOP_CLIP, 127);
		// sendMidi(0xB, MASTER, 127);
		if(isplaying) sendMidi(0xB, PLAY, 126);
		else sendMidi(0xB, PLAY, 125);
	}

	void processNote(midi::Message msg) {

		if (!shiftMode && (keyboard[msg.getNote()]->group == 0)) return;

		switch (msg.getStatus()) {
			case 0x9: 
				outputs[CVOUT_OUTPUT].setVoltage(((float)msg.getNote() - BASE_NOTE) / 12.f);
				outputs[GATEOUT_OUTPUT].setVoltage(10.f);
				if (msg.getValue() > 0) focusPressed = true;
				else focusPressed = false;
			    break;
			case 0x8:
				outputs[GATEOUT_OUTPUT].setVoltage(0.f);
				focusPressed = false;
				break;
			default:
				break;
		}
		focusNote = msg.getNote();
		focusGroup = keyboard[focusNote]->group;

		for (int i = 0; i < mapLen[focusGroup]; i++) {
			// Get Module
			Module* module = paramHandles[focusGroup][i]->module;
			if (module == NULL) continue;
			int paramId = paramHandles[focusGroup][i]->paramId;
			ParamQuantity* paramQuantity = module->paramQuantities[paramId];
			if (paramQuantity->isBounded())
				values[focusGroup][ccs[focusGroup][i]] = paramQuantity->getScaledValue() * 127.f;
		}

		display->setLabelsAndValues(&mapLen[focusGroup], paramHandles[focusGroup], values[focusGroup], ccs[focusGroup]);
	}

	void processKnob(midi::Message msg) {
		
		auto knobNum = msg.getNote();
		auto value = msg.getValue();

		if (knobNum == SHIFT && value) {
			if (shiftMode) shiftMode = false;
			else shiftMode = true;
			return;
		}

		if (knobNum == PLAY) {
			if (value) {
				outputs[GR_OUTPUT].setVoltage(10.f);
				outputs[GRT_OUTPUT].setVoltage(10.f);
				if (!isplaying) isplaying = true;
				else isplaying = false;
			} else {
				outputs[GR_OUTPUT].setVoltage(0.f);
				outputs[GRT_OUTPUT].setVoltage(0.f);
			}
		}

		if (shiftMode && (knobNum == TAPTEMPO_ENCODER)) {
			keyboard[focusNote]->group += (value & 0x40) ? -1 : 1;
			if (keyboard[focusNote]->group > 9) keyboard[focusNote]->group = 9;
			if (keyboard[focusNote]->group < 0) keyboard[focusNote]->group = 0;
		}

		// Learn
		if (0 <= learningId && values[focusGroup][knobNum] != value) {
			ccs[focusGroup][learningId] = knobNum;
			valueFilters[focusGroup][learningId].reset();
			learnedCc = true;
			commitLearn();
			updateMapLen(focusGroup);
			refreshParamHandleText(learningId);
		}
		values[focusGroup][knobNum] += (float)((int)(value & 0x3F) - 64 * ((value & 0x40) >> 6)) / 1.5;
		if(values[focusGroup][knobNum] > 127) values[focusGroup][knobNum] = 127;
		if(values[focusGroup][knobNum] < 0) values[focusGroup][knobNum] = 0;
	}

	void processMidi(midi::Message msg) {
		auto status = msg.getStatus();
		auto note = msg.getNote();

		if ((status == 0x9 || status == 0x8) && (note >= BASE_NOTE) && (note < BASE_NOTE + NOTES))
			processNote(msg);

		if (status == 0xB) {
			processKnob(msg);
		}
	}

	void process(const ProcessArgs &args) override {

		if (display && !display->display_connected) display->open();

		if (sampleCounter > args.sampleRate / updateFrequency) {

			if ((!inputConnected) && (!outputConnected)) {
				connectPush();
			}

			midi::Message msg;
			while (midiInput.shift(&msg)) {
				processMidi(msg);
			}

			lightUp();

			// Step channels
			for (int id = 0; id < mapLen[focusGroup]; id++) {
				int cc = ccs[focusGroup][id];
				if (cc < 0)
					continue;
				// Get Module
				Module* module = paramHandles[focusGroup][id]->module;
				if (!module)
					continue;
				// Get ParamQuantity from ParamHandle
				int paramId = paramHandles[focusGroup][id]->paramId;
				ParamQuantity* paramQuantity = module->paramQuantities[paramId];
				if (!paramQuantity)
					continue;
				if (!paramQuantity->isBounded())
					continue;
				// Set filter from param value if filter is uninitialized
				if (!filterInitialized[focusGroup][id]) {
					valueFilters[focusGroup][id].out = paramQuantity->getScaledValue();
					filterInitialized[focusGroup][id] = true;
					continue;
				}
				// Check if CC has been set by the MIDI device
				if (values[focusGroup][cc] < 0)
					continue;
				float value = values[focusGroup][cc] / 127.f;
				// Detect behavior from MIDI buttons.
				if (std::fabs(valueFilters[focusGroup][id].out - value) >= 1.f) {
					// Jump value
					valueFilters[focusGroup][id].out = value;
				}
				else {
					// Smooth value with filter
					valueFilters[focusGroup][id].process(args.sampleTime * divider.getDivision(), value);
				}
				paramQuantity->setScaledValue(valueFilters[focusGroup][id].out);
			}

			sampleCounter = 0;
		}
		sampleCounter ++;
	}

	void clearMap(int id) {
		learningId = -1;
		ccs[focusGroup][id] = -1;
		APP->engine->updateParamHandle(paramHandles[focusGroup][id], -1, 0, true);
		valueFilters[focusGroup][id].reset();
		updateMapLen(focusGroup);
		refreshParamHandleText(id);
	}

	void clearMaps() {
		learningId = -1;
		for (int g = 0; g < NUM_GROUPS; g++) {
			for (int id = 0; id < MAX_CHANNELS; id++) {
				ccs[g][id] = -1;
				APP->engine->updateParamHandle(paramHandles[g][id], -1, 0, true);
				valueFilters[g][id].reset();
				refreshParamHandleText(id);
			}
			mapLen[g] = 0;
		}
	}

	void updateMapLen(int group) {
		// Find last nonempty map
		int id;
		for (id = MAX_CHANNELS - 1; id >= 0; id--) {
			if (ccs[group][id] >= 0 || paramHandles[group][id]->moduleId >= 0)
				break;
		}
		mapLen[group] = id + 1;
		// Add an empty "Mapping..." slot
		if (mapLen[group] < MAX_CHANNELS)
			mapLen[group]++;
	}

	void commitLearn() {
		if (learningId < 0)
			return;
		if (!learnedCc)
			return;
		if (!learnedParam)
			return;
		// Reset learned state
		learnedCc = false;
		learnedParam = false;
		// Find next incomplete map


		// Get Module
		Module* module = paramHandles[focusGroup][learningId]->module;
		int paramId = paramHandles[focusGroup][learningId]->paramId;
		ParamQuantity* paramQuantity = module->paramQuantities[paramId];
		if (paramQuantity->isBounded())
			values[focusGroup][ccs[focusGroup][learningId]] = paramQuantity->getScaledValue() * 127.f;

		while (++learningId < MAX_CHANNELS) {
			if (ccs[focusGroup][learningId] < 0 || paramHandles[focusGroup][learningId]->moduleId < 0)
				return;
		}
		learningId = -1;

	}

	void enableLearn(int id) {
		if (learningId != id) {
			learningId = id;
			learnedCc = false;
			learnedParam = false;
		}
	}

	void disableLearn(int id) {
		if (learningId == id) {
			learningId = -1;
		}
	}

	void learnParam(int id, int moduleId, int paramId) {
		ParamHandle* oldParamHandle = APP->engine->getParamHandle(moduleId, paramId);
		if (oldParamHandle) {
			if (paramHandles[focusGroup][id]->moduleId == -1) {
				delete paramHandles[focusGroup][id];
			}
			paramHandles[focusGroup][id] = oldParamHandle;
		} else {
			APP->engine->updateParamHandle(paramHandles[focusGroup][id], moduleId, paramId, true);
		}
		learnedParam = true;
		commitLearn();
		updateMapLen(focusGroup);
	}

	void refreshParamHandleText(int id) {
		std::string text;
		if (ccs[focusGroup][id] >= 0)
			text = string::f("CC%02d", ccs[focusGroup][id]);
		else
			text = "MIDI-Map";
		paramHandles[focusGroup][id]->text = text;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		// Remember values so users don't have to touch MIDI controller knobs when restarting Rack
		json_t* valuesJ = json_array();
		for (int i = 0; i < 128; i++) {
			json_array_append_new(valuesJ, json_integer(keyboard[i]->group));
		}
		json_object_set_new(rootJ, "keygroups", valuesJ);

		for (int g = 0; g < NUM_GROUPS; g++) {
			json_t* mapsJ = json_array();
			for (int id = 0; id < mapLen[g]; id++) {
				json_t* mapJ = json_object();
				json_object_set_new(mapJ, "cc", json_integer(ccs[g][id]));
				json_object_set_new(mapJ, "moduleId", json_integer(paramHandles[g][id]->moduleId));
				json_object_set_new(mapJ, "paramId", json_integer(paramHandles[g][id]->paramId));
				// json_object_set_new(mapJ, "value", json_integer(values[g][id]));
				json_array_append_new(mapsJ, mapJ);
			}
			char name[20];
			sprintf(name, "maps%d", g);
			json_object_set_new(rootJ, name, mapsJ);
		}

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {

		json_t* valuesJ = json_object_get(rootJ, "keygroups");
		if (valuesJ) {
			for (int i = 0; i < 128; i++) {
				json_t* valueJ = json_array_get(valuesJ, i);
				if (valueJ) {
					keyboard[i]->group = json_integer_value(valueJ);
				}
			}
		}

		clearMaps();

		for (int g = 0; g < NUM_GROUPS; g++) {
			char name[20];
			sprintf(name, "maps%d", g);
			json_t* mapsJ = json_object_get(rootJ, name);
			if (mapsJ) {
				json_t* mapJ;
				size_t mapIndex;
				json_array_foreach(mapsJ, mapIndex, mapJ) {
					json_t* ccJ = json_object_get(mapJ, "cc");
					json_t* moduleIdJ = json_object_get(mapJ, "moduleId");
					json_t* paramIdJ = json_object_get(mapJ, "paramId");
					// json_t* valueJ = json_object_get(mapJ, "value");
					if (!(ccJ && moduleIdJ && paramIdJ))
						continue;
					if (mapIndex >= MAX_CHANNELS)
						continue;
					ccs[g][mapIndex] = json_integer_value(ccJ);
					// values[g][mapIndex] = json_integer_value(valueJ);

					ParamHandle* oldParamHandle = APP->engine->getParamHandle(json_integer_value(moduleIdJ), json_integer_value(paramIdJ));
					if (oldParamHandle) {
						paramHandles[g][mapIndex] = oldParamHandle;
					} else {
						APP->engine->updateParamHandle(paramHandles[g][mapIndex], json_integer_value(moduleIdJ), json_integer_value(paramIdJ), false);
					}

					refreshParamHandleText(mapIndex);
				}
			}
			updateMapLen(g);
		}


		for (int g = 0; g < NUM_GROUPS; g++) {
			for (int id = 0; id < mapLen[g]; id++) {
				int cc = ccs[g][id];
				if (cc < 0)
					continue;
				// Get Module
				Module* module = paramHandles[g][id]->module;
				if (!module)
					continue;
				// Get ParamQuantity from ParamHandle
				int paramId = paramHandles[g][id]->paramId;
				ParamQuantity* paramQuantity = module->paramQuantities[paramId];
				if (!paramQuantity)
					continue;
				if (!paramQuantity->isBounded())
					continue;
				// Check if CC has been set by the MIDI device
				// if (values[g][cc] < 0)
				// 	continue;
				values[g][cc] = paramQuantity->getScaledValue() * 127.f;
			}
		}
	}

};

struct MIDI_MapChoice : LedDisplayChoice {
	AbletonPush2* module;
	int id;
	int disableLearnFrames = -1;

	void setModule(AbletonPush2* module) {
		this->module = module;
	}

	void onButton(const event::Button& e) override {
		e.stopPropagating();
		if (!module)
			return;

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
		}

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			module->clearMap(id);
			e.consume(this);
		}
	}

	void onSelect(const event::Select& e) override {
		if (!module)
			return;

		ScrollWidget* scroll = getAncestorOfType<ScrollWidget>();
		scroll->scrollTo(box);

		// Reset touchedParam
		APP->scene->rack->touchedParam = NULL;
		module->enableLearn(id);
	}

	void onDeselect(const event::Deselect& e) override {
		if (!module)
			return;
		// Check if a ParamWidget was touched
		ParamWidget* touchedParam = APP->scene->rack->touchedParam;
		if (touchedParam) {
			APP->scene->rack->touchedParam = NULL;
			int moduleId = touchedParam->paramQuantity->module->id;
			int paramId = touchedParam->paramQuantity->paramId;
			module->learnParam(id, moduleId, paramId);
		}
		else {
			module->disableLearn(id);
		}
	}

	void step() override {
		if (!module)
			return;

		// Set bgColor and selected state
		if (module->learningId == id) {
			bgColor = color;
			bgColor.a = 0.15;

			// HACK
			if (APP->event->selectedWidget != this)
				APP->event->setSelected(this);
		}
		else {
			bgColor = nvgRGBA(0, 0, 0, 0);

			// HACK
			if (APP->event->selectedWidget == this)
				APP->event->setSelected(NULL);
		}

		// Set text
		text = "";
		if (module->ccs[module->focusGroup][id] >= 0) {
			text += string::f("CC%02d ", module->ccs[module->focusGroup][id]);
		}
		if (module->paramHandles[module->focusGroup][id]->moduleId >= 0) {
			text += getParamName();
		}
		if (module->ccs[module->focusGroup][id] < 0 && module->paramHandles[module->focusGroup][id]->moduleId < 0) {
			if (module->learningId == id) {
				text = "Mapping...";
			}
			else {
				text = "Unmapped";
			}
		}

		// Set text color
		if ((module->ccs[module->focusGroup][id] >= 0 && module->paramHandles[module->focusGroup][id]->moduleId >= 0) || module->learningId == id) {
			color.a = 1.0;
		}
		else {
			color.a = 0.5;
		}
	}

	std::string getParamName() {
		if (!module)
			return "";
		if (id >= module->mapLen[module->focusGroup])
			return "";
		ParamHandle* paramHandle = module->paramHandles[module->focusGroup][id];
		if (paramHandle->moduleId < 0)
			return "";
		ModuleWidget* mw = APP->scene->rack->getModule(paramHandle->moduleId);
		if (!mw)
			return "";
		// Get the Module from the ModuleWidget instead of the ParamHandle.
		// I think this is more elegant since this method is called in the app world instead of the engine world.
		Module* m = mw->module;
		if (!m)
			return "";
		int paramId = paramHandle->paramId;
		if (paramId >= (int) m->params.size())
			return "";
		ParamQuantity* paramQuantity = m->paramQuantities[paramId];
		std::string s;
		s += mw->model->name;
		s += " ";
		s += paramQuantity->label;
		return s;
	}
};

struct MIDI_MapDisplay : MidiWidget {
	AbletonPush2* module;
	ScrollWidget* scrolls[NUM_GROUPS];
	MIDI_MapChoice* choices[NUM_GROUPS][MAX_CHANNELS];
	LedDisplaySeparator* separators[NUM_GROUPS][MAX_CHANNELS];

	void setModule(AbletonPush2* module) {
		this->module = module;

		for (int g = 0; g < NUM_GROUPS; g++) {

			scrolls[g] = new ScrollWidget;
			scrolls[g]->box.pos = channelChoice->box.getBottomLeft();
			scrolls[g]->box.size.x = box.size.x;
			scrolls[g]->box.size.y = box.size.y - scrolls[g]->box.pos.y;
			addChild(scrolls[g]);

			LedDisplaySeparator* separator = createWidget<LedDisplaySeparator>(scrolls[g]->box.pos);
			separator->box.size.x = box.size.x;
			addChild(separator);
			separators[g][0] = separator;

			Vec pos;
			for (int id = 0; id < MAX_CHANNELS; id++) {
				if (id > 0) {
					LedDisplaySeparator* separator = createWidget<LedDisplaySeparator>(pos);
					separator->box.size.x = box.size.x;
					scrolls[g]->container->addChild(separator);
					separators[g][id] = separator;
				}

				MIDI_MapChoice* choice = createWidget<MIDI_MapChoice>(pos);
				choice->box.size.x = box.size.x;
				choice->id = id;
				choice->setModule(module);
				scrolls[g]->container->addChild(choice);
				choices[g][id] = choice;

				pos = choice->box.getBottomLeft();
			}
		}
	}

	void step() override {
		if (module) {
			for (int g = 0; g < NUM_GROUPS; g++) {
				int mapLen = module->mapLen[g];
				for (int id = 0; id < MAX_CHANNELS; id++) {
					choices[g][id]->visible = ((id < mapLen) && (g == module->focusGroup));
					separators[g][id]->visible = ((id < mapLen) && (g == module->focusGroup));
				}
				scrolls[g]->visible = (g == module->focusGroup);
			}
		}

		MidiWidget::step();
	}
};

struct AbletonPush2Widget : ModuleWidget {

	FramebufferWidget *push2;
	FramebufferWidget *connectButton;

	AbletonPush2Widget(AbletonPush2 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AbletonPush2.svg")));

		// initNanoVG()

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5., 70.)), module, AbletonPush2::GATEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5., 85.)), module, AbletonPush2::CVOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5., 100.)), module, AbletonPush2::VELOUT_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15., 70)), module, AbletonPush2::G1G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15., 85)), module, AbletonPush2::CV1G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15., 100)), module, AbletonPush2::V1G_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25., 70)), module, AbletonPush2::G2G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25., 85)), module, AbletonPush2::CV2G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25., 100)), module, AbletonPush2::V2G_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35., 70)), module, AbletonPush2::G3G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35., 85)), module, AbletonPush2::CV3G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35., 100)), module, AbletonPush2::V3G_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45., 70)), module, AbletonPush2::G4G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45., 85)), module, AbletonPush2::CV4G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45., 100)), module, AbletonPush2::V4G_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5., 115)), module, AbletonPush2::GR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15., 115)), module, AbletonPush2::GRT_OUTPUT));

		Push2Display *push = new Push2Display();
		push->box.pos = Vec(-959, -159);
		push->box.size = Vec(960, 160);
		addChild(push);
		this->push2 = push;

		// SvgButton *connectButton = new SvgButton();
		// connectButton->box.pos = Vec(50, 15);
		// connectButton->box.size = Vec(10, 10);
		// addChild(connectButton);
		// this->connectButton = connectButton;

		if (module) module->attachDisplay(push);

		MIDI_MapDisplay* midiWidget = createWidget<MIDI_MapDisplay>(mm2px(Vec(3.41891, 10.8373)));
		midiWidget->box.size = mm2px(Vec(43.999, 50.664));
		midiWidget->setMidiPort(module ? &module->midiInput : NULL);
		midiWidget->setModule(module);
		addChild(midiWidget);
	}

};

Model *modelAbletonPush2 = createModel<AbletonPush2, AbletonPush2Widget>("AbletonPush2");