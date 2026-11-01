#include "subjectextractor.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

namespace {

void sendDesktopNotification(const QString &summary, const QString &body, bool enabled = true)
{
    if (!enabled) {
        return;
    }
    const QString notifyBin = QStandardPaths::findExecutable(QStringLiteral("notify-send"));
    if (notifyBin.isEmpty()) {
        return;
    }
    QProcess::startDetached(notifyBin, {
        QStringLiteral("--app-name=KLiftSubject"),
        QStringLiteral("--icon=image-png"),
        summary,
        body
    });
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kliftsubject"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Extrae el sujeto principal y elimina el fondo de imágenes con IA (estilo macOS Lift Subject) para KDE Plasma 6 + Wayland."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption removeBgOpt(
        QStringLiteral("remove-bg"),
        QStringLiteral("Elimina el fondo y guarda <archivo>-nobg.png junto al original."));
    QCommandLineOption copySubjectOpt(
        QStringLiteral("copy-subject"),
        QStringLiteral("Extrae el sujeto, recorta márgenes transparentes y lo copia al portapapeles Wayland."));
    QCommandLineOption stickerOpt(
        QStringLiteral("sticker"),
        QStringLiteral("Extrae el sujeto, recorta márgenes, añade borde blanco de sticker (<archivo>-sticker.png) y lo copia al portapapeles."));
    QCommandLineOption noNotifyOpt(
        QStringLiteral("no-notify"),
        QStringLiteral("Desactiva las notificaciones de escritorio (notify-send)."));

    parser.addOption(removeBgOpt);
    parser.addOption(copySubjectOpt);
    parser.addOption(stickerOpt);
    parser.addOption(noNotifyOpt);
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Imágenes de entrada (PNG, JPEG, WebP)."), QStringLiteral("<file1> [file2...]"));

    parser.process(app);

    const QStringList files = parser.positionalArguments();
    const bool notify = !parser.isSet(noNotifyOpt);

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (files.isEmpty()) {
        err << "Error: Debe indicar al menos un archivo de imagen.\n";
        parser.showHelp(1);
    }

    const bool doCopy = parser.isSet(copySubjectOpt);
    const bool doSticker = parser.isSet(stickerOpt);
    const bool doRemoveBg = parser.isSet(removeBgOpt) || (!doCopy && !doSticker);

    SubjectExtractor extractor;
    sendDesktopNotification(
        QStringLiteral("KLiftSubject"),
        QStringLiteral("Extrayendo sujeto con IA..."),
        notify);

    int exitCode = 0;

    for (const QString &inputFile : files) {
        const QString suffix = doSticker ? QStringLiteral("-sticker") : QStringLiteral("-nobg");
        const QString outputPath = SubjectExtractor::defaultOutputPath(inputFile, suffix);

        QString errorMsg;
        if (!extractor.extractSubject(inputFile, outputPath, &errorMsg)) {
            err << "Error procesando " << inputFile << ": " << errorMsg << "\n";
            sendDesktopNotification(QStringLiteral("Error en KLiftSubject"), errorMsg, notify);
            exitCode = 1;
            continue;
        }

        if (doCopy || doSticker) {
            QImage img(outputPath);
            if (img.isNull()) {
                const QString loadErr = QStringLiteral("No se pudo cargar la imagen generada: %1").arg(outputPath);
                err << loadErr << "\n";
                sendDesktopNotification(QStringLiteral("Error en KLiftSubject"), loadErr, notify);
                exitCode = 1;
                continue;
            }

            if (doSticker) {
                img = SubjectExtractor::cropToSubject(img, 14, 8);
                img = SubjectExtractor::addStickerOutline(img, 6, Qt::white);
                img = SubjectExtractor::cropToSubject(img, 6, 8);
            } else {
                img = SubjectExtractor::cropToSubject(img, 8, 8);
            }

            if (!img.save(outputPath, "PNG")) {
                const QString saveErr = QStringLiteral("No se pudo guardar la imagen recortada: %1").arg(outputPath);
                err << saveErr << "\n";
                sendDesktopNotification(QStringLiteral("Error en KLiftSubject"), saveErr, notify);
                exitCode = 1;
                continue;
            }

            QString clipErr;
            if (!extractor.copyImageToWaylandClipboard(outputPath, &clipErr)) {
                err << "Aviso: imagen guardada en " << outputPath << " pero falló wl-copy: " << clipErr << "\n";
            } else {
                sendDesktopNotification(
                    QStringLiteral("KLiftSubject"),
                    QStringLiteral("Sujeto copiado al portapapeles (%1)").arg(QFileInfo(outputPath).fileName()),
                    notify);
            }
        } else if (doRemoveBg) {
            sendDesktopNotification(
                QStringLiteral("KLiftSubject"),
                QStringLiteral("Fondo eliminado: %1").arg(outputPath),
                notify);
        }

        out << outputPath << "\n";
    }

    return exitCode;
}
