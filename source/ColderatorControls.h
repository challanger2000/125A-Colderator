#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/cbuttonstate.h"

namespace VSTGUI { class VST3Editor; }

namespace Colderator {

class FrostLogo final : public VSTGUI::CView
{
public:
    explicit FrostLogo(const VSTGUI::CRect& r);
    FrostLogo(const FrostLogo& o) : VSTGUI::CView(o) {}
    VSTGUI::CBaseObject* newCopy() const override { return new FrostLogo(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class FrostFaceplate final : public VSTGUI::CView
{
public:
    explicit FrostFaceplate(const VSTGUI::CRect& r);
    FrostFaceplate(const FrostFaceplate& o);
    VSTGUI::CBaseObject* newCopy() const override { return new FrostFaceplate(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
private:
    VSTGUI::SharedPointer<VSTGUI::CBitmap> faceplate_;
};


class FrostZoomButton final : public VSTGUI::CView
{
public:
    FrostZoomButton(const VSTGUI::CRect& r, VSTGUI::VST3Editor* editor);
    FrostZoomButton(const FrostZoomButton& o);
    VSTGUI::CBaseObject* newCopy() const override { return new FrostZoomButton(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where,
                                           const VSTGUI::CButtonState& buttons) override;
private:
    VSTGUI::VST3Editor* editor_ {nullptr};
    bool zoomed_ {false};
};

class FrostKnob final : public VSTGUI::CAnimKnob
{
public:
    enum class Style { Main, Character };

    FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l, int32_t tag,
              Style style=Style::Character);
    FrostKnob(const FrostKnob& o);
    VSTGUI::CBaseObject* newCopy() const override { return new FrostKnob(*this); }
private:
    Style style_ {Style::Character};
    VSTGUI::SharedPointer<VSTGUI::CMultiFrameBitmap> strip_;
};

} // namespace Colderator
