#include "window.h"
#include "builder.h"
#include "startup_splash.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QScreen>
#include <QSplashScreen>
#include <QTimer>
int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv); app.setApplicationName("CartFoundry"); app.setOrganizationName("CartFoundry");
    app.setStyle("Fusion");
    app.setStyleSheet(R"(
        QWidget { background: #19232f; color: #e6eaf0; font-size: 13px; }
        QMainWindow, QMenuBar, QMenu { background: #19232f; }
        #topbar { background: #202c3a; border-bottom: 1px solid #3a4757; }
        #topbar QLabel { background: transparent; }
        #caption { color: #a9b7c7; font-size: 10px; font-weight: bold; letter-spacing: 1px; }
        #hint { color: #95a5b8; font-size: 11px; }
        QPushButton { background: #2b394a; border: 1px solid #46576b; border-radius: 5px; padding: 7px 11px; }
        QPushButton:hover { background: #3b4b60; border-color: #a4b4c7; }
        QPushButton:pressed { background: #17202b; }
        QPushButton:disabled { color: #6d7d90; border-color: #303e50; }
        #primary { background: #ee9563; color: #19232f; border-color: #f4a87e; font-weight: bold; }
        QLineEdit, QPlainTextEdit, QComboBox { background: #111923; border: 1px solid #415066; border-radius: 4px; padding: 6px; selection-background-color: #7f513b; }
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus { border-color: #ed9b70; }
        QComboBox QAbstractItemView { background: #202c3a; selection-background-color: #78503d; }
        QListWidget { border: none; background: #19232f; outline: none; }
        QListWidget::item { padding: 10px 1px; border-radius: 5px; }
        QListWidget::item:selected { background: #38414a; color: #ffc59d; }
        QScrollArea { border: none; }
        QSplitter::handle { background: #354152; width: 1px; }
        #validation { background: #342d2a; color: #f7bc93; font-size: 12px; }
        #validation[valid="true"] { background: #203934; color: #a5d7c8; }
        #screen { background: #e7e7cd; color: #213344; border: 7px solid #44515c; border-radius: 5px; padding: 7px; }
        QStatusBar { color: #a9b7c7; }
        QToolTip { color: #fff; background: #39495b; border: 1px solid #7c8a9b; }
    )");
    // Start-up splash: the Emerald Softlab logo on the editor's dark blue for about two seconds.
    // It is skipped silently if images/em_2_glow.png is missing.
    constexpr int splashMilliseconds = 2000;
    QElapsedTimer splashClock; QSplashScreen *splash = nullptr;
    const qreal dpr = app.primaryScreen() ? app.primaryScreen()->devicePixelRatio() : 1.0;
    const QPixmap splashArt = cf::startupSplashPixmap(cf::resourceRoot() + "/images/em_2_glow.png", QSize(600, 580), dpr, QSize(520, 500));
    if (!splashArt.isNull()) { splash = new QSplashScreen(splashArt, Qt::WindowStaysOnTopHint); splash->show(); app.processEvents(); splashClock.start(); }
    cf::Window window; if (argc > 1 && QFileInfo::exists(QString::fromLocal8Bit(argv[1]))) window.openProject(QString::fromLocal8Bit(argv[1]));
    if (!splash) { window.show(); return app.exec(); }
    // The window finished loading while the splash was up; wait out whatever is left of the two seconds.
    const int remaining = qMax(0, splashMilliseconds - int(splashClock.elapsed()));
    QTimer::singleShot(remaining, &app, [&window, splash] { window.show(); splash->finish(&window); delete splash; });
    return app.exec();
}
