#include "dekstop.hpp"
#include <math.h>


Plugin *plugin;

void init(rack::Plugin *p) {
        plugin = p;
        p->slug = TOSTRING(SLUG);
        p->version = TOSTRING(VERSION);
        p->website = "https://github.com/dekstop/vcvrackplugins_dekstop";
        p->addModel(modelTriSEQ3);
        p->addModel(modelGateSEQ8);
        p->addModel(modelRecorder2);
        p->addModel(modelRecorder8);
}
