#include "emulator_core.h"
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <QFile>
#include <vector>
namespace cf {
struct EmulatorCore::Impl {
    QByteArray audio; mCore *core=nullptr;std::vector<color_t> pixels=std::vector<color_t>(256*256,0);bool config=false;
    ~Impl(){if(core){if(config)mCoreConfigDeinit(&core->config);core->deinit(core);}}
};
EmulatorCore::EmulatorCore():impl(std::make_unique<Impl>()){}
EmulatorCore::~EmulatorCore()=default;
bool EmulatorCore::loaded()const{return impl->core;}
bool EmulatorCore::load(const QString &path,QString &error){
    auto bytes=QFile::encodeName(path);auto *core=mCoreFind(bytes.constData());if(!core){error="mGBA could not recognise this ROM.";return false;}
    auto next=std::make_unique<Impl>();next->core=core;
    if(!core->init(core)){error="Could not initialise mGBA.";return false;}
    mCoreInitConfig(core,nullptr);next->config=true;
    mCoreConfigSetDefaultIntValue(&core->config,"useBios",0);
    mCoreConfigSetDefaultIntValue(&core->config,"logLevel",0);
    mCoreConfigSetDefaultIntValue(&core->config,"volume",256);
    mCoreConfigSetDefaultIntValue(&core->config,"mute",0);
    mCoreLoadConfig(core);core->setVideoBuffer(core,next->pixels.data(),256);
    if(!mCoreLoadFile(core,bytes.constData())){error="Could not load ROM into mGBA.";return false;}
    core->setAudioBufferSize(core,4096);core->reset(core);
    for(int i=0;i<2;++i)blip_set_rates(core->getAudioChannel(core,i),core->frequency(core),48000);
    impl=std::move(next);return true;
}
void EmulatorCore::reset(){if(impl->core)impl->core->reset(impl->core);}
void EmulatorCore::frame(quint32 keys){
    auto *c=impl->core;if(!c)return;c->setKeys(c,keys);c->runFrame(c);
    short left[4096],right[4096];auto *l=c->getAudioChannel(c,0),*r=c->getAudioChannel(c,1);
    int count=qMin(4096,qMin(blip_samples_avail(l),blip_samples_avail(r)));
    blip_read_samples(l,left,count,0);blip_read_samples(r,right,count,0);
    impl->audio.resize(count*4);auto *out=reinterpret_cast<short *>(impl->audio.data());for(int i=0;i<count;++i){out[i*2]=left[i];out[i*2+1]=right[i];}
}
QByteArray EmulatorCore::takeAudio(){auto pcm=impl->audio;impl->audio.clear();return pcm;}
QImage EmulatorCore::image()const{if(!impl->core)return {};unsigned w,h;impl->core->desiredVideoDimensions(impl->core,&w,&h);if(w>256||h>256)return {};return QImage(reinterpret_cast<const uchar *>(impl->pixels.data()),w,h,256*sizeof(color_t),QImage::Format_RGBX8888).copy();}
quint8 EmulatorCore::readByte(quint32 address)const{return impl->core?impl->core->busRead8(impl->core,address):0;}
}
