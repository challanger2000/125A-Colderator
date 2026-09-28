#include "controller.h"
#include "parameters.h"
#include "state_format.h"
#include "ColderatorControls.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstring.h"
#include <cstring>
#include <array>
#include <algorithm>

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

    auto addMaterial = [&](const TChar* title, ParamID id,
                           const std::array<const TChar*, kMaterialCount>& names) {
        auto* p = new StringListParameter(
            title, id, nullptr, ParameterInfo::kCanAutomate | ParameterInfo::kIsList);
        for (auto* name : names)
            p->appendString(name);
        parameters.addParameter(p);
    };

    addMaterial(STR16("Ice Material"), kIceMaterial,
        {STR16("Crystal"), STR16("Glass"), STR16("Crack"),
         STR16("Black Ice"), STR16("Icicle"), STR16("Shatter")});
    addMaterial(STR16("Metal Material"), kMetalMaterial,
        {STR16("Steel"), STR16("Pipe"), STR16("Chain"),
         STR16("Sheet"), STR16("Machine"), STR16("Rust")});
    addMaterial(STR16("Frost Material"), kFrostMaterial,
        {STR16("Hoarfrost"), STR16("Snow"), STR16("Crunch"),
         STR16("Frozen Dust"), STR16("Rime"), STR16("Deep Freeze")});
    addMaterial(STR16("Shiver Material"), kShiverMaterial,
        {STR16("Tremble"), STR16("Stiff"), STR16("Chatter"),
         STR16("Strain"), STR16("Spasm"), STR16("Numb")});
    addMaterial(STR16("Space Material"), kSpaceMaterial,
        {STR16("Morgue"), STR16("Church"), STR16("Bunker"),
         STR16("Ice Cave"), STR16("Cold Hall"), STR16("Cemetery")});

    auto addAtmosType = [&](const TChar* title, ParamID id) {
        auto* p = new StringListParameter(
            title, id, nullptr, ParameterInfo::kCanAutomate | ParameterInfo::kIsList);
        const TChar* names[kAtmosphereTypeCount] = {
            STR16("Wind"), STR16("Storm"), STR16("Drone"), STR16("Rumble"),
            STR16("Distant Metal"), STR16("Ice Cracks"), STR16("Air"),
            STR16("Frozen Landscape"), STR16("Frozen Bloom"), STR16("Machine")
        };
        for (auto* name : names)
            p->appendString(name);
        parameters.addParameter(p);
    };
    addAtmosType(STR16("Atmosphere A"), kAtmosAType);
    addPercent(STR16("Atmosphere A Amount"), kAtmosAAmount);
    addAtmosType(STR16("Atmosphere B"), kAtmosBType);
    addPercent(STR16("Atmosphere B Amount"), kAtmosBAmount);

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
    ComponentStatePayload payload {};

    if (!readComponentStatePayload(stream, payload))
        return kResultFalse;

    const ParamID ids[kComponentStateValueCount] = {
        kCold, kIce, kMetal, kFrost, kShiver, kSpace, kOutput
    };

    for (int i = 0; i < kComponentStateValueCount; ++i)
        setParamNormalized(ids[i], payload.values[i]);

    const ParamID materialIds[kComponentMaterialCount] = {
        kIceMaterial, kMetalMaterial, kFrostMaterial, kShiverMaterial, kSpaceMaterial
    };
    for (int i = 0; i < kComponentMaterialCount; ++i)
    {
        const int index = std::max(0, std::min(kMaterialCount - 1,
                                               static_cast<int>(payload.materials[i])));
        setParamNormalized(materialIds[i],
            static_cast<ParamValue>(index) / static_cast<ParamValue>(kMaterialCount - 1));
    }

    const ParamID atmosphereIds[kComponentAtmosphereCount] = {
        kAtmosAType, kAtmosAAmount, kAtmosBType, kAtmosBAmount
    };
    setParamNormalized(kAtmosAType, payload.atmosphere[0]);
    setParamNormalized(kAtmosAAmount, payload.atmosphere[1]);
    setParamNormalized(kAtmosBType, payload.atmosphere[2]);
    setParamNormalized(kAtmosBAmount, payload.atmosphere[3]);

    setParamNormalized(kBypass, payload.bypass ? 1.0 : 0.0);
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
    if (std::strcmp(name, "BrandLogo") == 0)
        return new FrostLogo(r);

    auto knob = [&](const char* n, ParamID id,
                    FrostKnob::Style style=FrostKnob::Style::Character) -> VSTGUI::CView* {
        return std::strcmp(name, n) == 0 ? new FrostKnob(r, editor, id, style) : nullptr;
    };

    if (auto* v = knob("Cold",   kCold, FrostKnob::Style::Main)) return v;
    if (auto* v = knob("Ice",    kIce)) return v;
    if (auto* v = knob("Metal",  kMetal)) return v;
    if (auto* v = knob("Frost",  kFrost)) return v;
    if (auto* v = knob("Shiver", kShiver)) return v;
    if (auto* v = knob("Space",  kSpace)) return v;
    if (auto* v = knob("Output", kOutput)) return v;
    if (auto* v = knob("AtmosAAmount", kAtmosAAmount, FrostKnob::Style::Utility)) return v;
    if (auto* v = knob("AtmosBAmount", kAtmosBAmount, FrostKnob::Style::Utility)) return v;

    return nullptr;
}

} // namespace Colderator
