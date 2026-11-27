/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <latte_debug.h>
#include "panelbackground.h"

// local
#include "theme.h"

// Qt
#include <QDebug>
#include <QImage>
#include <QtGlobal>

#define CENTERWIDTH 100
#define CENTERHEIGHT 50

#define BASELINESHADOWTHRESHOLD 5

#if defined(Q_CC_CLANG)
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wcast-align")
#endif

namespace Latte {
namespace PlasmaExtended {

PanelBackground::PanelBackground(Plasma::Types::Location edge, Theme *parent)
  : QObject(parent)
  , m_location(edge)
  , m_parentTheme(parent)
{
}

PanelBackground::~PanelBackground() {}

bool
PanelBackground::hasMask(KSvg::Svg *svg) const
{
    if (!svg) {
        return false;
    }

    return svg->hasElement(QStringLiteral("mask-topleft"));
}

int
PanelBackground::paddingTop() const
{ return m_paddingTop; }

int
PanelBackground::paddingLeft() const
{ return m_paddingLeft; }

int
PanelBackground::paddingBottom() const
{ return m_paddingBottom; }

int
PanelBackground::paddingRight() const
{ return m_paddingRight; }

int
PanelBackground::roundness() const
{ return m_roundness; }

int
PanelBackground::shadowSize() const
{ return m_shadowSize; }

float
PanelBackground::maxOpacity() const
{ return m_maxOpacity; }

QColor
PanelBackground::shadowColor() const
{ return m_shadowColor; }

QString
PanelBackground::prefixed(const QString &id)
{
    if (m_location == Plasma::Types::TopEdge) {
        return QStringLiteral("north-") + id;
    } else if (m_location == Plasma::Types::LeftEdge) {
        return QStringLiteral("west-") + id;
    } else if (m_location == Plasma::Types::BottomEdge) {
        return QStringLiteral("south-") + id;
    } else if (m_location == Plasma::Types::RightEdge) {
        return QStringLiteral("east-") + id;
    }

    return id;
}

QString
PanelBackground::element(KSvg::Svg *svg, const QString &id)
{
    if (!svg) {
        return QString();
    }

    if (svg->hasElement(prefixed(id))) {
        return prefixed(id);
    }

    if (svg->hasElement(id)) {
        return id;
    }

    return QString();
}

void
PanelBackground::updateMaxOpacity(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    QImage center = svg->image(QSize(CENTERWIDTH, CENTERHEIGHT), element(svg, QStringLiteral("center")));

    if (center.format() != QImage::Format_ARGB32_Premultiplied) {
        center.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    float alphasum{ 0 };

    //! calculating the mid opacity (this is needed in order to handle Oxygen
    //! that has different opacity levels in the same center element)
    for (int row = 0; row < 2; ++row) {
        QRgb *line = reinterpret_cast<QRgb *>(center.scanLine(row));

        for (int col = 0; col < CENTERWIDTH; ++col) {
            QRgb pixelData = line[col];
            alphasum += static_cast<float>(qAlpha(pixelData)) / 255.0f;
        }
    }

    m_maxOpacity = alphasum / static_cast<float>(2 * CENTERWIDTH);

    //! minimum acceptable panel background opacity is 1%. Such is a case is when
    //! panel background is fully transparent but it provides a border. In such case
    //! previous approach was identifying as background max opacity 0% and in such case
    //! all the upcoming calculations where returning a fully transparent plasma svg to the user
    m_maxOpacity = qMax(0.01f, m_maxOpacity);

    Q_EMIT maxOpacityChanged();
}

void
PanelBackground::updatePaddings(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    m_paddingTop = svg->elementSize(element(svg, QStringLiteral("top"))).height();
    m_paddingLeft = svg->elementSize(element(svg, QStringLiteral("left"))).width();
    m_paddingBottom = svg->elementSize(element(svg, QStringLiteral("bottom"))).height();
    m_paddingRight = svg->elementSize(element(svg, QStringLiteral("right"))).width();

    Q_EMIT paddingsChanged();
}

void
PanelBackground::updateRoundnessFromMask(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    bool topLeftCorner = (m_location == Plasma::Types::BottomEdge || m_location == Plasma::Types::RightEdge);

    QString cornerId = (topLeftCorner ? QStringLiteral("mask-topleft") : QStringLiteral("mask-bottomright"));
    QImage corner = svg->image(svg->elementSize(cornerId).toSize(), cornerId);

    if (corner.format() != QImage::Format_ARGB32_Premultiplied) {
        corner.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    int baseRow = (topLeftCorner ? corner.height() - 1 : 0);
    int baseCol = (topLeftCorner ? corner.width() - 1 : 0);

    int baseLineLength = 0;
    int roundnessLines = 0;

    if (topLeftCorner) {
        //! TOPLEFT corner
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(baseRow));
        QRgb basePoint = line[baseCol];

        QRgb *isRoundedLine = reinterpret_cast<QRgb *>(corner.scanLine(0));
        QRgb isRoundedPoint = isRoundedLine[0];

        //! If there is roundness, if that point is not fully transparent then
        //! there is no roundness
        if (qAlpha(isRoundedPoint) == 0) {

            if (qAlpha(basePoint) > 0) {
                //! calculate the mask baseLine length
                for (int c = baseCol; c >= 0; --c) {
                    QRgb point = line[c];

                    if (qAlpha(point) > 0) {
                        baseLineLength++;
                    } else {
                        break;
                    }
                }
            }

            qCDebug(lattePlasma) << " TOP LEFT CORNER MASK base line length :: " << baseLineLength;

            if (baseLineLength > 0) {
                int headLimitR = baseRow;
                int tailLimitR = baseRow;

                for (int r = baseRow - 1; r >= 0; --r) {
                    QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                    QRgb fpoint = line[baseCol];

                    if (qAlpha(fpoint) == 0) {
                        //! a line that is not part of the roundness because its first pixel is fully transparent
                        break;
                    }

                    headLimitR = r;
                }

                int c = qMax(0, corner.width() - baseLineLength);

                for (int r = baseRow - 1; r >= 0; --r) {
                    QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                    QRgb point = line[c];

                    if (qAlpha(point) != 255) {
                        tailLimitR = r;
                        break;
                    }
                }

                if (headLimitR != tailLimitR) {
                    roundnessLines = tailLimitR - headLimitR + 1;
                }
            }
        }
    } else {
        //! BOTTOMRIGHT CORNER
        //! it should be TOPRIGHT corner in that case
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(baseRow));
        QRgb basePoint = line[baseCol];

        QRgb *isRoundedLine = reinterpret_cast<QRgb *>(corner.scanLine(corner.height()) - 1);
        QRgb isRoundedPoint = isRoundedLine[corner.width() - 1];

        //! If there is roundness, if that point is not fully transparent then
        //! there is no roundness
        if (qAlpha(isRoundedPoint) == 0) {

            if (qAlpha(basePoint) > 0) {
                //! calculate the mask baseLine length
                for (int c = baseCol; c < corner.width(); ++c) {
                    QRgb point = line[c];

                    if (qAlpha(point) > 0) {
                        baseLineLength++;
                    } else {
                        break;
                    }
                }
            }

            qCDebug(lattePlasma) << " BOTTOM RIGHT CORNER MASK base line length :: " << baseLineLength;

            if (baseLineLength > 0) {
                int headLimitR = 0;
                int tailLimitR = 0;

                for (int r = baseRow + 1; r <= corner.height(); ++r) {
                    QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                    QRgb fpoint = line[baseCol];

                    if (qAlpha(fpoint) == 0) {
                        //! a line that is not part of the roundness because its first pixel is not transparent
                        break;
                    }

                    headLimitR = r;
                }

                int c = baseLineLength - 1;

                for (int r = baseRow + 1; r <= corner.height(); ++r) {
                    QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                    QRgb point = line[c];

                    if (qAlpha(point) != 255) {
                        tailLimitR = r;
                        break;
                    }
                }

                if (headLimitR != tailLimitR) {
                    roundnessLines = headLimitR - tailLimitR + 1;
                }
            }
        }
    }

    m_roundness = roundnessLines;
    Q_EMIT roundnessChanged();
}

void
PanelBackground::updateRoundnessFromShadows(KSvg::Svg *svg)
{
    //! 1.  Algorithm is choosing which corner shadow based on panel location
    //! 2.  For that corner discovers the maxOpacity (most solid shadow point) and
    //!     how pixels (distance) is to the most solid point, that is called [baseLineLength]
    //! 3.  After [2] the algorigthm for each next line calculates the maxOpacity
    //!     for that line and how many points are needed to reach there. If the points
    //!     to reach the line max opacity are shorter than baseLineLength then that line
    //!     is considered part of the roundness
    //! 3.1 Avoid zig-zag cases such as the Air plasma theme case. When the shadow is not
    //!     following a straight line until reaching the rounded part the algorithm is
    //!     considering as valid roundness only the last part of the discovered roundness and
    //!     ignores all the previous.
    //! 4.  Calculating the lines that are shorter than the baseline provides
    //!     the discovered roundness

    if (!svg) {
        return;
    }

    bool topLeftCorner = (m_location == Plasma::Types::BottomEdge || m_location == Plasma::Types::RightEdge);

    QString cornerId = (topLeftCorner ? QStringLiteral("shadow-topleft") : QStringLiteral("shadow-bottomright"));
    QImage corner = svg->image(svg->elementSize(cornerId).toSize(), cornerId);

    // Some themes advertise panel shadows without providing every corner asset.
    // The shadow algorithm reads a base scanline, so an empty image would turn
    // its -1 base row/column into an out-of-bounds access. Roundness is derived
    // state; zero is the safe fallback until a complete shadow asset is present.
    if (corner.isNull() || corner.width() == 0 || corner.height() == 0) {
        m_roundness = 0;
        Q_EMIT roundnessChanged();
        return;
    }

    if (corner.format() != QImage::Format_ARGB32_Premultiplied) {
        corner.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    int baseRow = (topLeftCorner ? corner.height() - 1 : 0);
    int baseCol = (topLeftCorner ? corner.width() - 1 : 0);

    int baseLineLength = 0;
    int roundnessLines = 0;

    if (topLeftCorner) {
        //! TOPLEFT corner
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(baseRow));
        QRgb basePoint = line[baseCol];

        int baseShadowMaxOpacity = 0;

        if (qAlpha(basePoint) == 0) {
            //! calculate the shadow maxOpacity in the base line
            //! and number of pixels to reach there
            for (int c = baseCol; c >= 0; --c) {
                QRgb point = line[c];

                if (qAlpha(point) > baseShadowMaxOpacity) {
                    baseShadowMaxOpacity = qAlpha(point);
                    baseLineLength = (baseCol - c + 1);
                }
            }
        }

        qCDebug(lattePlasma) << " TOP LEFT CORNER SHADOW base line length :: " << baseLineLength << " with max shadow opacity : " << baseShadowMaxOpacity;

        if (baseLineLength > 0) {
            for (int r = baseRow - 1; r >= 0; --r) {
                QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                QRgb fpoint = line[baseCol];

                if (qAlpha(fpoint) != 0) {
                    //! a line that is not part of the roundness because its first pixel is not transparent
                    break;
                }

                int transPixels = 0;
                int rowMaxOpacity = 0;

                for (int c = baseCol; c >= 0; --c) {
                    QRgb point = line[c];

                    if (qAlpha(point) > rowMaxOpacity) {
                        rowMaxOpacity = qAlpha(point);
                        continue;
                    }
                }

                for (int c = baseCol; c >= (baseCol - baseLineLength + 1); --c) {
                    QRgb point = line[c];

                    if (qAlpha(point) != rowMaxOpacity) {
                        transPixels++;
                        continue;
                    }

                    if (transPixels != baseLineLength) {
                        roundnessLines++;
                        break;
                    }
                }

                if (transPixels == baseLineLength) {
                    //! 3.1 avoid zig-zag shadows Air plasma theme case
                    roundnessLines = 0;
                }

            }
        }
    } else {
        //! BOTTOMRIGHT CORNER
        //! it should be TOPRIGHT corner in that case
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(baseRow));
        QRgb basePoint = line[baseCol];

        int baseShadowMaxOpacity = 0;

        if (qAlpha(basePoint) == 0) {
            //! calculate the base line transparent pixels
            for (int c = baseCol; c < corner.width(); ++c) {
                QRgb point = line[c];

                if (qAlpha(point) > baseShadowMaxOpacity) {
                    baseShadowMaxOpacity = qAlpha(point);
                    baseLineLength = c + 1;
                }
            }
        }

        qCDebug(lattePlasma) << " BOTTOM RIGHT CORNER SHADOW base line length :: " << baseLineLength << " with max shadow opacity : " << baseShadowMaxOpacity;

        if (baseLineLength > 0) {
            for (int r = baseRow + 1; r < corner.height(); ++r) {
                QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(r));
                QRgb fpoint = line[baseCol];

                if (qAlpha(fpoint) != 0) {
                    //! a line that is not part of the roundness because its first pixel is not transparent
                    break;
                }

                int transPixels = 0;
                int rowMaxOpacity = 0;

                for (int c = baseCol; c < corner.width(); ++c) {
                    QRgb point = line[c];

                    if (qAlpha(point) > rowMaxOpacity) {
                        rowMaxOpacity = qAlpha(point);
                        baseLineLength = c + 1;
                    }
                }

                for (int c = baseCol; c < baseLineLength; ++c) {
                    QRgb point = line[c];

                    if (qAlpha(point) != rowMaxOpacity) {
                        transPixels++;
                        continue;
                    }

                    if (transPixels != baseLineLength) {
                        roundnessLines++;
                        break;
                    }
                }

                if (transPixels == baseLineLength) {
                    //! 3.1 avoid zig-zag shadows Air plasma theme case
                    roundnessLines = 0;
                }

            }
        }
    }

