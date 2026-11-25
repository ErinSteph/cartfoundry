#include <QCoreApplication>
#include <QTimer>
#include <QFileInfo>
#include "update_model.h"
int main(int argc,char **argv){QCoreApplication app(argc,argv);auto args=app.arguments();if(args.contains("--hold")){QTimer::singleShot(1200,&app,&QCoreApplication::quit);return app.exec();}

#ifdef Q_OS_MACOS
 if(QFileInfo::exists(QCoreApplication::applicationDirPath()+"/../Resources/fail-startup"))return 1;
#else
 if(QFileInfo::exists(QCoreApplication::applicationDirPath()+"/resources/fail-startup"))return 1;
#endif
 if(args.contains("--update-job")){auto path=args.value(args.indexOf("--update-job")+1);auto plan=cf::updates::readJson(path);QString error;cf::updates::writeJson(QFileInfo(path).absolutePath()+"/confirmed.json",{{"version",plan["version"]},{"target",plan["target"]}},error);}return 0;}
