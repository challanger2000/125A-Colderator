#include "ColderatorControls.h"
#include "branding_master.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include <vector>

namespace Colderator {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr VSTGUI::CColor kLogoSilver {126, 145, 156, 255};
constexpr VSTGUI::CColor kLogoRed {215, 25, 32, 255};

struct LogoSubpath {
    std::vector<VSTGUI::CPoint> points;
};

struct LogoPath {
    std::vector<LogoSubpath> subpaths;
    bool red {false};
};

std::vector<LogoPath> parseMasterLogo()
{
    std::vector<LogoPath> result;
    result.reserve(Branding::kMasterPathCount);

    for (const auto& source : Branding::kMasterPaths)
    {
        const std::string_view d {source.d};
        LogoPath path;
        path.red = source.red;

        const char* p = d.data();
        const char* end = d.data() + d.size();
        char command = 0;
        LogoSubpath* current = nullptr;

        while (p < end)
        {
            while (p < end && (std::isspace(static_cast<unsigned char>(*p)) || *p == ','))
                ++p;
            if (p >= end)
                break;

            if (std::isalpha(static_cast<unsigned char>(*p)))
            {
                command = *p++;
                if (command == 'Z' || command == 'z')
                {
                    command = 0;
                    current = nullptr;
                    continue;
                }
            }

            if (command != 'M' && command != 'm' && command != 'L' && command != 'l')
            {
                ++p;
                continue;
            }

            char* next = nullptr;
            const double x = std::strtod(p, &next);
            if (next == p || next > end)
                break;
            p = next;

            while (p < end && (std::isspace(static_cast<unsigned char>(*p)) || *p == ','))
                ++p;

            const double y = std::strtod(p, &next);
            if (next == p || next > end)
                break;
            p = next;

            if (command == 'M' || command == 'm')
            {
                path.subpaths.emplace_back();
                current = &path.subpaths.back();
                current->points.emplace_back(x, y);
                command = (command == 'M') ? 'L' : 'l';
            }
            else if (current)
            {
                current->points.emplace_back(x, y);
            }
        }

        if (!path.subpaths.empty())
            result.emplace_back(std::move(path));
    }

    return result;
}

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

struct FilmstripSpec
{
    const char* oneX;
    const char* one25X;
    const char* one5X;
    const char* twoX;
    double frameSize;
};

VSTGUI::SharedPointer<VSTGUI::CMultiFrameBitmap> loadFilmstrip(const FilmstripSpec& spec)
{
    VSTGUI::CMultiFrameBitmapDescription desc;
    desc.frameSize = {spec.frameSize, spec.frameSize};
    desc.numFrames = 128;
    desc.framesPerRow = 1;

    auto strip = VSTGUI::makeOwned<VSTGUI::CMultiFrameBitmap>(
        VSTGUI::CResourceDescription(spec.oneX), desc);
    if (!strip || !strip->isLoaded())
        return {};

    const auto addScale = [&](const char* name, double scale) {
        VSTGUI::CBitmap source{VSTGUI::CResourceDescription(name)};
        auto bitmap = source.getPlatformBitmap();
        if (!bitmap)
            return;
        bitmap->setScaleFactor(scale);
        strip->addBitmap(bitmap);
    };

    addScale(spec.one25X, 1.25);
    addScale(spec.one5X, 1.5);
    addScale(spec.twoX, 2.0);
    return strip;
}

const VSTGUI::SharedPointer<VSTGUI::CMultiFrameBitmap>& filmstripFor(FrostKnob::Style style)
{
    static const FilmstripSpec mainSpec {
        "colderator_main_120.png","colderator_main_150.png",
        "colderator_main_180.png","colderator_main_240.png",120.0
    };
    static const FilmstripSpec characterSpec {
        "colderator_character_88.png","colderator_character_110.png",
        "colderator_character_132.png","colderator_character_176.png",88.0
    };
    static const FilmstripSpec utilitySpec {
        "colderator_utility_72.png","colderator_utility_90.png",
        "colderator_utility_108.png","colderator_utility_144.png",72.0
    };

    static const auto mainStrip = loadFilmstrip(mainSpec);
    static const auto characterStrip = loadFilmstrip(characterSpec);
    static const auto utilityStrip = loadFilmstrip(utilitySpec);

    switch (style)
    {
        case FrostKnob::Style::Main: return mainStrip;
        case FrostKnob::Style::Utility: return utilityStrip;
        case FrostKnob::Style::Character:
        default: return characterStrip;
    }
}

FrostLogo::FrostLogo(const VSTGUI::CRect& r) : VSTGUI::CView(r)
{
    setMouseEnabled(false);
}

void FrostLogo::draw(VSTGUI::CDrawContext* c)
{
    static const auto logo = parseMasterLogo();
    const auto r = getViewSize();
    constexpr double masterWidth = 1774.0;
    constexpr double masterHeight = 887.0;
    const double scale = std::min(r.getWidth() / masterWidth, r.getHeight() / masterHeight);
    const double x0 = r.left + (r.getWidth() - masterWidth * scale) * 0.5;
    const double y0 = r.top + (r.getHeight() - masterHeight * scale) * 0.5;

    c->setDrawMode(VSTGUI::kAntiAliasing);
    for (const auto& sourcePath : logo)
    {
        auto* path = c->createGraphicsPath();
        if (!path)
            continue;

        for (const auto& subpath : sourcePath.subpaths)
        {
            if (subpath.points.empty())
                continue;

            const auto toView = [&](const VSTGUI::CPoint& p) {
                return VSTGUI::CPoint {x0 + p.x * scale, y0 + p.y * scale};
            };

            path->beginSubpath(toView(subpath.points.front()));
            for (std::size_t i = 1; i < subpath.points.size(); ++i)
                path->addLine(toView(subpath.points[i]));
            path->closeSubpath();
        }

        c->setFillColor(sourcePath.red ? kLogoRed : kLogoSilver);
        c->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathFilledEvenOdd);
        path->forget();
    }