    m_roundness = roundnessLines;
    Q_EMIT roundnessChanged();
}

void
PanelBackground::updateRoundnessFallback(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    QString cornerId = element(svg, (m_location == Plasma::Types::LeftEdge ? QStringLiteral("bottomright") : QStringLiteral("topleft")));
    QImage corner = svg->image(svg->elementSize(cornerId).toSize(), cornerId);

    if (corner.format() != QImage::Format_ARGB32_Premultiplied) {
        corner.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    int discovRow = (m_location == Plasma::Types::LeftEdge ? corner.height() - 1 : 0);
    int round{ 0 };

    int minOpacity = m_maxOpacity * 255;

    if (m_location == Plasma::Types::BottomEdge || m_location == Plasma::Types::RightEdge || m_location == Plasma::Types::TopEdge) {
        //! TOPLEFT corner
        //! first LEFT pixel found
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(discovRow));

        for (int col = 0; col < corner.width() - 1; ++col) {
            QRgb pixelData = line[col];

            if (qAlpha(pixelData) < minOpacity) {
                round++;
            } else {
                break;
            }
        }
    } else if (m_location == Plasma::Types::LeftEdge) {
        //! it should be TOPRIGHT corner in that case
        //! first RIGHT pixel found
        QRgb *line = reinterpret_cast<QRgb *>(corner.scanLine(discovRow));

        for (int col = corner.width() - 1; col > 0; --col) {
            QRgb pixelData = line[col];

            if (qAlpha(pixelData) < minOpacity) {
                round++;
            } else {
                break;
            }
        }
    }

    m_roundness = round;
    Q_EMIT roundnessChanged();
}

