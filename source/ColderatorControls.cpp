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
    const double ox = r.left;
    const double oy = r.top;
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);

    const auto rect = [&](double x, double y, double w, double h) {
        return VSTGUI::CRect(ox + x, oy + y, ox + x + w, oy + y + h);
    };
    const auto line = [&](double x1, double y1, double x2, double y2,
                          const VSTGUI::CColor& color, double width = 1.0) {
        c->setFrameColor(color);
        c->setLineWidth(width);
        c->drawLine({ox + x1, oy + y1}, {ox + x2, oy + y2});
    };
    const auto panel = [&](double x, double y, double w, double h, double radius) {
        const auto shadow = rect(x + 1.5, y + 3.0, w, h);
        fillRound(c, shadow, radius, {96, 127, 146, 28}, {96, 127, 146, 0});
        const auto rr = rect(x, y, w, h);
        gradientRound(c, rr, radius,
                      {250, 253, 254, 255}, {232, 242, 248, 255},
                      {178, 207, 223, 255});
        auto inner = rr;
        inner.inset(2.0, 2.0);
        fillRound(c, inner, std::max(2.0, radius - 2.0),
                  {247, 251, 253, 38}, {255, 255, 255, 150});
    };
    const auto well = [&](double cx, double cy, double radius) {
        VSTGUI::CRect shadow(ox + cx - radius, oy + cy - radius + 3.0,
                            ox + cx + radius, oy + cy + radius + 3.0);
        c->setFillColor({70, 105, 126, 24});
        c->drawEllipse(shadow, VSTGUI::kDrawFilled);

        VSTGUI::CRect rr(ox + cx - radius, oy + cy - radius,
                         ox + cx + radius, oy + cy + radius);
        c->setFillColor({226, 238, 245, 255});
        c->drawEllipse(rr, VSTGUI::kDrawFilled);
        c->setFrameColor({145, 202, 230, 235});
        c->setLineWidth(1.5);
        c->drawEllipse(rr, VSTGUI::kDrawStroked);

        rr.inset(2.5, 2.5);
        c->setFrameColor({255, 255, 255, 210});
        c->setLineWidth(1.0);
        c->drawEllipse(rr, VSTGUI::kDrawStroked);
    };
    const auto selectorWell = [&](double x, double y, double w, double h) {
        const auto rr = rect(x, y, w, h);
        fillRound(c, rr, 7.0, {232, 242, 248, 255}, {164, 207, 228, 255});
        auto hi = rr;
        hi.inset(1.5, 1.5);
        fillRound(c, hi, 5.5, {247, 251, 253, 22}, {255, 255, 255, 165});
    };
    const auto screw = [&](double x, double y) {
        const auto sr = rect(x - 5.0, y - 5.0, 10.0, 10.0);
        c->setFillColor({220, 232, 239, 255});
        c->drawEllipse(sr, VSTGUI::kDrawFilled);
        c->setFrameColor({103, 142, 163, 230});
        c->setLineWidth(1.0);
        c->drawEllipse(sr, VSTGUI::kDrawStroked);
        line(x - 2.0, y, x + 2.0, y, {118, 151, 169, 230}, 1.0);
    };

    // Continuous snow-white chassis. All labels, logo and controls are separate views.
    gradientRound(c, r, 0.0,
                  {252, 254, 255, 255}, {229, 240, 247, 255},
                  {145, 188, 212, 255});

    panel(12, 10, 776, 67, 14.0);    // header
    panel(12, 87, 776, 219, 13.0);   // main control field
    panel(12, 316, 776, 64, 11.0);   // five material selectors
    panel(12, 390, 776, 158, 13.0);  // atmosphere / output field

    // Main row: COLD is deliberately dominant, followed by five equal character controls.
    well(85, 185, 61);
    for (double x : {225.0, 345.0, 465.0, 585.0, 705.0})
        well(x, 185, 45);

    // Quiet separators: enough hierarchy without turning the GUI into six boxed modules.
    for (double x : {165.0, 285.0, 405.0, 525.0, 645.0})
        line(x, 110, x, 284, {179, 207, 222, 115}, 1.0);

    // Exact five material controls; COLD intentionally has no material selector.
    for (double x : {179.0, 299.0, 419.0, 539.0, 659.0})
        selectorWell(x, 337, 92, 25);

    // Bottom field: Atmosphere A, Atmosphere B and master output.
    line(280, 410, 280, 528, {179, 207, 222, 130}, 1.0);
    line(560, 410, 560, 528, {179, 207, 222, 130}, 1.0);

    selectorWell(32, 437, 132, 31);
    well(225, 456, 36);
    selectorWell(312, 437, 132, 31);
    well(505, 456, 36);
    well(664, 455, 44);

    // Minimal physical fasteners only at the chassis corners.
    for (const auto& p : std::array<VSTGUI::CPoint, 4> {{
            {17.0, 17.0}, {783.0, 17.0}, {17.0, 543.0}, {783.0, 543.0}}})
        screw(p.x, p.y);

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
