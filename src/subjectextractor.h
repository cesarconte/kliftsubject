#pragma once

#include <QColor>
#include <QImage>
#include <QRect>
#include <QString>

class SubjectExtractor
{
public:
    SubjectExtractor() = default;

    static QString defaultOutputPath(const QString &inputPath, const QString &suffix = QStringLiteral("-nobg"));
    static QRect alphaBoundingBox(const QImage &image, int alphaThreshold = 8);
    static QImage cropToSubject(const QImage &image, int padding = 8, int alphaThreshold = 8);
    static QImage addStickerOutline(const QImage &image, int outlineRadius = 6, const QColor &outlineColor = Qt::white);

    bool extractSubject(const QString &inputPath, const QString &outputPath, QString *errorOut = nullptr);
    bool copyImageToWaylandClipboard(const QString &pngPath, QString *errorOut = nullptr);

    void setRembgExecutable(const QString &executablePath);
    QString rembgExecutable() const;

    void setWlCopyExecutable(const QString &executablePath);
    QString wlCopyExecutable() const;

    static QString resolveRembgExecutable();

private:
    QString m_rembgExecutable;
    QString m_wlCopyExecutable;
};
