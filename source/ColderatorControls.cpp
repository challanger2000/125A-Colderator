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

VSTGUI::SharedPointer<VSTGUI::CBitmap> loadFaceplateBitmap()
{
    auto bitmap = VSTGUI::makeOwned<VSTGUI::CBitmap>(
        VSTGUI::CResourceDescription("Colderator_Faceplate_800x560.png"));
    if (!bitmap || !bitmap->isLoaded())
        return {};

    VSTGUI::CBitmap hiDpi {
        VSTGUI::CResourceDescription("Colderator_Faceplate_1200x840.png")
    };
    if (auto platform = hiDpi.getPlatformBitmap())
    {
        platform->setScaleFactor(1.5);
        bitmap->addBitmap(platform);
    }
    return bitmap;
}

const VSTGUI::SharedPointer<VSTGUI::CMultiFrameBitmap>& colderatorKnobStrip(FrostKnob::Style style)
{
    static const auto mainStrip = VSTGUI::makeOwned<VSTGUI::CMultiFrameBitmap>(
        VSTGUI::CResourceDescription("Colderator_Knob_120x120_101f.png"),
        VSTGUI::CMultiFrameBitmapDescription {
            {120.0, 120.0},
            101,
            1
        });

    static const auto characterStrip = VSTGUI::makeOwned<VSTGUI::CMultiFrameBitmap>(
        VSTGUI::CResourceDescription("Colderator_Knob_88x88_101f.png"),
        VSTGUI::CMultiFrameBitmapDescription {
            {88.0, 88.0},
            101,
            1
        });

    static const auto utilityStrip = VSTGUI::makeOwned<VSTGUI::CMultiFrameBitmap>(
        VSTGUI::CResourceDescription("Colderator_Knob_72x72_101f.png"),
        VSTGUI::CMultiFrameBitmapDescription {
            {72.0, 72.0},
            101,
            1
        });

    switch (style)
    {
        case FrostKnob::Style::Main:      return mainStrip;
        case FrostKnob::Style::Utility:   return utilityStrip;
        case FrostKnob::Style::Character: return characterStrip;
    }
    return characterStrip;
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
  faceplate_(loadFaceplateBitmap())
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
              {18, 38, 51, 238},
              {142, 181, 201, 255},
              1.0);
    c->setFont(VSTGUI::kNormalFontSmaller);
    c->setFontColor({234, 247, 255, 255});
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
: VSTGUI::CAnimKnob(r, l, tag, colderatorKnobStrip(style).get()),
  style_(style),
  strip_(colderatorKnobStrip(style))
{
    // The JKnobMan artwork covers -140..+140 degrees.
    setStartAngle(static_cast<float>(130.0 / 180.0 * kPi));
    setRangeAngle(static_cast<float>(280.0 / 180.0 * kPi));
    setTransparency(true);
    setWantsFocus(true);
}

FrostKnob::FrostKnob(const FrostKnob& o)
: VSTGUI::CAnimKnob(o), style_(o.style_), strip_(o.strip_)
{
    setTransparency(true);
    setWantsFocus(true);
}


} // namespace Colderator