void
PanelBackground::updateShadow(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    if (!m_parentTheme->hasShadow()) {
        m_shadowSize = 0;
        m_shadowColor = Qt::black;
        return;
    }

    bool horizontal = (m_location == Plasma::Types::BottomEdge || m_location == Plasma::Types::TopEdge);

    QString borderId{ QStringLiteral("shadow-top") };

    if (m_location == Plasma::Types::TopEdge) {
        borderId = QStringLiteral("shadow-bottom");
    } else if (m_location == Plasma::Types::LeftEdge) {
        borderId = QStringLiteral("shadow-right");
    } else if (m_location == Plasma::Types::RightEdge) {
        borderId = QStringLiteral("shadow-left");
    }

    QImage border = svg->image(svg->elementSize(borderId).toSize(), borderId);

    if (border.format() != QImage::Format_ARGB32_Premultiplied) {
        border.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    //! find shadow size through, plasma theme
    int themeshadowsize{ 0 };

    if (m_location == Plasma::Types::TopEdge) {
        themeshadowsize = svg->elementSize(element(svg, QStringLiteral("shadow-hint-bottom-margin"))).height();
    } else if (m_location == Plasma::Types::LeftEdge) {
        themeshadowsize = svg->elementSize(element(svg, QStringLiteral("shadow-hint-right-margin"))).width();
    } else if (m_location == Plasma::Types::RightEdge) {
        themeshadowsize = svg->elementSize(element(svg, QStringLiteral("shadow-hint-left-margin"))).width();
    } else {
        themeshadowsize = svg->elementSize(element(svg, QStringLiteral("shadow-hint-top-margin"))).height();
    }

    //! find shadow size through heuristics, elementsize provided through svg may not be valid because it could contain
    //! many fully transparent pixels in its edges
    int discoveredshadowsize{ 0 };
    int firstPixel{ -1 };
    int lastPixel{ -1 };

    if (horizontal) {
        for (int y = 0; y < border.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(border.scanLine(y));
            QRgb pixel = line[0];

            if (qAlpha(pixel) > 0) {
                if (firstPixel < 0) {
                    firstPixel = y;
                    lastPixel = y;
                } else {
                    lastPixel = y;
                }
            }
        }
    } else {
        QRgb *line = reinterpret_cast<QRgb *>(border.scanLine(0));

        for (int x = 0; x < border.width(); ++x) {
            QRgb pixel = line[x];

            if (qAlpha(pixel) > 0) {
                if (firstPixel < 0) {
                    firstPixel = x;
                    lastPixel = x;
                } else {
                    lastPixel = x;
                }
            }
        }
    }

    discoveredshadowsize = (firstPixel >= 0 ? qMax(0, lastPixel - firstPixel + 1) : 0);

    m_shadowSize = qMax(themeshadowsize, discoveredshadowsize);

    //! find maximum shadow color applied
    int maxopacity{ 0 };

    for (int r = 0; r < border.height(); ++r) {
        QRgb *line = reinterpret_cast<QRgb *>(border.scanLine(r));

        for (int c = 0; c < border.width(); ++c) {
            QRgb pixel = line[c];

            if (qAlpha(pixel) > maxopacity) {
                maxopacity = qAlpha(pixel);
                m_shadowColor = QColor(pixel);
                m_shadowColor.setAlpha(qMin(255, maxopacity));
            }
        }
    }
}

void
PanelBackground::updateRoundness(KSvg::Svg *svg)
{
    if (!svg) {
        return;
    }

    if (hasMask(svg)) {
        qCDebug(lattePlasma) << "PLASMA THEME, calculating roundness from mask...";
        updateRoundnessFromMask(svg);
    } else if (m_parentTheme->hasShadow()) {
        qCDebug(lattePlasma) << "PLASMA THEME, calculating roundness from shadows...";
        updateRoundnessFromShadows(svg);
    } else {
        qCDebug(lattePlasma) << "PLASMA THEME, calculating roundness from fallback code...";
        updateRoundnessFallback(svg);
    }
}

void
PanelBackground::update()
{
    KSvg::Svg *backSvg = new KSvg::Svg(this);
    backSvg->setImagePath(QStringLiteral("widgets/panel-background"));
    backSvg->resize();

    updateMaxOpacity(backSvg);
    updatePaddings(backSvg);
    updateRoundness(backSvg);
    updateShadow(backSvg);

    qCDebug(lattePlasma) << " PLASMA THEME EXTENDED :: " << m_location << " | roundness:" << m_roundness << " center_max_opacity:" << m_maxOpacity;
    qCDebug(lattePlasma) << " PLASMA THEME EXTENDED :: " << m_location << " | padtop:" << m_paddingTop << " padleft:" << m_paddingLeft << " padbottom:" << m_paddingBottom
                         << " padright:" << m_paddingRight;
    qCDebug(lattePlasma) << " PLASMA THEME EXTENDED :: " << m_location << " | shadowsize:" << m_shadowSize << " shadowcolor:" << m_shadowColor;

    backSvg->deleteLater();
}

#if defined(Q_CC_CLANG)
QT_WARNING_POP
#endif

}
}
