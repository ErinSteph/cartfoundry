#pragma once
#include <QDialog>
#include <QStringList>
#include <QVector>
class QListWidget;class QLineEdit;class QTextBrowser;class QLabel;class QPushButton;
namespace cf {
QString documentationRoot(const QString &resourceRoot);
class HelpBook:public QDialog {
public:
    explicit HelpBook(const QString &resourceRoot,QWidget *parent=nullptr);
private:
    struct Topic {QString title,source,anchor,markdown;};
    QVector<Topic> topics;QVector<int> history;int historyIndex=-1,current=-1;
    QListWidget *contents;QLineEdit *search;QTextBrowser *reader;QLabel *status;QPushButton *back,*forward;
    void load(const QString &,const QString &);
    void filter();void showTopic(int,bool remember=true);void highlight();void followLink(const QUrl &);
};
}
