#include "subjectextractor.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <algorithm>

QString SubjectExtractor::defaultOutputPath(const QString &inputPath, const QString &suffix)
{
    if (inputPath.trimmed().isEmpty()) {
        return QString();
    }
    const QFileInfo fi(inputPath);
    const QString stem = fi.completeBaseName().isEmpty() ? fi.fileName() : fi.completeBaseName();
    const QString fileName = stem + suffix + QStringLiteral(".png");
    if (inputPath.contains(QLatin1Char('/')) || inputPath.contains(QLatin1Char('\\'))) {
        return fi.dir().filePath(fileName);
    }
    return fileName;
}

QRect SubjectExtractor::alphaBoundingBox(const QImage &image, int alphaThreshold)
{
    if (image.isNull()) {
        return QRect();
    }

    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    const int width = argb.width();
    const int height = argb.height();
    const int threshold = std::clamp(alphaThreshold, 1, 255);

    int minX = width;
    int minY = height;
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < height; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            if (qAlpha(line[x]) >= threshold) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return QRect();
    }

    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

QImage SubjectExtractor::cropToSubject(const QImage &image, int padding, int alphaThreshold)
{
    if (image.isNull()) {
        return QImage();
    }

    const QRect bbox = alphaBoundingBox(image, alphaThreshold);
    if (!bbox.isValid() || bbox.isEmpty()) {
        return image.convertToFormat(QImage::Format_ARGB32);
    }

    const int pad = std::max(0, padding);
    const int left = std::max(0, bbox.left() - pad);
    const int top = std::max(0, bbox.top() - pad);
    const int right = std::min(image.width() - 1, bbox.right() + pad);
    const int bottom = std::min(image.height() - 1, bbox.bottom() + pad);

    const QRect expanded(left, top, right - left + 1, bottom - top + 1);
    return image.convertToFormat(QImage::Format_ARGB32).copy(expanded);
}

QImage SubjectExtractor::addStickerOutline(const QImage &image, int outlineRadius, const QColor &outlineColor)
{
    if (image.isNull()) {
        return QImage();
    }

    const QImage src = image.convertToFormat(QImage::Format_ARGB32);
    if (outlineRadius <= 0) {
        return src;
    }

    const int width = src.width();
    const int height = src.height();
    const int r = outlineRadius;
    const int rSq = r * r;
    constexpr int alphaThreshold = 8;

    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    const QRgb outlineRgb = qRgba(outlineColor.red(), outlineColor.green(), outlineColor.blue(), outlineColor.alpha());

    for (int y = 0; y < height; ++y) {
        QRgb *dstLine = reinterpret_cast<QRgb *>(result.scanLine(y));
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));

        for (int x = 0; x < width; ++x) {
            const QRgb srcPixel = srcLine[x];
            const int srcA = qAlpha(srcPixel);

            if (srcA == 255) {
                dstLine[x] = srcPixel;
                continue;
            }

            bool inOutline = (srcA >= alphaThreshold);
            if (!inOutline) {
                const int yStart = std::max(0, y - r);
                const int yEnd = std::min(height - 1, y + r);
                const int xStart = std::max(0, x - r);
                const int xEnd = std::min(width - 1, x + r);

                for (int ny = yStart; ny <= yEnd && !inOutline; ++ny) {
                    const int dy = ny - y;
                    const int dySq = dy * dy;
                    const QRgb *nLine = reinterpret_cast<const QRgb *>(src.constScanLine(ny));
                    for (int nx = xStart; nx <= xEnd; ++nx) {
                        const int dx = nx - x;
                        if (dx * dx + dySq <= rSq && qAlpha(nLine[nx]) >= alphaThreshold) {
                            inOutline = true;
                            break;
                        }
                    }
                }
            }

            if (!inOutline) {
                dstLine[x] = qRgba(0, 0, 0, 0);
            } else if (srcA == 0) {
                dstLine[x] = outlineRgb;
            } else {
                const int invA = 255 - srcA;
                const int outR = (qRed(srcPixel) * srcA + qRed(outlineRgb) * invA) / 255;
                const int outG = (qGreen(srcPixel) * srcA + qGreen(outlineRgb) * invA) / 255;
                const int outB = (qBlue(srcPixel) * srcA + qBlue(outlineRgb) * invA) / 255;
                dstLine[x] = qRgba(outR, outG, outB, 255);
            }
        }
    }

    return result;
}

