#include "controller.h"
#include "parameters.h"
#include "state_format.h"
#include "ColderatorControls.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstring.h"
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Colderator {

tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
    auto r = EditController::initialize(context);
    if (r != kResultOk)
        return r;

    auto addPercent = [&](const TChar* title, ParamID id) {
        auto* p = new RangeParameter(title, id, STR16("%"), 0.0, 100.0, 0.0);
        p->setPrecision(0);
        parameters.addParameter(p);
    };

    addPercent(STR16("Cold"), kCold);
    addPercent(STR16("Ice"), kIce);
    addPercent(STR16("Metal"), kMetal);
    addPercent(STR16("Frost"), kFrost);
    addPercent(STR16("Shiver"), kShiver);
    addPercent(STR16("Space"), kSpace);

    auto* output = new RangeParameter(STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, 0.0);
    output->setPrecision(1);
    parameters.addParameter(output);

    auto* bypass = new StringListParameter(
        STR16("Bypass"),
        kBypass,
        nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList);
    bypass->appendString(STR16("Off"));
    bypass->appendString(STR16("On"));
    parameters.addParameter(bypass);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer stream(state, kLittleEndian);
    float values[kComponentStateValueCount] {};
    int32 bypass = 0;

    if (!readComponentStatePayload(stream, values, bypass))
        return kResultFalse;

    const ParamID ids[kComponentStateValueCount] = {
        kCold, kIce, kMetal, kFrost, kShiver, kSpace, kOutput
    };

    for (int i = 0; i < kComponentStateValueCount; ++i)
        setParamNormalized(ids[i], values[i]);

    setParamNormalized(kBypass, bypass ? 1.0 : 0.0);
    return kResultOk;
}

Steinberg::IPlugView* PLUGIN_API Controller::createView(const char* name)
{
    Steinberg::ConstString viewName(name);
    if (viewName == Steinberg::Vst::ViewType::kEditor)
    {
        auto* editor = new VSTGUI::VST3Editor(this, "view", "colderator.uidesc");
        editor->setAllowedZoomFactors({1.0, 1.25, 1.5, 1.75, 2.0});
        editor->setZoomFactor(1.0);
        return editor;
    }
    return nullptr;
}

VSTGUI::CView* Controller::createCustomView(
    VSTGUI::UTF8StringPtr name,
    const VSTGUI::UIAttributes& attributes,
    const VSTGUI::IUIDescription*,
    VSTGUI::VST3Editor* editor)
{
    if (!name || !editor)
        return nullptr;

    VSTGUI::CPoint origin {0, 0};
    VSTGUI::CPoint size {60, 60};
    attributes.getPointAttribute("origin", origin);
    attributes.getPointAttribute("size", size);
    VSTGUI::CRect r(origin.x, origin.y, origin.x + size.x, origin.y + size.y);

    if (std::strcmp(name, "Faceplate") == 0)
        return new FrostFaceplate(r);

    auto knob = [&](const char* n, ParamID id, bool primary=false) -> VSTGUI::CView* {
        return std::strcmp(name, n) == 0 ? new FrostKnob(r, editor, id, primary) : nullptr;
    };

    if (auto* v = knob("Cold",   kCold, true)) return v;
    if (auto* v = knob("Ice",    kIce)) return v;
    if (auto* v = knob("Metal",  kMetal)) return v;
    if (auto* v = knob("Frost",  kFrost)) return v;
    if (auto* v = knob("Shiver", kShiver)) return v;
    if (auto* v = knob("Space",  kSpace)) return v;
    if (auto* v = knob("Output", kOutput)) return v;

    return nullptr;
}

} // namespace Colderator
