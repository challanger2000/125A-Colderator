#include "ColderatorControls.h"
#include "branding_master.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/plugin-bindings/vst3editor.h"
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

const VSTGUI::SharedPointer<VSTGUI::CBitmap>& stripFor(FrostKnob::Style style)
{
    static const BitmapSpec mainSpec {
        "colderator_main_120.png", "colderator_main_180.png"
    };
    static const BitmapSpec characterSpec {
        "colderator_character_88.png", "colderator_character_132.png"
    };
    static const BitmapSpec utilitySpec {
        "colderator_utility_72.png", "colderator_utility_108.png"
    };

    static const auto mainStrip = loadBitmapPair(mainSpec);
    static const auto characterStrip = loadBitmapPair(characterSpec);
    static const auto utilityStrip = loadBitmapPair(utilitySpec);

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

FrostZoomButton::FrostZoomButton(const VSTGUI::CRect& r, VSTGUI::VST3Editor* editor)
: VSTGUI::CView(r), editor_(editor)
{
    setMouseEnabled(true);
}

FrostZoomButton::FrostZoomButton(const FrostZoomButton& o)
: VSTGUI::CView(o), editor_(o.editor_), zoomed_(o.zoomed_)
{
    setMouseEnabled(true);
}

void FrostZoomButton::draw(VSTGUI::CDrawContext* c)
{
    const auto r = getViewSize();
    fillRound(c, r, 8.0,
              {248, 252, 254, 230},
              {157, 194, 212, 255},
              1.0);
    c->setFont(VSTGUI::kNormalFontSmall);
    c->setFontColor({36, 68, 83, 255});
    c->drawString(zoomed_ ? "ZOOM 150%" : "ZOOM 100%", r,
                  VSTGUI::kCenterText, true);
    setDirty(false);
}

VSTGUI::CMouseEventResult FrostZoomButton::onMouseDown(
    VSTGUI::CPoint&, const VSTGUI::CButtonState& buttons)
{
    if (!buttons.isLeftButton() || !editor_)
        return VSTGUI::kMouseEventNotHandled;

    zoomed_ = !zoomed_;
    editor_->setZoomFactor(zoomed_ ? 1.5 : 1.0);
    invalid();
    return VSTGUI::kMouseEventHandled;
}

FrostKnob::FrostKnob(const VSTGUI::CRect& r, VSTGUI::IControlListener* l,
                     int32_t tag, Style style)
: VSTGUI::CKnobBase(r,l,tag,nullptr), style_(style), body_(stripFor(style))
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
    // Use the supplied JKnobMan artwork only. No rings, pointers, shadows or
    // other knob graphics are redrawn by VSTGUI.
    if (!body_ || !body_->isLoaded())
    {
        setDirty(false);
        return;
    }

    constexpr int32_t kFrames = 128;
    const auto bitmapSize = body_->getSize();
    if (bitmapSize.x <= 0.0 || bitmapSize.y <= 0.0)
    {
        setDirty(false);
        return;
    }

    const double frameWidth = bitmapSize.x;
    const double frameHeight = bitmapSize.y / static_cast<double>(kFrames);
    const int32_t frame = std::max<int32_t>(
        0, std::min<int32_t>(
            kFrames - 1,
            static_cast<int32_t>(std::lround(getValueNormalized() * (kFrames - 1)))));

    const VSTGUI::CRect source {
        0.0,
        frameHeight * static_cast<double>(frame),
        frameWidth,
        frameHeight * static_cast<double>(frame + 1)
    };

    c->setBitmapInterpolationQuality(VSTGUI::BitmapInterpolationQuality::kHigh);
    c->fillRectWithBitmap(body_, source, getViewSize(), 1.0f);
    setDirty(false);
}

} // namespace Colderator
