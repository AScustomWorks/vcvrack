#include "aepelzen.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

	p->addModel(modelBurst);
	p->addModel(modelDice);
     p->addModel(modelErwin);
     p->addModel(modelGateSeq);
     p->addModel(modelQuadSeq);
     p->addModel(modelWalker);
	p->addModel(modelFolder);
//      p->addModel(modeldTime);
}
// this goes in dTime.cpp
//p->addModel(createModel<dTimeWidget>("Aepelzens Modules", "dTime", "dTime Sequencer", SEQUENCER_TAG));
