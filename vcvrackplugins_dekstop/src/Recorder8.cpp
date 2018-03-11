#include <atomic>
#include <functional>
#include <thread>
#include <climits> // for SHRT_MAX in SoXR header

#include "dekstop.hpp"
#include "dsp/samplerate.hpp"
#include "../dep/osdialog/osdialog.h"
#include "write_wav.h"
#include "dsp/digital.hpp"
#include "dsp/ringbuffer.hpp"
#include "dsp/frame.hpp"

#define BLOCKSIZE 1024
#define BUFFERSIZE 32*BLOCKSIZE

struct Recorder8 : Module {
	enum ParamIds {
		RECORD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		AUDIO1_INPUT,
		NUM_INPUTS = AUDIO1_INPUT + 8
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		RECORDING_LIGHT,
		NUM_LIGHTS
	};

	std::string filename;
	WAV_Writer writer;
	std::atomic_bool isRecording;

	std::mutex mutex;
	std::thread thread;
	RingBuffer<Frame<8>, BUFFERSIZE> buffer;
	short writeBuffer[8*BUFFERSIZE];

	Recorder8() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{
		isRecording = false;
	}
	~Recorder8();
	void step() override;
	void clear();
	void startRecording();
	void stopRecording();
	void saveAsDialog();
	void openWAV();
	void closeWAV();
	void recorderRun();
};

Recorder8::~Recorder8() {
	if (isRecording) stopRecording();
}

void Recorder8::clear() {
	filename = "";
}

void Recorder8::startRecording() {
	saveAsDialog();
	if (!filename.empty()) {
		openWAV();
		isRecording = true;
		thread = std::thread(&Recorder8::recorderRun, this);
	}
}

void Recorder8::stopRecording() {
	isRecording = false;
	thread.join();
	closeWAV();
}

void Recorder8::saveAsDialog() {
	std::string dir = filename.empty() ? "." : extractDirectory(filename);
	char *path = osdialog_file(OSDIALOG_SAVE, dir.c_str(), "Output.wav", NULL);
	if (path) {
		filename = path;
		free(path);
	} else {
		filename = "";
	}
}

void Recorder8::openWAV() {
	float gSampleRate = engineGetSampleRate();
	if (!filename.empty()) {
		fprintf(stdout, "Recording to %s\n", filename.c_str());
		int result = Audio_WAV_OpenWriter(&writer, filename.c_str(), gSampleRate, 8);
		if (result < 0) {
			isRecording = false;
			char msg[100];
			snprintf(msg, sizeof(msg), "Failed to open WAV file, result = %d\n", result);
			osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
			fprintf(stderr, "%s", msg);
		}
	}
}

void Recorder8::closeWAV() {
	fprintf(stdout, "Stopping the recording.\n");
	int result = Audio_WAV_CloseWriter(&writer);
	if (result < 0) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Failed to close WAV file, result = %d\n", result);
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
		fprintf(stderr, "%s", msg);
	}
	isRecording = false;
}

// Run in a separate thread

// Excerpts from
// SoX Resampler Library      Copyright (c) 2007-18 robs@users.sourceforge.net

#define rint16D(a,b) __asm__ __volatile__("fistps %0": "=m"(a): "t"(b): "st")

static __inline int16_t rint16(double input) {
  int16_t result; rint16D(result, input); return result;}

void src_float_to_short_array(float const * src, short * dest, int len)
{
  double d, N = 1. + SHRT_MAX;
  assert (src && dest);

  while (len--) d = src[len] * N, dest[len] =
    (short)(d > N - 1? (short)(N - 1) : d < -N? (short)-N : rint16(d));
}

