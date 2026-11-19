#pragma once
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace cf {
// The editor's own background colour, so the splash blends into the window that follows it.
inline QColor startupSplashColor() { return QColor(0x19, 0x23, 0x2f); }

// Builds the start-up splash: the logo (padding and glow included) scaled to fit inside `size`,
// centred on the editor's dark blue. `dpr` is the screen's device pixel ratio so the logo stays
// sharp on Retina displays. Returns a null pixmap when the image cannot be read.
inline QPixmap startupSplashPixmap(const QString &logoPath, const QSize &size, qreal dpr, const QSize &fit = QSize()) {
    QImage logo(logoPath);
    if (logo.isNull() || size.isEmpty() || dpr <= 0) return {};
    const QSize logical = logo.size().scaled(fit.isEmpty() ? size : fit, Qt::KeepAspectRatio);
    QImage scaled = logo.scaled(logical * dpr, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    scaled.setDevicePixelRatio(dpr);
    QPixmap canvas(size * dpr);
    canvas.setDevicePixelRatio(dpr);
    canvas.fill(startupSplashColor());
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QPoint((size.width() - logical.width()) / 2, (size.height() - logical.height()) / 2), scaled);
    return canvas;
}
}