QString SubjectExtractor::resolveRembgExecutable()
{
    QString bin = QStandardPaths::findExecutable(QStringLiteral("rembg"));
    if (!bin.isEmpty()) {
        return bin;
    }
    const QString sysBin = QStringLiteral("/usr/bin/rembg");
    if (QFileInfo::exists(sysBin) && QFileInfo(sysBin).isExecutable()) {
        return sysBin;
    }
    const QString localRembg = QDir::homePath() + QStringLiteral("/.local/bin/rembg");
    if (QFileInfo::exists(localRembg) && QFileInfo(localRembg).isExecutable()) {
        return localRembg;
    }
    return QString();
}

void SubjectExtractor::setRembgExecutable(const QString &executablePath)
{
    m_rembgExecutable = executablePath;
}

QString SubjectExtractor::rembgExecutable() const
{
    return m_rembgExecutable.isEmpty() ? resolveRembgExecutable() : m_rembgExecutable;
}

void SubjectExtractor::setWlCopyExecutable(const QString &executablePath)
{
    m_wlCopyExecutable = executablePath;
}

QString SubjectExtractor::wlCopyExecutable() const
{
    if (!m_wlCopyExecutable.isEmpty()) {
        return m_wlCopyExecutable;
    }
    return QStandardPaths::findExecutable(QStringLiteral("wl-copy"));
}

bool SubjectExtractor::extractSubject(const QString &inputPath, const QString &outputPath, QString *errorOut)
{
    if (inputPath.trimmed().isEmpty() || outputPath.trimmed().isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Ruta de entrada o salida vacía.");
        }
        return false;
    }

    const QFileInfo inFi(inputPath);
    if (!inFi.exists() || !inFi.isFile()) {
        if (errorOut) {
            *errorOut = QStringLiteral("El archivo de entrada no existe: %1").arg(inputPath);
        }
        return false;
    }

    const QString rembgBin = rembgExecutable();
    if (rembgBin.isEmpty() || !QFileInfo::exists(rembgBin)) {
        if (errorOut) {
            *errorOut = QStringLiteral("No se encontró el ejecutable 'rembg' en ~/.local/bin/rembg ni en PATH.");
        }
        return false;
    }

    QProcess proc;
    proc.start(rembgBin, {QStringLiteral("i"), inputPath, outputPath});
    if (!proc.waitForStarted(10000)) {
        if (errorOut) {
            *errorOut = QStringLiteral("No se pudo iniciar rembg: %1").arg(proc.errorString());
        }
        return false;
    }

    if (!proc.waitForFinished(180000)) {
        proc.kill();
        proc.waitForFinished(2000);
        if (errorOut) {
            *errorOut = QStringLiteral("Tiempo de espera agotado al ejecutar rembg.");
        }
        return false;
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        const QString stdErr = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        if (errorOut) {
            *errorOut = stdErr.isEmpty()
                ? QStringLiteral("rembg finalizó con código de error %1.").arg(proc.exitCode())
                : QStringLiteral("Error en rembg: %1").arg(stdErr);
        }
        return false;
    }

    const QFileInfo outFi(outputPath);
    if (!outFi.exists() || outFi.size() <= 0) {
        if (errorOut) {
            *errorOut = QStringLiteral("rembg no generó el archivo de salida esperado: %1").arg(outputPath);
        }
        return false;
    }

    return true;
}

bool SubjectExtractor::copyImageToWaylandClipboard(const QString &pngPath, QString *errorOut)
{
    QFile file(pngPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QStringLiteral("No se pudo abrir la imagen para copiar al portapapeles: %1").arg(pngPath);
        }
        return false;
    }

    const QByteArray pngBytes = file.readAll();
    file.close();

    if (pngBytes.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("El archivo PNG está vacío: %1").arg(pngPath);
        }
        return false;
    }

    const QString wlCopyBin = wlCopyExecutable();
    if (wlCopyBin.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("No se encontró 'wl-copy' en el sistema.");
        }
        return false;
    }

    QProcess proc;
    proc.start(wlCopyBin, {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!proc.waitForStarted(5000)) {
        if (errorOut) {
            *errorOut = QStringLiteral("No se pudo iniciar wl-copy: %1").arg(proc.errorString());
        }
        return false;
    }

    proc.write(pngBytes);
    proc.closeWriteChannel();

    if (!proc.waitForFinished(10000)) {
        proc.kill();
        proc.waitForFinished(1000);
        if (errorOut) {
            *errorOut = QStringLiteral("Tiempo de espera agotado al copiar al portapapeles con wl-copy.");
        }
        return false;
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        const QString stdErr = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        if (errorOut) {
            *errorOut = stdErr.isEmpty()
                ? QStringLiteral("wl-copy falló con código %1.").arg(proc.exitCode())
                : QStringLiteral("Error en wl-copy: %1").arg(stdErr);
        }
        return false;
    }

    return true;
}
