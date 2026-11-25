#include "update_model.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QMap>
#include <QSet>
#include <QRegularExpression>
namespace cf::updates {
static quint16 u16(const QByteArray &b,int p){return quint8(b[p])|(quint16(quint8(b[p+1]))<<8);}
static quint32 u32(const QByteArray &b,int p){return u16(b,p)|(quint32(u16(b,p+2))<<16);}
// Validate both ZIP directory and local headers before invoking an OS extractor.
// Our release tool stores symlink payloads uncompressed so they can be checked
// without trusting extraction to follow them safely.
bool inspectArchive(const QString &path,const QString &root,QString &error){
 auto fail=[&](QString reason){error="Unsafe or unsupported update archive: "+reason;return false;};
 QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()<22||f.size()>1024LL*1024*1024)return fail("invalid size");
 f.seek(qMax(0LL,f.size()-65557));auto tail=f.readAll();int end=tail.lastIndexOf(QByteArray("PK\5\6",4));
 if(end<0||end+22>tail.size()||end+22+u16(tail,end+20)!=tail.size())return fail("missing ZIP directory");
 auto count=u16(tail,end+10);quint32 length=u32(tail,end+12),start=u32(tail,end+16);
 if(u16(tail,end+4)||u16(tail,end+6)||count!=u16(tail,end+8)||!count||count==65535||length>32*1024*1024||quint64(start)+length>quint64(f.size()-tail.size()+end))return fail("split/ZIP64 archive");
 f.seek(start);auto directory=f.read(length);if(directory.size()!=length)return fail("truncated directory");
 QSet<QString> names;QMap<QString,QString> links;quint64 total=0;int pos=0;
 for(int index=0;index<count;++index){
  if(pos+46>directory.size()||directory.mid(pos,4)!=QByteArray("PK\1\2",4))return fail("invalid directory entry");
  quint16 flags=u16(directory,pos+8),method=u16(directory,pos+10),nameLength=u16(directory,pos+28),extra=u16(directory,pos+30),comment=u16(directory,pos+32);
  quint32 packed=u32(directory,pos+20),size=u32(directory,pos+24),attrs=u32(directory,pos+38),offset=u32(directory,pos+42);
  if(pos+46+nameLength+extra+comment>directory.size()||(flags&1)||(method!=0&&method!=8)||u16(directory,pos+34)||quint64(size)>1024LL*1024*1024||(total+=size)>2ULL*1024*1024*1024)return fail("invalid compression or expansion size");
  auto raw=directory.mid(pos+46,nameLength);QString name=QString::fromUtf8(raw);bool folder=name.endsWith('/');if(folder)name.chop(1);
  if(name.isEmpty()||name.contains(QChar(0))||name.toUtf8()!=(folder?raw.left(raw.size()-1):raw)||name.contains('\\')||name.contains(':')||name!=QDir::cleanPath(name)||QDir::isAbsolutePath(name)||(!name.startsWith(root+"/")&&name!=root)||name.split('/').contains(".."))return fail("invalid path");
#ifdef Q_OS_WIN
  for(const auto &part:name.split('/'))if(part.endsWith(' ')||part.endsWith('.')||part.contains(QRegularExpression(R"([<>"|?*])"))||QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)",QRegularExpression::CaseInsensitiveOption).match(part).hasMatch())return fail("reserved Windows path");
#endif
  QString key=name.toCaseFolded();if(names.contains(key))return fail("duplicate path");names.insert(key);
  if(quint64(offset)+30>start)return fail("invalid local header");f.seek(offset);auto local=f.read(30);
  if(local.size()!=30||local.left(4)!=QByteArray("PK\3\4",4)||u16(local,6)!=flags||u16(local,8)!=method||u16(local,26)!=nameLength||f.read(nameLength)!=raw)return fail("inconsistent local header");
  quint64 data=quint64(offset)+30+nameLength+u16(local,28);if(data+packed>start)return fail("truncated payload");
  quint16 kind=(attrs>>16)&0170000;
  if(kind!=0&&kind!=0100000&&kind!=0040000&&kind!=0120000)return fail("special file");
  if(kind==0120000){
#ifdef Q_OS_WIN
   return fail("Windows packages cannot contain symbolic links");
#else
   if(method||size!=packed||size>4096||folder)return fail("invalid symbolic link");f.seek(data);auto targetBytes=f.read(size);QString target=QString::fromUtf8(targetBytes);
   if(target.isEmpty()||target.toUtf8()!=targetBytes||target.contains(QChar(0))||target.contains('\\')||QDir::isAbsolutePath(target)||target.contains(':')||target.split('/').contains(".."))return fail("invalid link target");
   auto resolved=QDir::cleanPath(QFileInfo(name).path()+"/"+target);if(!resolved.startsWith(root+"/"))return fail("escaping link");links[name.toCaseFolded()]=resolved;
#endif
  }
  pos+=46+nameLength+extra+comment;
 }
 if(pos!=directory.size())return fail("unexpected directory data");
 for(auto it=links.begin();it!=links.end();++it){
  // No archive entry may extract through a symlink, even one inside the bundle.
  for(const auto &name:names)if(name.startsWith(it.key().toCaseFolded()+"/"))return fail("entry below symbolic link");
  QString target=it.value();QSet<QString> visited;
  for(int step=0;step<64;++step){bool replaced=false;auto pieces=target.split('/');QString prefix;
   for(int i=0;i<pieces.size();++i){prefix+=(i?"/":"")+pieces[i];if(links.contains(prefix.toCaseFolded())){if(visited.contains(prefix.toCaseFolded()))return fail("cyclic symbolic link");visited.insert(prefix.toCaseFolded());target=QDir::cleanPath(links[prefix.toCaseFolded()]+(i+1<pieces.size()?"/"+pieces.mid(i+1).join('/'):QString()));replaced=true;break;}}
   if(!replaced){if(!names.contains(target.toCaseFolded()))return fail("dangling symbolic link");break;}
   if(step==63)return fail("excessive link depth");
  }
 }
 return true;
}
}