void Recorder8::recorderRun() {
	float gSampleRate = engineGetSampleRate();
	while (isRecording) {
		// Wake up a few times a second, often enough to never overflow the buffer.
		float sleepTime = (1.0 * BUFFERSIZE / gSampleRate) / 2.0;
		std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
		if (buffer.full()) {
			fprintf(stderr, "Recording buffer overflow. Can't write quickly enough to disk. Current buffer size: %d\n", BUFFERSIZE);
		}
		// Check if there is data
		int numFrames = buffer.size();
		if (numFrames > 0) {
			// Convert float frames to shorts
			{
				std::lock_guard<std::mutex> lock(mutex); // Lock during conversion
				src_float_to_short_array(static_cast<float*>(buffer.data[0].samples), writeBuffer, 8*numFrames);
				buffer.start = 0;
				buffer.end = 0;
			}

			fprintf(stdout, "Writing %d frames to disk\n", numFrames);
			int result = Audio_WAV_WriteShorts(&writer, writeBuffer, 8*numFrames);
			if (result < 0) {
				stopRecording();

				char msg[100];
				snprintf(msg, sizeof(msg), "Failed to write WAV file, result = %d\n", result);
				osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
				fprintf(stderr, "%s", msg);
			}
		}
	}
}

void Recorder8::step() {
	lights[RECORDING_LIGHT].value = isRecording ? 1.0 : 0.0;
	if (isRecording) {
		// Read input samples into recording buffer
		std::lock_guard<std::mutex> lock(mutex);
		if (!buffer.full()) {
			Frame<8> f;
			for (unsigned int i = 0; i < 8; i++) {
				f.samples[i] = inputs[AUDIO1_INPUT + i].value / 5.0;
			}
			buffer.push(f);
		}
	}
}

struct RecordButton : LEDButton {
	using Callback = std::function<void()>;

	Callback onPressCallback;
	SchmittTrigger recordTrigger;

	void onChange(EventChange &e) override {
		if (recordTrigger.process(value)) {
			onPress(e);
		}
	}
	void onPress(EventChange &e) {
		assert (onPressCallback);
		onPressCallback();
	}
};

struct Recorder8Widget : ModuleWidget {
	Recorder8Widget(Recorder8 *module);
	json_t *toJsonData();
	void fromJsonData(json_t *root);
};

Recorder8Widget::Recorder8Widget(Recorder8 *module) : ModuleWidget(module) {
	//	setPanel(SVG::load(assetPlugin(plugin, "res/Recorder8.svg")));

	unsigned int ChannelCount = 8;
	float margin = 5;
	float labelHeight = 15;
	float yPos = margin;
	float xPos = margin;
	box.size = Vec(15*6+5, 380);

	Panel *panel = new LightPanel();
	panel->box.size = box.size;
	addChild(panel);

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

	Label *recorderLabel = new Label();
	recorderLabel->box.pos = Vec(xPos, yPos + 5); // shim to put label below screws
	recorderLabel->text = "Recorder " + std::to_string(ChannelCount);
	addChild(recorderLabel);
	yPos += labelHeight + margin;

	xPos = 35;
	yPos += 2*margin;
	ParamWidget *recordButton = ParamWidget::create<RecordButton>(Vec(xPos, yPos-1), module, Recorder8::RECORD_PARAM, 0.0, 1.0, 0.0);
	RecordButton *btn = dynamic_cast<RecordButton*>(recordButton);
	Recorder8 *recorder = dynamic_cast<Recorder8 *>(module);

	btn->onPressCallback = [=]()
	{
		if (!recorder->isRecording) {
			recorder->startRecording();
		} else {
			recorder->stopRecording();
		}
	};
	addParam(recordButton);
	addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(xPos+6, yPos+5), module, Recorder8::RECORDING_LIGHT));
	xPos = margin;
	yPos += recordButton->box.size.y + 3*margin;

	Label *channelLabel = new Label();
	channelLabel->box.pos = Vec(margin, yPos);
	channelLabel->text = "Channels";
	addChild(channelLabel);
	yPos += labelHeight + margin;

	yPos += 5;
	xPos = 10;
	for (unsigned int i = 0; i < ChannelCount; i++) {
		addInput(Port::create<PJ3410Port>(Vec(xPos, yPos), Port::INPUT, module, i));
		Label *portLabel = new Label();
		portLabel->box.pos = Vec(xPos + 4, yPos + 28);
		portLabel->text = stringf("%d", i + 1);
		addChild(portLabel);

		if (i % 2 ==0) {
			xPos += 37 + margin;
		} else {
			xPos = 10;
			yPos += 40 + margin;
		}
	}
}
Model *modelRecorder8 = Model::create<Recorder8, Recorder8Widget>("dekstop", "Recorder8", "Recorder 8", UTILITY_TAG);

