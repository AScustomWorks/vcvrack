#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

extern Model *modelBurst;
extern Model *modelDice;
extern Model *modelErwin;
extern Model *modelFolder;
extern Model *modelGateSeq;
extern Model *modelQuadSeq;
extern Model *modelWalker;

struct Knob29 : RoundKnob {
	Knob29() {
		setSVG(SVG::load(assetPlugin(plugin, "res/knob_29px.svg")));
		box.size = Vec(29, 29);
	}
};

////////////////////
// module widgets
////////////////////

// struct dTimeWidget : ModuleWidget {
// 	dTimeWidget();
// };

