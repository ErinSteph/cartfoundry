#pragma once
#include <QImage>
#include <QString>
#include <memory>
namespace cf {
class EmulatorCore {
public:
    EmulatorCore();~EmulatorCore();
    bool load(const QString &,QString &error);
    void reset();void frame(quint32 keys=0);
    QImage image() const;
    quint8 readByte(quint32 address) const;
    bool loaded() const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
}
