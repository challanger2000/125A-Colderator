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

struct BitmapSpec
{
    const char* oneX;
    const char* one5X;
};

VSTGUI::SharedPointer<VSTGUI::CBitmap> loadBitmapPair(const BitmapSpec& spec)
{
    auto bitmap = VSTGUI::makeOwned<VSTGUI::CBitmap>(
        VSTGUI::CResourceDescription(spec.oneX));
    if (!bitmap || !bitmap->isLoaded())
        return {};

    VSTGUI::CBitmap hiDpi {VSTGUI::CResourceDescription(spec.one5X)};
    if (auto platform = hiDpi.getPlatformBitmap())
    {
        platform->setScaleFactor(1.5);
        bitmap->addBitmap(platform);
    }
    return bitmap;
}

const VSTGUI::SharedPointer<VSTGUI::CBitmap>& faceplateBitmap()
{
    static const BitmapSpec spec {
        "colderator_faceplate_100.png", "colderator_faceplate_150.png"
    };
    static const auto bitmap = loadBitmapPair(spec);
    return bitmap;
}

const VSTGUI::SharedPointer<VSTGUI::CBitmap>& bodyFor(FrostKnob::Style style)
{
    static const BitmapSpec mainSpec {
        "colderator_knob_main_100.png", "colderator_knob_main_150.png"
    };
    static const BitmapSpec characterSpec {
        "colderator_knob_character_100.png", "colderator_knob_character_150.png"
    };
    static const BitmapSpec utilitySpec {
        "colderator_knob_utility_100.png", "colderator_knob_utility_150.png"
    };

    static const auto mainBody = loadBitmapPair(mainSpec);
    static const auto characterBody = loadBitmapPair(characterSpec);
    static const auto utilityBody = loadBitmapPair(utilitySpec);

    switch (style)
    {
        case FrostKnob::Style::Main: return mainBody;
        case FrostKnob::Style::Utility: return utilityBody;
        case FrostKnob::Style::Character:
        default: return characterBody;
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

FrostFaceplate::FrostFaceplate(const VSTGUI::CRect& r)
: VSTGUI::CView(r),
  faceplate_(VSTGUI::makeOwned<VSTGUI::CBitmap>(
      VSTGUI::CResourceDescription("Colderator_Frostplate_800x560.png")))
{
    setMouseEnabled(false);
}

FrostFaceplate::FrostFaceplate(const FrostFaceplate& o)
: VSTGUI::CView(o), faceplate_(o.faceplate_)
{
    setMouseEnabled(false);
}

void FrostFaceplate::draw(VSTGUI::CDrawContext* c)
{
    const auto r = getViewSize();

    if (faceplate_ && faceplate_->isLoaded())
    {
        const auto source = faceplate_->getSize();
        if (source.x > 0.0 && source.y > 0.0)
        {
            c->setBitmapInterpolationQuality(VSTGUI::BitmapInterpolationQuality::kHigh);
            c->fillRectWithBitmap(
                faceplate_,
                VSTGUI::CRect(0.0, 0.0, source.x, source.y),
                r,
                1.0f);
            setDirty(false);
            return;
        }
    }

    // Safe fallback only if the image resource cannot be loaded.
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);
    gradientRound(c, r, 0.0,
                  {252, 254, 255, 255},
                  {239, 247, 251, 255},
                  {196, 216, 226, 255});
    setDirty(false);
}

FrostIcicle::FrostIcicle(const VSTGUI::CRect& r)
: VSTGUI::CView(r),
  bitmap_(VSTGUI::makeOwned<VSTGUI::CBitmap>(
      VSTGUI::CResourceDescription("colderator_icicle_broken.png")))
{
    setMouseEnabled(false);
}

FrostIcicle::FrostIcicle(const FrostIcicle& o)
: VSTGUI::CView(o), bitmap_(o.bitmap_)
{
    setMouseEnabled(false);
}

void FrostIcicle::draw(VSTGUI::CDrawContext* c)
{
    if (!bitmap_ || !bitmap_->isLoaded())
    {
        setDirty(false);
        return;
    }

    const auto r = getViewSize();
    const auto source = bitmap_->getSize();
    if (source.x <= 0.0 || source.y <= 0.0)
    {
        setDirty(false);
        return;
    }

    const double scale = std::min(r.getWidth() / source.x, r.getHeight() / source.y);
    const double w = source.x * scale;
    const double h = source.y * scale;
    const double x = r.left + (r.getWidth() - w) * 0.5;
    const double y = r.top + (r.getHeight() - h) * 0.5;

    c->setBitmapInterpolationQuality(VSTGUI::BitmapInterpolationQuality::kHigh);
    c->fillRectWithBitmap(
        bitmap_,
        VSTGUI::CRect(0.0, 0.0, source.x, source.y),
        VSTGUI::CRect(x, y, x + w, y + h),
        1.0f);

    setDirty(false);
}

FrostKnob::FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l,
                     int32_t tag, Style style)
: VSTGUI::CKnobBase(r,l,tag,nullptr), style_(style), body_(bodyFor(style))
{
    setTransparency(true);
    setWantsFocus(true);
}

FrostKnob::FrostKnob(const FrostKnob& o)
: VSTGUI::CKnobBase(o), style_(o.style_), body_(o.body_)
{
    setTransparency(true);
    setWantsFocus(true);
}

