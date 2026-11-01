#include "subjectextractor.h"

#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

class TestKLiftSubject : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDefaultOutputPath()
    {
        QCOMPARE(SubjectExtractor::defaultOutputPath(QStringLiteral("/home/user/Fotos/retrato.jpg")),
                 QStringLiteral("/home/user/Fotos/retrato-nobg.png"));
        QCOMPARE(SubjectExtractor::defaultOutputPath(QStringLiteral("/tmp/vacaciones.foto.webp"), QStringLiteral("-sticker")),
                 QStringLiteral("/tmp/vacaciones.foto-sticker.png"));
        QCOMPARE(SubjectExtractor::defaultOutputPath(QStringLiteral("icono.png")),
                 QStringLiteral("icono-nobg.png"));
        QVERIFY(SubjectExtractor::defaultOutputPath(QStringLiteral("   ")).isEmpty());
    }

    void testAlphaBoundingBoxAndCropToSubject()
    {
        QImage img(200, 200, QImage::Format_ARGB32);
        img.fill(Qt::transparent);

        const QRect subjectRect(80, 75, 40, 50);
        for (int y = subjectRect.top(); y <= subjectRect.bottom(); ++y) {
            for (int x = subjectRect.left(); x <= subjectRect.right(); ++x) {
                img.setPixelColor(x, y, QColor(220, 40, 60, 255));
            }
        }

        const QRect bbox = SubjectExtractor::alphaBoundingBox(img, 8);
        QCOMPARE(bbox, subjectRect);

        const int padding = 8;
        const QImage cropped = SubjectExtractor::cropToSubject(img, padding, 8);
        QCOMPARE(cropped.width(), subjectRect.width() + 2 * padding);
        QCOMPARE(cropped.height(), subjectRect.height() + 2 * padding);

        // Top-left padding corner should still be transparent
        QCOMPARE(qAlpha(cropped.pixel(0, 0)), 0);
        // Center of cropped image should be opaque subject color
        const QColor centerColor = cropped.pixelColor(cropped.width() / 2, cropped.height() / 2);
        QCOMPARE(centerColor.alpha(), 255);
        QCOMPARE(centerColor.red(), 220);
    }

    void testAddStickerOutline()
    {
        QImage img(200, 200, QImage::Format_ARGB32);
        img.fill(Qt::transparent);

        const QRect subjectRect(80, 75, 40, 50);
        for (int y = subjectRect.top(); y <= subjectRect.bottom(); ++y) {
            for (int x = subjectRect.left(); x <= subjectRect.right(); ++x) {
                img.setPixelColor(x, y, QColor(30, 140, 240, 255));
            }
        }

        const int radius = 6;
        const QImage sticker = SubjectExtractor::addStickerOutline(img, radius, Qt::white);
        QCOMPARE(sticker.size(), img.size());

        // Pixel inside subject remains original color
        const QColor insideColor = sticker.pixelColor(100, 100);
        QCOMPARE(insideColor, QColor(30, 140, 240, 255));

        // Pixel 3px outside the left border of subjectRect (x = 77, y = 100) must be opaque white outline
        const QColor outlinePixel = sticker.pixelColor(77, 100);
        QCOMPARE(outlinePixel.alpha(), 255);
        QCOMPARE(outlinePixel.red(), 255);
        QCOMPARE(outlinePixel.green(), 255);
        QCOMPARE(outlinePixel.blue(), 255);

        // Distant pixel (10, 10) must remain completely transparent
        QCOMPARE(qAlpha(sticker.pixel(10, 10)), 0);
    }

    void testEndToEndExtractSubjectWithRembg()
    {
        const QString rembgBin = SubjectExtractor::resolveRembgExecutable();
        if (rembgBin.isEmpty()) {
            QSKIP("rembg binary not found, skipping end-to-end extraction test");
        }

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString inputPath = tempDir.filePath(QStringLiteral("synthetic_subject.png"));
        const QString outputPath = SubjectExtractor::defaultOutputPath(inputPath);

        // Create a clear test image with a distinct foreground figure on a uniform background
        QImage testImg(256, 256, QImage::Format_ARGB32);
        testImg.fill(QColor(245, 245, 245, 255));
        {
            QPainter p(&testImg);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 30, 30));
            p.drawEllipse(QPoint(128, 128), 60, 60);
            p.setBrush(QColor(30, 30, 30));
            p.drawEllipse(QPoint(108, 112), 10, 10);
            p.drawEllipse(QPoint(148, 112), 10, 10);
        }
        QVERIFY(testImg.save(inputPath, "PNG"));

        SubjectExtractor extractor;
        QString err;
        const bool ok = extractor.extractSubject(inputPath, outputPath, &err);
        QVERIFY2(ok, qPrintable(err));
        QVERIFY(QFileInfo::exists(outputPath));
        QVERIFY(QFileInfo(outputPath).size() > 0);

        const QImage outImg(outputPath);
        QVERIFY(!outImg.isNull());
        QCOMPARE(outImg.size(), testImg.size());
    }
};

QTEST_MAIN(TestKLiftSubject)
#include "test_kliftsubject.moc"
