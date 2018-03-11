#include <atomic>
#include <functional>
#include <thread>

#include "dekstop.hpp"
#include "engine.hpp"
#include "../dep/osdialog/osdialog.h"
#include "write_wav.h"
#include "dsp/digital.hpp"
#include "dsp/ringbuffer.hpp"
#include "dsp/frame.hpp"
#include "dsp/samplerate.hpp"

#define BLOCKSIZE 1024
#define BUFFERSIZE 32*BLOCKSIZE


struct Recorder2 : Module {
        enum ParamIds {
                RECORD_PARAM,
                NUM_PARAMS
        };
        enum InputIds {
                AUDIO1_INPUT,
                NUM_INPUTS = AUDIO1_INPUT + 2
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
        RingBuffer<Frame<2>, BUFFERSIZE> buffer;
        short writeBuffer[2*BUFFERSIZE];

        Recorder2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
        {
                isRecording = false;
        }
        ~Recorder2();
        void step() override;
        void clear();
        void startRecording();
        void stopRecording();
        void saveAsDialog();
        void openWAV();
        void closeWAV();
        void recorderRun();
};

Recorder2::~Recorder2() {
        if (isRecording) stopRecording();
}

void Recorder2::clear() {
        filename = "";
}

void Recorder2::startRecording() {
        saveAsDialog();
        if (!filename.empty()) {
                openWAV();
                isRecording = true;
                thread = std::thread(&Recorder2::recorderRun, this);
        }
}

void Recorder2::stopRecording() {
        isRecording = false;
        thread.join();
        closeWAV();
}

void Recorder2::saveAsDialog() {
        std::string dir = filename.empty() ? "." : extractDirectory(filename);
        char *path = osdialog_file(OSDIALOG_SAVE, dir.c_str(), "Output.wav", NULL);
        if (path) {
                filename = path;
                free(path);
        } else {
                filename = "";
        }
}

void Recorder2::openWAV() {
        float gSampleRate = engineGetSampleRate();
        if (!filename.empty()) {
                fprintf(stdout, "Recording to %s\n", filename.c_str());
                int result = Audio_WAV_OpenWriter(&writer, filename.c_str(), gSampleRate, 2);
                if (result < 0) {
                        isRecording = false;
                        char msg[100];
                        snprintf(msg, sizeof(msg), "Failed to open WAV file, result = %d\n", result);
                        osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
                        fprintf(stderr, "%s", msg);
                }
        }
}

void Recorder2::closeWAV()
{
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
// See Recorder8.cpp for implementation, avoid multiple definition
void src_float_to_short_array(float const *, short *, int);

void Recorder2::recorderRun() {
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
                                src_float_to_short_array(static_cast<float*>(buffer.data[0].samples), writeBuffer, 2*numFrames);
                                buffer.start = 0;
                                buffer.end = 0;
                        }

                        fprintf(stdout, "Writing %d frames to disk\n", numFrames);
                        int result = Audio_WAV_WriteShorts(&writer, writeBuffer, 2*numFrames);
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

void Recorder2::step() {
        lights[RECORDING_LIGHT].value = isRecording ? 1.0 : 0.0;
        if (isRecording) {
                // Read input samples into recording buffer
                std::lock_guard<std::mutex> lock(mutex);
                if (!buffer.full()) {
                        Frame<2> f;
                        for (unsigned int i = 0; i < 2; i++) {
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

struct Recorder2Widget : ModuleWidget {
    Recorder2Widget(Recorder2 *module);
    json_t *toJsonData();
    void fromJsonData(json_t *root);
};

Recorder2Widget::Recorder2Widget(Recorder2 *module) : ModuleWidget(module) {
    float margin = 5;
    float yPos = margin;
    float xPos = margin;

    setPanel(SVG::load(assetPlugin(plugin, "res/Recorder2.svg")));
    box.size = Vec(15*6+5, 380);

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    xPos = 35;
    yPos += 2*margin;
    ParamWidget *recordButton = ParamWidget::create<RecordButton>(Vec(xPos, yPos-1), module, Recorder2::RECORD_PARAM, 0.0, 1.0, 0.0);
    RecordButton *btn = dynamic_cast<RecordButton *>(recordButton);
    Recorder2 *recorder = dynamic_cast<Recorder2 *>(module);

    btn->onPressCallback = [=]()
    {
        if (!recorder->isRecording) {
            recorder->startRecording();
        } else {
            recorder->stopRecording();
        }
    };
    addParam(recordButton);
    addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(xPos+6, yPos+5), module, Recorder2::RECORDING_LIGHT));

    yPos += 5;
    xPos = 10;
    for (unsigned int i = 0; i < 2; i++) {
        addInput(Port::create<PJ3410Port>(Vec(xPos, yPos), Port::INPUT, module, i));

        if (i % 2 ==0) {
            xPos += 37 + margin;
        } else {
            xPos = 10;
            yPos += 40 + margin;
        }
    }
}
Model *modelRecorder2 = Model::create<Recorder2, Recorder2Widget>("dekstop", "Recorder2", "Recorder 2", UTILITY_TAG);
