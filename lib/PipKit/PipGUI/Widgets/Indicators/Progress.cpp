#include <PipGUI/Core/GUI.hpp>
#include <PipGUI/Graphics/Utils/Colors.hpp>
#include <PipGUI/Graphics/Utils/Easing.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kIndeterminatePeriodMs = 1600U;
        constexpr uint32_t kShimmerPeriodMs = 2000U;
        constexpr int16_t kSpritePad = 2;
        constexpr uint8_t kDefaultBaseBoost = 10;
        constexpr uint8_t kShimmerPeakIntensity = 110;
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kCircleProgressStartDeg = 32.0f;
        constexpr float kCircleProgressGapVisibleDeg = 12.0f;
        constexpr float kCircleProgressGapMinDeg = 20.0f;
        constexpr float kCircleProgressGapMaxDeg = 42.0f;

        struct CircularProgressShimmerShader
        {
            uint16_t fillColor;
            float fillStart;
            float fillSpan;
            float shimmerCenter;
            float halfBand;
            float segmentSpan;
            int segmentCount;
        };

        [[nodiscard]] uint16_t blendWhite(uint16_t color, uint8_t intensity)
        {
            return intensity ? (uint16_t)detail::blend565(color, (uint16_t)0xFFFF, intensity) : color;
        }

        [[nodiscard]] uint16_t lighten565Progress(uint16_t c, uint8_t amount)
        {
            uint16_t a = static_cast<uint16_t>(amount) * 8U;
            if (a > 255U)
                a = 255U;
            return static_cast<uint16_t>(detail::blend565(c, static_cast<uint16_t>(0xFFFF), static_cast<uint8_t>(a)));
        }

        [[nodiscard]] uint8_t clampProgressValue(uint8_t value) noexcept
        {
            return value > 100 ? 100 : value;
        }

        [[nodiscard]] int16_t fillWidthForValue(int16_t width, uint8_t value) noexcept
        {
            if (width <= 0)
                return 0;
            return static_cast<int16_t>((width * static_cast<uint16_t>(clampProgressValue(value))) / 100U);
        }

        [[nodiscard]] float fillSpanForValue(uint8_t value) noexcept
        {
            return static_cast<float>(clampProgressValue(value)) * 3.6f;
        }

        [[nodiscard]] float circularProgressCapSweepDeg(int16_t r, uint8_t thickness) noexcept
        {
            if (r <= 0)
                return 0.0f;

            float capRadius = static_cast<float>(thickness) * 0.5f;
            float centerRadius = static_cast<float>(r) - capRadius + 0.5f;
            if (centerRadius < (capRadius + 0.5f))
                centerRadius = capRadius + 0.5f;

            float ratio = capRadius / centerRadius;
            if (ratio > 0.98f)
                ratio = 0.98f;
            if (ratio < 0.0f)
                ratio = 0.0f;

            return 2.0f * asinf(ratio) * 180.0f / kPi;
        }

        [[nodiscard]] float resolveCircularProgressGapDeg(int16_t r, uint8_t thickness) noexcept
        {
            float gapDeg = circularProgressCapSweepDeg(r, thickness) + kCircleProgressGapVisibleDeg;
            if (gapDeg < kCircleProgressGapMinDeg)
                gapDeg = kCircleProgressGapMinDeg;
            if (gapDeg > kCircleProgressGapMaxDeg)
                gapDeg = kCircleProgressGapMaxDeg;
            return gapDeg;
        }

        [[nodiscard]] float circularProgressMinSegmentDeg(int16_t r, uint8_t thickness) noexcept
        {
            float minDeg = circularProgressCapSweepDeg(r, thickness) * 0.8f + 4.0f;
            if (minDeg < 8.0f)
                minDeg = 8.0f;
            return minDeg;
        }

        [[nodiscard]] float circularProgressAvailableSweep(int16_t r, uint8_t thickness) noexcept
        {
            const float gapDeg = resolveCircularProgressGapDeg(r, thickness);
            const float sweep = 360.0f - (gapDeg * 2.0f);
            return sweep > 0.0f ? sweep : 0.0f;
        }

        [[nodiscard]] float normalizeCircularSweep(float sweepDeg) noexcept
        {
            while (sweepDeg < 0.0f)
                sweepDeg += 360.0f;
            while (sweepDeg >= 360.0f)
                sweepDeg -= 360.0f;
            return sweepDeg;
        }

        [[nodiscard]] float circularSweepBetween(float startDeg, float endDeg) noexcept
        {
            float sweep = normalizeCircularSweep(endDeg) - normalizeCircularSweep(startDeg);
            if (sweep < 0.0f)
                sweep += 360.0f;
            return sweep;
        }

        [[nodiscard]] float circularProgressSweepForValue(uint8_t value, int16_t r, uint8_t thickness) noexcept
        {
            const float availableSweep = circularProgressAvailableSweep(r, thickness);
            return (static_cast<float>(clampProgressValue(value)) * availableSweep) / 100.0f;
        }

        struct CircularProgressSegments
        {
            float fillStart = kCircleProgressStartDeg;
            float fillEnd = kCircleProgressStartDeg;
            float fillSweep = 0.0f;
            float gapDeg = kCircleProgressGapMinDeg;
            float baseStart = kCircleProgressStartDeg + kCircleProgressGapMinDeg;
            float baseEnd = kCircleProgressStartDeg - kCircleProgressGapMinDeg;
            bool hasFill = false;
            bool hasBase = true;
        };

        [[nodiscard]] CircularProgressSegments resolveCircularProgressSegments(float rawFillStart,
                                                                              float fillSweep,
                                                                              int16_t r,
                                                                              uint8_t thickness) noexcept
        {
            CircularProgressSegments seg;
            seg.gapDeg = resolveCircularProgressGapDeg(r, thickness);
            const float minSegmentDeg = circularProgressMinSegmentDeg(r, thickness);
            const float availableSweep = circularProgressAvailableSweep(r, thickness);

            seg.fillStart = normalizeCircularSweep(rawFillStart);
            if (fillSweep < 0.0f)
                fillSweep = 0.0f;
            if (fillSweep > availableSweep)
                fillSweep = availableSweep;

            seg.fillSweep = fillSweep;
            seg.hasFill = fillSweep >= minSegmentDeg;
            seg.fillEnd = seg.fillStart + fillSweep;

            if (seg.hasFill)
            {
                const float baseSweep = 360.0f - fillSweep - (seg.gapDeg * 2.0f);
                seg.baseStart = seg.fillEnd + seg.gapDeg;
                seg.baseEnd = seg.fillStart - seg.gapDeg;
                seg.hasBase = baseSweep >= minSegmentDeg;
            }
            else
            {
                seg.fillEnd = seg.fillStart;
                seg.baseStart = kCircleProgressStartDeg + seg.gapDeg;
                seg.baseEnd = kCircleProgressStartDeg - seg.gapDeg;
                seg.hasBase = (360.0f - seg.gapDeg * 2.0f) >= minSegmentDeg;
            }

            return seg;
        }

        [[nodiscard]] CircularProgressSegments resolveCircularProgressSegments(uint8_t value, int16_t r, uint8_t thickness) noexcept
        {
            const float fillSweep = circularProgressSweepForValue(value, r, thickness);
            return resolveCircularProgressSegments(kCircleProgressStartDeg, fillSweep, r, thickness);
        }

        [[nodiscard]] uint8_t shimmerIntensity(float dist, float halfSpan) noexcept
        {
            if (halfSpan <= 0.0f)
                return 0;
            float norm = dist / halfSpan;
            if (norm > 1.0f)
                norm = 1.0f;
            const float curve = 1.0f - (norm * norm);
            const uint8_t intensity = static_cast<uint8_t>(kShimmerPeakIntensity * curve);
            return intensity < 2 ? 0 : intensity;
        }

        [[nodiscard]] uint16_t circularProgressShimmerColorAtAngle(const void *ctx, float deg) noexcept
        {
            const auto &shader = *static_cast<const CircularProgressShimmerShader *>(ctx);
            if (shader.segmentCount <= 0 || shader.segmentSpan <= 0.0f)
                return shader.fillColor;

            float clampedDeg = circularSweepBetween(shader.fillStart, deg);
            if (clampedDeg < 0.0f)
                clampedDeg = 0.0f;
            if (clampedDeg >= shader.fillSpan)
                clampedDeg = shader.fillSpan;

            int index = static_cast<int>(clampedDeg / shader.segmentSpan);
            if (index >= shader.segmentCount)
                index = shader.segmentCount - 1;

            const float segStart = shader.segmentSpan * static_cast<float>(index);
            const float segEnd = (index == shader.segmentCount - 1) ? shader.fillSpan : (segStart + shader.segmentSpan);
            const float mid = (segStart + segEnd) * 0.5f;
            const uint8_t intensity = shimmerIntensity(fabsf(mid - shader.shimmerCenter), shader.halfBand);
            return intensity == 0 ? shader.fillColor : blendWhite(shader.fillColor, intensity);
        }

        void resolveLinearProgressFillRange(int16_t x, int16_t w,
                                            uint8_t value,
                                            ProgressAnim anim,
                                            uint32_t now,
                                            int16_t &fillLeft,
                                            int16_t &fillRight) noexcept
        {
            fillLeft = x;
            fillRight = x;
            if (w <= 0)
                return;

            if (anim == Indeterminate)
            {
                int16_t segmentW = w / 4;
                if (segmentW < 20)
                    segmentW = 20;
                if (segmentW > w)
                    segmentW = w;

                const int32_t totalDist = static_cast<int32_t>(w) + segmentW;
                const float p = static_cast<float>(now % kIndeterminatePeriodMs) / static_cast<float>(kIndeterminatePeriodMs);
                const float eased = detail::motion::smoothstep(p);
                const int32_t offset = static_cast<int32_t>(eased * static_cast<float>(totalDist)) - segmentW;

                fillLeft = x + static_cast<int16_t>(offset);
                fillRight = fillLeft + segmentW;
                if (fillLeft < x)
                    fillLeft = x;
                if (fillRight > x + w)
                    fillRight = x + w;
                return;
            }

            const int16_t fillW = fillWidthForValue(w, value);
            if (fillW <= 0)
                return;
            fillRight = x + fillW;
        }

    }

    [[nodiscard]] static uint8_t clampRoundRadius(int16_t w, int16_t h, uint8_t radius) noexcept
    {
        int16_t limit = h / 2;
        if (w / 2 < limit)
            limit = w / 2;
        if (limit <= 0)
            return 0;
        return radius > static_cast<uint8_t>(limit) ? static_cast<uint8_t>(limit) : radius;
    }

    void GUI::drawLinearProgressRange(int16_t x, int16_t y, int16_t w, int16_t h,
                                      int16_t fillLeft, int16_t fillRight,
                                      uint16_t baseColor, uint16_t fillColor,
                                      uint8_t radius)
    {
        if (w <= 0 || h <= 0)
            return;

        const uint8_t rounded = clampRoundRadius(w, h, radius);
        fillRoundRect(x, y, w, h, rounded, baseColor);

        if (fillLeft < x)
            fillLeft = x;
        if (fillRight > x + w)
            fillRight = x + w;
        if (fillRight <= fillLeft)
            return;

        const int16_t fillW = fillRight - fillLeft;
        fillRoundRect(fillLeft, y, fillW, h, clampRoundRadius(fillW, h, rounded), fillColor);
    }

    void GUI::drawProgress(int16_t x, int16_t y,
                           int16_t w, int16_t h,
                           uint8_t value,
                           uint16_t baseColor,
                           uint16_t fillColor,
                           uint8_t radius,
                           ProgressAnim anim)
    {
        if (w <= 0 || h <= 0)
            return;
        value = clampProgressValue(value);

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        if (!getDrawTarget())
            return;

        if (x == center)
            x = AutoX(w);
        if (y == center)
            y = AutoY(h);

        const uint8_t rounded = clampRoundRadius(w, h, radius);
        int16_t fillLeft = x;
        int16_t fillRight = x;
        resolveLinearProgressFillRange(x, w, value, anim, nowMs(), fillLeft, fillRight);
        drawLinearProgressRange(x, y, w, h, fillLeft, fillRight, baseColor, fillColor, rounded);

        if (anim == Indeterminate)
            return;

        const int16_t fillW = fillRight - x;
        if (fillW <= 0)
            return;

        if (anim == ProgressAnimNone)
            return;

        const uint32_t now = nowMs();

        auto getCornerOffset = [&](int16_t px) -> int16_t
        {
            int16_t offsetLeft = 0;
            int16_t offsetRight = 0;

            if (px < x + rounded)
            {
                int16_t dx = (x + rounded) - px - 1;
                if (dx < 0)
                    dx = 0;
                const float dy = sqrtf(static_cast<float>(rounded * rounded) - static_cast<float>(dx * dx));
                offsetLeft = static_cast<int16_t>(rounded - dy);
            }

            const int16_t rightCircleCenter = x + fillW - rounded;
            if (px >= rightCircleCenter)
            {
                int16_t dx = px - rightCircleCenter;
                if (dx >= rounded)
                    dx = rounded - 1;
                if (dx < 0)
                    dx = 0;

                const float dy = sqrtf(static_cast<float>(rounded * rounded) - static_cast<float>(dx * dx));
                offsetRight = static_cast<int16_t>(rounded - dy);
            }

            return (offsetLeft > offsetRight) ? offsetLeft : offsetRight;
        };

        if (anim == Shimmer)
        {
            int16_t bandW = (w * 2) / 3;
            if (bandW < 40)
                bandW = 40;
            const int16_t totalDist = w + bandW;
            const int16_t progressShift = static_cast<int16_t>((now % kShimmerPeriodMs) * totalDist / kShimmerPeriodMs);

            const int16_t shimmerLeft = x - bandW + progressShift;
            const int16_t shimmerRight = shimmerLeft + bandW;
            const int16_t shimmerCenter = shimmerLeft + bandW / 2;
            const int16_t halfBand = bandW / 2;

            const int16_t barLeft = x;
            const int16_t barRight = x + fillW;

            const int16_t drawLeft = (shimmerLeft > barLeft) ? shimmerLeft : barLeft;
            const int16_t drawRight = (shimmerRight < barRight) ? shimmerRight : barRight;

            if (drawRight > drawLeft)
            {
                for (int16_t px = drawLeft; px < drawRight; ++px)
                {
                    const uint8_t intensity = shimmerIntensity(static_cast<float>(abs(px - shimmerCenter)), static_cast<float>(halfBand));
                    if (intensity == 0)
                        continue;
                    const uint16_t col = blendWhite(fillColor, intensity);

                    const int16_t offset = getCornerOffset(px);
                    const int16_t lineH = h - (offset * 2);
                    if (lineH > 0)
                        fillRect(px, y + offset, 1, lineH, col);
                }
            }
        }
    }

    void GUI::drawProgressTextSpan(int16_t x, int16_t y,
                                   int16_t w, int16_t h,
                                   const String &text,
                                   uint16_t textColor565,
                                   uint16_t bgColor565,
                                   TextAlign align,
                                   uint16_t fontPx,
                                   int16_t clipX,
                                   int16_t clipW)
    {
        if (w <= 0 || h <= 0 || text.length() == 0 || clipW <= 0)
            return;

        if (fontPx == 0)
        {
            uint16_t target = (uint16_t)((h > 6) ? (h - 4) : h);
            if (target < 8)
                target = 8;
            fontPx = target;
        }

        uint16_t prevSize = _typo.psdfSizePx;
        uint16_t prevWeight = _typo.psdfWeight;
        setFontSize(fontPx);
        setFontWeight(Medium);

        int16_t tw = 0, th = 0;
        measureText(text, tw, th);

        int16_t tx = x;
        int16_t ty = (int16_t)(y + (h - th) / 2);

        switch (align)
        {
        case TextAlign::Left:
            tx = (int16_t)(x + 4);
            break;
        case TextAlign::Right:
            tx = (int16_t)(x + w - tw - 4);
            break;
        case TextAlign::Center:
        default:
            tx = (int16_t)(x + (w - tw) / 2);
            break;
        }

        auto target = getDrawTarget();
        if (!target)
            return;
        const int16_t ox = _render.originX;
        const int16_t oy = _render.originY;
        const ClipState prevGuiClip = _clip;
        int32_t prevClipX = 0, prevClipY = 0, prevClipW = 0, prevClipH = 0;
        target->getClipRect(&prevClipX, &prevClipY, &prevClipW, &prevClipH);
        applyClip(clipX, y, clipW, h);
        target->setClipRect((int16_t)(clipX - ox), (int16_t)(y - oy), clipW, h);
        drawTextAligned(text, tx, ty, textColor565, bgColor565, TextAlign::Left);
        _clip = prevGuiClip;
        target->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);

        setFontSize(prevSize);
        setFontWeight(prevWeight);
    }

    void GUI::drawProgressAttachedText(int16_t x, int16_t y, int16_t w, int16_t h,
                                       int16_t fillLeft, int16_t fillRight,
                                       uint16_t baseColor, uint16_t fillColor,
                                       const String &text, uint16_t textColor565,
                                       TextAlign align, uint16_t fontPx)
    {
        if (text.length() == 0 || w <= 0 || h <= 0)
            return;

        const int16_t barLeft = x;
        const int16_t barRight = x + w;
        if (fillLeft < barLeft)
            fillLeft = barLeft;
        if (fillRight > barRight)
            fillRight = barRight;

        if (fillLeft > barLeft)
            drawProgressTextSpan(x, y, w, h, text, textColor565, baseColor, align, fontPx, barLeft, fillLeft - barLeft);
        if (fillRight > fillLeft)
            drawProgressTextSpan(x, y, w, h, text, textColor565, fillColor, align, fontPx, fillLeft, fillRight - fillLeft);
        if (fillRight < barRight)
            drawProgressTextSpan(x, y, w, h, text, textColor565, baseColor, align, fontPx, fillRight, barRight - fillRight);
    }

    void GUI::drawProgressDecorated(int16_t x, int16_t y, int16_t w, int16_t h,
                                    uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius,
                                    ProgressAnim anim,
                                    const String *label, uint16_t labelColor, TextAlign labelAlign, uint16_t labelFontPx,
                                    bool showPercent, uint16_t percentColor, TextAlign percentAlign, uint16_t percentFontPx)
    {
        if ((_flags.spriteEnabled && _disp.display && !_flags.inSpritePass) && ((label && label->length() > 0) || showPercent))
        {
            updateProgressDecorated(x, y, w, h, value, baseColor, fillColor, radius, anim,
                                    label, labelColor, labelAlign, labelFontPx,
                                    showPercent, percentColor, percentAlign, percentFontPx);
            return;
        }

        drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
        if ((!(label && label->length() > 0)) && !showPercent)
            return;

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t fillLeft = rx;
        int16_t fillRight = rx;
        resolveLinearProgressFillRange(rx, w, value, anim, nowMs(), fillLeft, fillRight);

        if (label && label->length() > 0)
            drawProgressAttachedText(rx, ry, w, h, fillLeft, fillRight, baseColor, fillColor, *label, labelColor, labelAlign, labelFontPx);

        if (showPercent)
        {
            value = clampProgressValue(value);
            String percent;
            (void)percent.reserve(5);
            percent += String((int)value);
            percent += "%";
            drawProgressAttachedText(rx, ry, w, h, fillLeft, fillRight, baseColor, fillColor, percent, percentColor, percentAlign, percentFontPx);
        }
    }

    void GUI::updateProgressDecorated(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius,
                                      ProgressAnim anim,
                                      const String *label, uint16_t labelColor, TextAlign labelAlign, uint16_t labelFontPx,
                                      bool showPercent, uint16_t percentColor, TextAlign percentAlign, uint16_t percentFontPx)
    {
        if ((!(label && label->length() > 0)) && !showPercent)
        {
            updateProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawProgressDecorated(x, y, w, h, value, baseColor, fillColor, radius, anim,
                                  label, labelColor, labelAlign, labelFontPx,
                                  showPercent, percentColor, percentAlign, percentFontPx);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);
        const int16_t radiusPad = clampRoundRadius(w, h, radius) + 1;
        const int16_t updatePad = radiusPad > kSpritePad ? radiusPad : kSpritePad;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect()
            .pos((int16_t)(rx - updatePad), (int16_t)(ry - updatePad))
            .size((int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2))
            .fill(_render.bgColor565)
            .draw();
        drawProgressDecorated(x, y, w, h, value, baseColor, fillColor, radius, anim,
                              label, labelColor, labelAlign, labelFontPx,
                              showPercent, percentColor, percentAlign, percentFontPx);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
        if (!prevRender)
            invalidateRect((int16_t)(rx - updatePad), (int16_t)(ry - updatePad), (int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2));
    }

    void GUI::drawProgress(int16_t x, int16_t y,
                           int16_t w, int16_t h,
                           uint8_t value,
                           uint16_t color,
                           uint8_t radius,
                           ProgressAnim anim)
    {
        const uint16_t base = lighten565Progress(color, kDefaultBaseBoost);
        drawProgress(x, y, w, h, value, base, color, radius, anim);
    }

    void GUI::drawCircleProgress(int16_t x, int16_t y,
                                 int16_t r,
                                 uint8_t thickness,
                                 uint8_t value,
                                 uint16_t baseColor,
                                 uint16_t fillColor,
                                 ProgressAnim anim)
    {
        if (r <= 0)
            return;
        value = clampProgressValue(value);
        if (thickness < 1)
            thickness = 1;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateCircleProgress(x, y, r, thickness, value, baseColor, fillColor, anim);
            return;
        }

        int16_t cx = x;
        int16_t cy = y;

        if (cx == center)
        {
            int16_t availW = _render.screenWidth;
            cx = availW / 2;
        }

        if (cy == center)
        {
            int16_t top = 0;
            int16_t availH = _render.screenHeight;
            int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0)
            {
                if (_status.pos == Top)
                {
                    top += sb;
                    availH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    availH -= sb;
                }
            }
            cy = top + availH / 2;
        }

        if (!getDrawTarget())
            return;

        if (anim == Indeterminate)
        {
            const uint32_t now = nowMs();
            const float segDeg = 70.0f;
            const float p = static_cast<float>(now % kIndeterminatePeriodMs) / static_cast<float>(kIndeterminatePeriodMs);
            const float turn = detail::motion::smoothstep(p);
            const float startDeg = normalizeCircularSweep(kCircleProgressStartDeg + turn * 360.0f);
            const CircularProgressSegments seg = resolveCircularProgressSegments(startDeg, segDeg, r, thickness);
            if (seg.hasBase)
                drawArc(cx, cy, r, thickness, seg.baseStart, seg.baseEnd, baseColor);

            drawArc(cx, cy, r, thickness, seg.fillStart, seg.fillEnd, fillColor);
            return;
        }

        const CircularProgressSegments seg = resolveCircularProgressSegments(value, r, thickness);
        if (seg.hasBase)
            drawArc(cx, cy, r, thickness, seg.baseStart, seg.baseEnd, baseColor);

        if (!seg.hasFill)
            return;

        float fillSpan = seg.fillSweep;
        if (fillSpan <= 0.0f)
            return;

        if (anim == ProgressAnimNone)
        {
            drawArc(cx, cy, r, thickness, seg.fillStart, seg.fillEnd, fillColor);
            return;
        }

        uint32_t now = nowMs();

        if (anim == Shimmer)
        {
            float bandW = (fillSpan * 2.0f) / 3.0f;
            if (bandW < 50.0f)
                bandW = 50.0f;
            const float totalDist = fillSpan + bandW;
            const float shift = static_cast<float>(now % kShimmerPeriodMs) * totalDist / static_cast<float>(kShimmerPeriodMs);
            const int segmentCount = (fillSpan < 120.0f) ? 12 : 18;

            const CircularProgressShimmerShader shader{
                fillColor,
                seg.fillStart,
                fillSpan,
                -bandW * 0.5f + shift,
                bandW * 0.5f,
                fillSpan / static_cast<float>(segmentCount),
                segmentCount};

            drawArcShaded(cx, cy, r, thickness, seg.fillStart, seg.fillEnd,
                          circularProgressShimmerColorAtAngle, &shader,
                          true, false);
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 1, cy - r - 1, r * 2 + 3, r * 2 + 3);
    }

    void GUI::drawCircleProgress(int16_t x, int16_t y,
                                 int16_t r,
                                 uint8_t thickness,
                                 uint8_t value,
                                 uint16_t color,
                                 ProgressAnim anim)
    {
        const uint16_t base = lighten565Progress(color, kDefaultBaseBoost);
        drawCircleProgress(x, y, r, thickness, value, base, color, anim);
    }

    void GUI::updateProgress(int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             uint8_t value,
                             uint16_t baseColor,
                             uint16_t fillColor,
                             uint8_t radius,
                             ProgressAnim anim)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);
        const int16_t radiusPad = clampRoundRadius(w, h, radius) + 1;
        const int16_t updatePad = radiusPad > kSpritePad ? radiusPad : kSpritePad;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect()
            .pos((int16_t)(rx - kSpritePad), (int16_t)(ry - kSpritePad))
            .size((int16_t)(w + kSpritePad * 2), (int16_t)(h + kSpritePad * 2))
            .fill(_render.bgColor565)
            .draw();
        drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect((int16_t)(rx - kSpritePad), (int16_t)(ry - kSpritePad), (int16_t)(w + kSpritePad * 2), (int16_t)(h + kSpritePad * 2));
    }

    void GUI::updateProgress(int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             uint8_t value,
                             uint16_t color,
                             uint8_t radius,
                             ProgressAnim anim)
    {
        const uint16_t base = lighten565Progress(color, kDefaultBaseBoost);
        updateProgress(x, y, w, h, value, base, color, radius, anim);
    }

    void GUI::updateProgress(ProgressState &s,
                             int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             uint8_t value,
                             uint16_t baseColor,
                             uint16_t fillColor,
                             uint8_t radius,
                             ProgressAnim anim)
    {
        if (w <= 0 || h <= 0)
            return;

        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);
        const int16_t edgePad = clampRoundRadius(w, h, radius) + 1;
        const int16_t updatePad = edgePad > kSpritePad ? edgePad : kSpritePad;

        value = clampProgressValue(value);

        bool needFull = !s.inited || s.anim != anim;
        if (anim != ProgressAnimNone && anim != None)
            needFull = true;
        if (needFull)
        {
            bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;

            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;

            drawRect()
                .pos((int16_t)(rx - updatePad), (int16_t)(ry - updatePad))
                .size((int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2))
                .fill(_render.bgColor565)
                .draw();
            drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;

            if (!prevRender)
            {
                invalidateRect((int16_t)(rx - updatePad), (int16_t)(ry - updatePad), (int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2));
            }

            s.inited = true;
            s.value = value;
            s.anim = anim;
            return;
        }

        const int16_t fillWPrev = fillWidthForValue(w, s.value);
        const int16_t fillWNew = fillWidthForValue(w, value);

        int16_t l = (fillWPrev < fillWNew) ? fillWPrev : fillWNew;
        int16_t r = (fillWPrev > fillWNew) ? fillWPrev : fillWNew;
        if (l == r)
        {
            s.value = value;
            return;
        }

        int16_t cx = (int16_t)(rx + l - updatePad);
        int16_t cw = (int16_t)((r - l) + updatePad * 2);
        if (cx < rx - updatePad)
            cx = rx - updatePad;
        if (cx + cw > rx + w + updatePad)
            cw = (int16_t)((rx + w + updatePad) - cx);

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _render.sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int16_t ox = _render.originX;
        const int16_t oy = _render.originY;
        _render.sprite.setClipRect((int16_t)(cx - ox), (int16_t)((ry - updatePad) - oy), cw, (int16_t)(h + updatePad * 2));

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect()
            .pos(cx, (int16_t)(ry - updatePad))
            .size(cw, (int16_t)(h + updatePad * 2))
            .fill(_render.bgColor565)
            .draw();
        drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        _render.sprite.setClipRect(clipX, clipY, clipW, clipH);

        if (!prevRender)
        {
            invalidateRect(cx, (int16_t)(ry - updatePad), cw, (int16_t)(h + updatePad * 2));
        }

        s.value = value;
        s.anim = anim;
    }

    void GUI::updateCircleProgress(int16_t x, int16_t y,
                                   int16_t r,
                                   uint8_t thickness,
                                   uint8_t value,
                                   uint16_t baseColor,
                                   uint16_t fillColor,
                                   ProgressAnim anim)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawCircleProgress(x, y, r, thickness, value, baseColor, fillColor, anim);
            return;
        }

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        int16_t cx = x;
        int16_t cy = y;
        if (cx == center)
            cx = AutoX(0);
        if (cy == center)
            cy = AutoY(0);

        int16_t rr = (int16_t)(r + kSpritePad);
        drawRect()
            .pos((int16_t)(cx - rr), (int16_t)(cy - rr))
            .size((int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1))
            .fill(_render.bgColor565)
            .draw();
        drawCircleProgress(x, y, r, thickness, value, baseColor, fillColor, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(cx - rr), (int16_t)(cy - rr), (int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1));
        }
    }

    void GUI::updateCircleProgress(int16_t x, int16_t y,
                                   int16_t r,
                                   uint8_t thickness,
                                   uint8_t value,
                                   uint16_t color,
                                   ProgressAnim anim)
    {
        const uint16_t base = lighten565Progress(color, kDefaultBaseBoost);
        updateCircleProgress(x, y, r, thickness, value, base, color, anim);
    }
}
