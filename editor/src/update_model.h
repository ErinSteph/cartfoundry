#pragma once
#include <QJsonObject>
#include <QSettings>
#include <QUrl>
#include <array>
#include <optional>
namespace cf::updates {
inline constexpr auto FeedUrl="https://raw.githubusercontent.com/ErinSteph/cartfoundry/main/updates/stable.json";
struct Version {
 std::array<quint32,3> parts{};
 static std::optional<Version> parse(const QString &);
 QString text()const;
 bool operator<(const Version &other)const{return parts<other.parts;}
 bool operator==(const Version &other)const{return parts==other.parts;}
};
struct Release {Version version;QString notes,platform;QUrl url;QByteArray sha256;qint64 size=0;};
QString platformKey();
QString packageName();
bool trustedPackageUrl(const QUrl &);
// A valid manifest may have no package for this platform. That is not an error.
bool parseManifest(const QByteArray &,const QString &platform,Release &,bool &available,QString &error);
void observeInstalledVersion(QSettings &,const Version &);
bool verifyPackage(const QString &path,const Release &,QString &error);
QString metadataPath(const QString &root);
QString executablePath(const QString &root);
bool validateInstallation(const QString &root,const Version &,QString &error,bool signature=true);
bool inspectArchive(const QString &zip,const QString &root,QString &error);
bool copyTree(const QString &from,const QString &to,QString &error);
bool runTool(const QString &,const QStringList &,QString &,int timeout=120000);
bool writeJson(const QString &,const QJsonObject &,QString &error);
QJsonObject readJson(const QString &);
struct InstallPlan {QString target,staged,job;Version version;qint64 parentPid=0;};
bool parsePlan(const QString &file,InstallPlan &,QString &error);
bool replaceInstallation(const InstallPlan &,QString &backup,QString &error,bool signature=true);
bool rollbackInstallation(const InstallPlan &,const QString &backup,QString &error);
bool waitForExit(qint64 pid,QString &error,int timeoutMs=60000);
bool installationIsRunning(const QString &root);
}