void FrostKnob::draw(VSTGUI::CDrawContext* c)
{
    c->setDrawMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode);

    // Colderator winter-white control based on the approved JKnobMan direction:
    // ice-blue scale ring, white rotating handle, dark blue-grey pointer and
    // a pronounced soft-looking drop shadow. Kept vector-drawn here so all
    // declared editor zoom factors remain crisp.
    const auto r = getViewSize();
    const double d = std::min(r.getWidth(), r.getHeight());
    const auto center = r.getCenter();

    const double pad = style_ == Style::Main ? d * 0.035 :
                       style_ == Style::Utility ? d * 0.045 : d * 0.04;

    const VSTGUI::CRect outer {
        r.left + pad, r.top + pad,
        r.right - pad, r.bottom - pad
    };

    // Strong shadow: the main separation mechanism on the winter-white panel.
    const double shadow = style_ == Style::Main ? d * 0.055 : d * 0.05;
    const VSTGUI::CRect shadowRect {
        outer.left + shadow, outer.top + shadow,
        outer.right + shadow, outer.bottom + shadow
    };
    c->setFillColor({42, 58, 67, 82});
    c->drawEllipse(shadowRect, VSTGUI::kDrawFilled);

    // Fixed ice-blue scale ring (R165/G205/B225 from the approved visual pass).
    c->setFillColor({165, 205, 225, 255});
    c->drawEllipse(outer, VSTGUI::kDrawFilled);

    // Open the scale ring at the bottom, like the JKnobMan source.
    const double gapY = center.y + d * 0.365;
    c->setFillColor({251, 253, 254, 255});
    c->drawRect(
        VSTGUI::CRect(outer.left - 2.0, gapY, outer.right + 2.0, outer.bottom + 2.0),
        VSTGUI::kDrawFilled);

    // White scale marks across the 280-degree travel.
    const double start = 130.0 * kPi / 180.0;
    const double sweep = 280.0 * kPi / 180.0;
    const double ringOuter = d * 0.445;
    const double ringInner = d * 0.385;
    c->setFrameColor({255, 255, 255, 245});
    c->setLineWidth(std::max(1.5, d * 0.025));
    for (int i = 0; i <= 10; ++i)
    {
        const double a = start + sweep * (static_cast<double>(i) / 10.0);
        const VSTGUI::CPoint p1 {
            center.x + std::cos(a) * ringInner,
            center.y + std::sin(a) * ringInner
        };
        const VSTGUI::CPoint p2 {
            center.x + std::cos(a) * ringOuter,
            center.y + std::sin(a) * ringOuter
        };
        c->drawLine(p1, p2);
    }

    // Metallic rim and a simple directional highlight/shade pair.
    const double ringInset = d * 0.105;
    const VSTGUI::CRect metal {
        r.left + ringInset, r.top + ringInset,
        r.right - ringInset, r.bottom - ringInset
    };
    c->setFillColor({221, 226, 229, 255});
    c->drawEllipse(metal, VSTGUI::kDrawFilled);
    c->setFrameColor({102, 112, 118, 255});
    c->setLineWidth(std::max(1.3, d * 0.018));
    c->drawEllipse(metal, VSTGUI::kDrawStroked);

    const double faceInset = d * 0.135;
    const VSTGUI::CRect shadeFace {
        r.left + faceInset + d * 0.035, r.top + faceInset + d * 0.025,
        r.right - faceInset + d * 0.035, r.bottom - faceInset + d * 0.025
    };
    c->setFillColor({135, 139, 142, 255});
    c->drawEllipse(shadeFace, VSTGUI::kDrawFilled);

    const VSTGUI::CRect face {
        r.left + faceInset, r.top + faceInset,
        r.right - faceInset - d * 0.055, r.bottom - faceInset - d * 0.055
    };
    c->setFillColor({250, 252, 253, 255});
    c->drawEllipse(face, VSTGUI::kDrawFilled);

    // The supplied 101-frame source uses -140..+140 degrees.
    const double angle = start + sweep * getValueNormalized();

    // Rotating white handle opposite the dark pointer.
    const double hx = std::cos(angle);
    const double hy = std::sin(angle);
    const double px = -hy;
    const double py = hx;
    const double handleLen = d * 0.40;
    const double handleHalfWidth = d * 0.15;

    const VSTGUI::CPoint h0 {center.x - px * handleHalfWidth,
                             center.y - py * handleHalfWidth};
    const VSTGUI::CPoint h1 {center.x + px * handleHalfWidth,
                             center.y + py * handleHalfWidth};
    const VSTGUI::CPoint backCenter {center.x - hx * handleLen,
                                     center.y - hy * handleLen};
    const VSTGUI::CPoint h2 {backCenter.x + px * handleHalfWidth,
                             backCenter.y + py * handleHalfWidth};
    const VSTGUI::CPoint h3 {backCenter.x - px * handleHalfWidth,
                             backCenter.y - py * handleHalfWidth};

    auto* hp = c->createGraphicsPath();
    if (hp)
    {
        hp->beginSubpath(h0);
        hp->addLine(h1);
        hp->addLine(h2);
        hp->addLine(h3);
        hp->closeSubpath();
        c->setFillColor({252, 253, 254, 255});
        c->drawGraphicsPath(hp, VSTGUI::CDrawContext::kPathFilled);
        hp->forget();
    }

    // Dark blue-grey pointer: R43/G58/B67 from the approved visual pass.
    const double pointerInner = d * 0.045;
    const double pointerOuter = d * 0.315;
    const VSTGUI::CPoint p1 {
        center.x + hx * pointerInner,
        center.y + hy * pointerInner
    };
    const VSTGUI::CPoint p2 {
        center.x + hx * pointerOuter,
        center.y + hy * pointerOuter
    };
    c->setFrameColor({43, 58, 67, 255});
    c->setLineWidth(style_ == Style::Main ? d * 0.075 :
                    style_ == Style::Utility ? d * 0.065 : d * 0.07);
    c->drawLine(p1, p2);

    setDirty(false);
}

} // namespace Colderator