    setDirty(false);
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
    const auto softBand = [&](double x, double y, double w, double h, double radius) {
        const auto rr = rect(x, y, w, h);
        gradientRound(c, rr, radius,
                      {252, 254, 255, 210}, {236, 245, 250, 210},
                      {178, 209, 226, 85});
    };
    const auto selectorBed = [&](double x, double y, double w, double h) {
        const auto rr = rect(x, y, w, h);
        fillRound(c, rr, 8.0, {238, 247, 251, 190}, {160, 204, 227, 105});
        auto hi = rr;
        hi.inset(1.0, 1.0);
        fillRound(c, hi, 7.0, {255, 255, 255, 14}, {255, 255, 255, 120});
    };
    const auto screw = [&](double x, double y) {
        const auto sr = rect(x - 3.8, y - 3.8, 7.6, 7.6);
        c->setFillColor({226, 236, 241, 235});
        c->drawEllipse(sr, VSTGUI::kDrawFilled);
        c->setFrameColor({120, 151, 169, 150});
        c->setLineWidth(0.8);
        c->drawEllipse(sr, VSTGUI::kDrawStroked);
    };

    // Snow-white chassis: nearly flat, cold and premium. No decorative knob wells.
    gradientRound(c, r, 0.0,
                  {253, 254, 255, 255}, {233, 243, 248, 255},
                  {164, 198, 216, 165});

    // Very restrained top plate and a single frost datum line.
    softBand(10, 9, 780, 66, 13.0);
    line(22, 83, 778, 83, {137, 190, 218, 90}, 1.0);

    // Main control field is intentionally open. Only a faint lower shelf anchors the row.
    line(26, 257, 774, 257, {153, 200, 224, 70}, 1.0);

    // Five material controls sit directly beneath their matching character controls.
    for (double x : {177.0, 297.0, 417.0, 537.0, 657.0})
        selectorBed(x, 274, 96, 28);

    // Bottom section is one continuous cold deck rather than three framed boxes.
    softBand(18, 343, 764, 190, 15.0);
    line(292, 365, 292, 515, {149, 198, 223, 70}, 1.0);
    line(574, 365, 574, 515, {149, 198, 223, 70}, 1.0);

    selectorBed(132, 427, 138, 30);
    selectorBed(414, 427, 138, 30);

    // Minimal corner hardware only.
    for (const auto& p : std::array<VSTGUI::CPoint, 4> {{
            {16.0, 16.0}, {784.0, 16.0}, {16.0, 544.0}, {784.0, 544.0}}})
        screw(p.x, p.y);

    setDirty(false);
}

FrostKnob::FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l,
                     int32_t tag, Style style)
: VSTGUI::CKnobBase(r,l,tag,nullptr), style_(style), filmstrip_(filmstripFor(style))
{
    setTransparency(true);
    setWantsFocus(true);
}

FrostKnob::FrostKnob(const FrostKnob& o)
: VSTGUI::CKnobBase(o), style_(o.style_), filmstrip_(o.filmstrip_)
{
    setTransparency(true);
    setWantsFocus(true);
}

void FrostKnob::draw(VSTGUI::CDrawContext* c)
{
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);

    if (filmstrip_ && filmstrip_->isLoaded())
    {
        const auto frameIndex = filmstrip_->normalizedValueToFrameIndex(
            static_cast<float>(getValueNormalized()));
        filmstrip_->drawFrame(c, frameIndex, getViewSize().getTopLeft());
        setDirty(false);
        return;
    }

    const auto r = getViewSize();
    const double d = std::min(r.getWidth(), r.getHeight());
    const bool primary = style_ == Style::Main;
    const double pad = primary ? 8.0 : 5.0;
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
        c->setFrameColor(primary ? VSTGUI::CColor{91,164,205,255}
                                 : VSTGUI::CColor{132,174,197,255});
        c->setLineWidth(primary ? 3.0 : 2.0);
        c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
        path->forget();
    }

    const double start = 0.75 * kPi;
    const double sweep = 1.5 * kPi;
    const double angle = start + sweep * getValueNormalized();
    const auto center = ring.getCenter();
    const double radius = ring.getWidth() * 0.31;
    c->setFrameColor(primary ? VSTGUI::CColor{20,132,193,255}
                             : VSTGUI::CColor{49,112,149,255});
    c->setLineWidth(primary ? 4.0 : 3.0);
    c->drawLine(center,{center.x + std::cos(angle)*radius,
                        center.y + std::sin(angle)*radius});
    setDirty(false);
}

} // namespace Colderator
