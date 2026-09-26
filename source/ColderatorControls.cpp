#include "ColderatorControls.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include <algorithm>
#include <cmath>

namespace Colderator {
namespace {
constexpr double kPi = 3.14159265358979323846;

void fillRound(VSTGUI::CDrawContext* c, const VSTGUI::CRect& r, double radius,
               const VSTGUI::CColor& fill, const VSTGUI::CColor& frame, double width=1.0)
{
    auto* p = c->createRoundRectGraphicsPath(r, radius);
    if (!p) return;
    c->setFillColor(fill);
    c->drawGraphicsPath(p, VSTGUI::CDrawContext::kPathFilled);
    c->setFrameColor(frame);
    c->setLineWidth(width);
    c->drawGraphicsPath(p, VSTGUI::CDrawContext::kPathStroked);
    p->forget();
}

void gradientRound(VSTGUI::CDrawContext* c, const VSTGUI::CRect& r, double radius,
                   const VSTGUI::CColor& top, const VSTGUI::CColor& bottom,
                   const VSTGUI::CColor& frame)
{
    auto* p = c->createRoundRectGraphicsPath(r, radius);
    if (!p) return;
    if (auto* g = VSTGUI::CGradient::create(0.0, 1.0, top, bottom))
    {
        c->fillLinearGradient(p, *g, r.getTopLeft(), r.getBottomLeft(), false);
        g->forget();
    }
    c->setFrameColor(frame);
    c->setLineWidth(1.0);
    c->drawGraphicsPath(p, VSTGUI::CDrawContext::kPathStroked);
    p->forget();
}
}

FrostFaceplate::FrostFaceplate(const VSTGUI::CRect& r) : VSTGUI::CView(r)
{
    setMouseEnabled(false);
}

void FrostFaceplate::draw(VSTGUI::CDrawContext* c)
{
    const auto r = getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);

    c->setFillColor({236, 244, 248, 255});
    c->drawRect(r, VSTGUI::kDrawFilled);

    VSTGUI::CRect header(r.left, r.top, r.right, r.top + 72.0);
    gradientRound(c, header, 0.0,
                  {250,253,255,255}, {218,232,241,255}, {178,204,220,255});

    fillRound(c, {18,88,244,438}, 14.0, {247,251,253,255}, {181,207,222,255});
    fillRound(c, {260,88,654,438}, 14.0, {242,249,252,255}, {173,202,220,255});
    fillRound(c, {670,88,782,438}, 14.0, {247,251,253,255}, {181,207,222,255});

    c->setFrameColor({255,255,255,180});
    c->setLineWidth(2.0);
    for (int y=102; y<430; y+=42)
        c->drawLine({278.0, static_cast<double>(y)}, {636.0, static_cast<double>(y)});

    c->setFrameColor({145,190,216,120});
    c->setLineWidth(1.0);
    c->drawLine({28,421},{232,421});
    c->drawLine({274,421},{640,421});

    setDirty(false);
}

FrostKnob::FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l,
                     int32_t tag, bool primary)
: VSTGUI::CKnobBase(r,l,tag,nullptr), primary_(primary)
{
    setTransparency(true);
    setWantsFocus(true);
}

FrostKnob::FrostKnob(const FrostKnob& o)
: VSTGUI::CKnobBase(o), primary_(o.primary_)
{
    setTransparency(true);
    setWantsFocus(true);
}

void FrostKnob::draw(VSTGUI::CDrawContext* c)
{
    const auto r = getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);

    const double d = std::min(r.getWidth(), r.getHeight());
    const double pad = primary_ ? 8.0 : 5.0;
    VSTGUI::CRect ring(r.left+pad, r.top+pad, r.left+d-pad, r.top+d-pad);

    VSTGUI::CRect shadow = ring;
    shadow.offset(2.0, 3.0);
    c->setFillColor({94,127,146,55});
    c->drawEllipse(shadow, VSTGUI::kDrawFilled);

    if (auto* path = c->createRoundRectGraphicsPath(ring, ring.getWidth()*0.5))
    {
        if (auto* g = VSTGUI::CGradient::create(0.0,1.0,
                VSTGUI::CColor{255,255,255,255},
                VSTGUI::CColor{184,205,217,255}))
        {
            c->fillLinearGradient(path,*g,ring.getTopLeft(),ring.getBottomLeft(),false);
            g->forget();
        }
        c->setFrameColor(primary_ ? VSTGUI::CColor{91,164,205,255}
                                  : VSTGUI::CColor{132,174,197,255});
        c->setLineWidth(primary_ ? 3.0 : 2.0);
        c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
        path->forget();
    }

    VSTGUI::CRect inner = ring;
    inner.inset(primary_ ? 17.0 : 12.0, primary_ ? 17.0 : 12.0);
    if (auto* p = c->createRoundRectGraphicsPath(inner, inner.getWidth()*0.5))
    {
        if (auto* g = VSTGUI::CGradient::create(0.0,1.0,
                VSTGUI::CColor{231,242,248,255},
                VSTGUI::CColor{151,180,197,255}))
        {
            c->fillLinearGradient(p,*g,inner.getTopLeft(),inner.getBottomLeft(),false);
            g->forget();
        }
        c->setFrameColor({91,126,146,255});
        c->setLineWidth(1.2);
        c->drawGraphicsPath(p,VSTGUI::CDrawContext::kPathStroked);
        p->forget();
    }

    const double start = 0.75 * kPi;
    const double sweep = 1.5 * kPi;
    const double angle = start + sweep * getValueNormalized();
    const auto center = inner.getCenter();
    const double radius = inner.getWidth() * 0.34;

    c->setFrameColor(primary_ ? VSTGUI::CColor{20,132,193,255}
                              : VSTGUI::CColor{49,112,149,255});
    c->setLineWidth(primary_ ? 4.0 : 3.0);
    c->drawLine(center,
                {center.x + std::cos(angle)*radius,
                 center.y + std::sin(angle)*radius});

    setDirty(false);
}

} // namespace Colderator
