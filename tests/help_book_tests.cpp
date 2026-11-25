#include "help_book.h"
#include <QtTest>
#include <QtWidgets>
#include <QTemporaryDir>
class HelpBookTests:public QObject {
    Q_OBJECT
    static void write(const QString &path,const QByteArray &bytes){QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(bytes),bytes.size());}
private slots:
    void editionMustMatchExecutable(){QTemporaryDir root;QVERIFY(QDir(root.path()).mkpath("docs/versions/0.9.0"));write(root.filePath("app-version.json"),"{}");write(root.filePath("docs/versions/0.9.0/book.json"),"{\"version\":\"0.9.0\"}");auto previous=QCoreApplication::applicationVersion();QCoreApplication::setApplicationVersion("0.9.0");QCOMPARE(cf::documentationRoot(root.path()),root.filePath("docs/versions/0.9.0"));QCoreApplication::setApplicationVersion("0.9.1");QVERIFY(cf::documentationRoot(root.path()).isEmpty());QCoreApplication::setApplicationVersion(previous);}

    void searchesBodiesAndNavigatesChapters(){
        QTemporaryDir root;QVERIFY(QDir(root.path()).mkdir("docs"));
        write(root.filePath("docs/handbook.md"),"# Welcome\nUse this book offline.\n## Pause menus\nOpen a pause menu using `menu pause_menu`, then return.\n[Animation lists](editor-authoring.md#animation-lists)\n## Input\nUse on_press start to open the pause menu.\n");
        write(root.filePath("docs/editor-authoring.md"),"# Reference\nControls.\n## Animation lists\nExport clips without image pixels.\n### Matching sheets\nUse the same frame dimensions.\n");
        cf::HelpBook book(root.path());auto *query=book.findChild<QLineEdit *>("helpSearch");auto *topics=book.findChild<QListWidget *>("helpTopics");auto *reader=book.findChild<QTextBrowser *>("helpReader");QVERIFY(query&&topics&&reader);const int all=topics->count();QCOMPARE(all,5);
        query->setText("PAUSE menu");QCOMPARE(topics->count(),2);QVERIFY(reader->toPlainText().contains("pause menu"));QVERIFY(!reader->extraSelections().isEmpty());
        query->setText("on_press");QCOMPARE(topics->count(),1);QVERIFY(reader->toPlainText().contains("on_press start"));
        query->setText("not_a_documentation_term");QCOMPARE(topics->count(),0);QVERIFY(reader->toPlainText().contains("No matching topics"));query->clear();QCOMPARE(topics->count(),all);
        for(int i=0;i<topics->count();++i)if(topics->item(i)->text().contains("Pause menus"))topics->setCurrentRow(i);
        reader->anchorClicked(QUrl("editor-authoring.md#matching-sheets"));QVERIFY(reader->toPlainText().contains("same frame dimensions"));QVERIFY(topics->currentItem()->text().contains("Animation lists"));
        QPushButton *back=nullptr,*forward=nullptr;for(auto *button:book.findChildren<QPushButton *>()){if(button->text().contains("Back"))back=button;if(button->text().contains("Forward"))forward=button;}QVERIFY(back&&forward);QVERIFY(back->isEnabled());back->click();QVERIFY(reader->toPlainText().contains("menu pause_menu"));forward->click();QVERIFY(reader->toPlainText().contains("without image pixels"));auto before=reader->toPlainText();reader->anchorClicked(QUrl("missing.md"));QCOMPARE(reader->toPlainText(),before);
    }
    void bundledBookContainsWorkedExamples(){cf::HelpBook book(QString(CF_HELP_BOOK_DIR));auto *query=book.findChild<QLineEdit *>("helpSearch");auto *topics=book.findChild<QListWidget *>("helpTopics");auto *reader=book.findChild<QTextBrowser *>("helpReader");QVERIFY(topics->count()>30);query->setText("pause menu");QVERIFY(topics->count()>0);query->setText("on_press start");QVERIFY(topics->count()>0);query->setText(".cfanimations");QVERIFY(topics->count()>0);QVERIFY(reader->toPlainText().contains(".cfanimations"));}
};
QTEST_MAIN(HelpBookTests)
#include "help_book_tests.moc"
