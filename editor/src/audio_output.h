#pragma once
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QIODevice>
#include <QGuiApplication>
#include <memory>
namespace cf {
class AudioOutput {
    std::unique_ptr<QAudioSink> sink;
    QIODevice *device=nullptr;
public:
    void write(const QByteArray &pcm){
        if(!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))return;
        if(!sink){auto output=QMediaDevices::defaultAudioOutput();QAudioFormat format;format.setSampleRate(48000);format.setChannelCount(2);format.setSampleFormat(QAudioFormat::Int16);if(output.isNull()||!output.isFormatSupported(format))return;sink=std::make_unique<QAudioSink>(output,format);sink->setBufferSize(16384);device=sink->start();}
        if(device&&sink->bytesFree()>=pcm.size())device->write(pcm);
    }
};
}
