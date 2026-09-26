#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/cview.h"

namespace Colderator {

class FrostFaceplate final : public VSTGUI::CView
{
public:
    explicit FrostFaceplate(const VSTGUI::CRect& r);
    FrostFaceplate(const FrostFaceplate& o) : VSTGUI::CView(o) {}
    VSTGUI::CBaseObject* newCopy() const override { return new FrostFaceplate(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class FrostKnob final : public VSTGUI::CKnobBase
{
public:
    FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l, int32_t tag, bool primary=false);
    FrostKnob(const FrostKnob& o);
    VSTGUI::CBaseObject* newCopy() const override { return new FrostKnob(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
private:
    bool primary_ {false};
};

} // namespace Colderator
