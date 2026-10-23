#pragma once
#include <QJSEngine>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <stdexcept>
#include <functional>

class Emulator {
    QJSEngine engine;
    QString savePath, previousSave;
public:
    std::function<void(const QString&,const QString&)> buttonSound;
    QJSValue evaluate(const QString& code, const QString& name = QStringLiteral("host")) {
        auto result = engine.evaluate(code, name);
        if (result.isError()) throw std::runtime_error((name + ": " + result.toString() + "\n" + result.property("stack").toString()).toStdString());
        return result;
    }
    static QByteArray read(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(("Cannot read " + path).toStdString());
        if (file.size() > 4 * 1024 * 1024) throw std::runtime_error("Emulator input is too large");
        return file.readAll();
    }
    Emulator(const QString& base, const QString& data): savePath(data + "/save.json") {
        QDir().mkpath(data);
        QByteArray saved = "{}";
        if (QFile::exists(savePath)) {
            saved = read(savePath);
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(saved, &error);
            if (error.error || !doc.isObject() || doc.object().value("schema").toInt() != 1
                || !doc.object().value("storage").isObject() || !doc.object().value("cookies").isObject())
                throw std::runtime_error("Invalid save.json; the existing save has been preserved");
        }
        evaluate("var __saved = " + QString::fromUtf8(saved) + ";");
        evaluate("var __animations = " + QString::fromUtf8(read(base + "/emulator/anims.json")) + ";");
        // One script scope lets the display bridge observe upstream's lexical
        // state without rewriting its let/const declarations.
        evaluate(QString::fromUtf8(read(base + "/runtime/host.js")) + "\n" + QString::fromUtf8(read(base + "/runtime/alarm.js")) + "\n" + QString::fromUtf8(read(base + "/runtime/device.js")) + "\n" +
                 QString::fromUtf8(read(base + "/emulator/script.js")) + "\n__installPixelRenderer();\n__start();", "pocket-pikachu.js");
    }
    void tick(double ms) {
        call("__advance", {QJSValue(ms)});
        if(call("__takeAlarmSound").toBool()){
            save(); // Persist today's ring before playing, preventing restart repeats.
            if(buttonSound&&call("__soundEnabled").toBool())buttonSound("alarm","ringing");
        }
    }
    void press(const QString& name) {
        QString before=buttonSound&&name!="shake"?snapshot().property("status").toString():QString();
        call("__press", {QJSValue(name)});
        if(buttonSound&&name!="shake"&&call("__soundEnabled").toBool())buttonSound(name,before);
    }
    QJSValue call(const QString& name, const QJSValueList& args = {}) {
        auto value = engine.globalObject().property(name).call(args);
        if (value.isError()) throw std::runtime_error((value.toString() + "\n" + value.property("stack").toString()).toStdString());
        return value;
    }
    QJSValue snapshot() { return call("__snapshot"); }
    void save() {
        QString value = call("__save").toString();
        if (value == previousSave) return;
        QSaveFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Cannot create emulator save");
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        auto bytes = value.toUtf8();
        if (file.write(bytes) != bytes.size() || !file.commit()) throw std::runtime_error("Cannot save emulator progress");
        previousSave = value;
    }
};
