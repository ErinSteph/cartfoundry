#include "help_book.h"
#include <QtWidgets>
namespace cf {
static QString edition(const QString &root){QFile file(root+"/book.json");if(!file.open(QIODevice::ReadOnly))return {};return QJsonDocument::fromJson(file.readAll()).object()["version"].toString();}
QString documentationRoot(const QString &root){auto version=QCoreApplication::applicationVersion();auto bundled=root+"/docs/versions/"+version;if(QFileInfo::exists(root+"/app-version.json"))return edition(bundled)==version?bundled:QString();auto development=QString(CF_HELP_BOOK_DIR);return edition(development)==version?development:QString();}

static QString anchorFor(QString title){QString result;for(auto ch:title.toLower()){if(ch.isLetterOrNumber()||ch=='_'||ch=='-')result+=ch;else if(ch.isSpace())result+='-';}return result;}
HelpBook::HelpBook(const QString &root,QWidget *parent):QDialog(parent){
    auto version=edition(root);setWindowTitle(version.isEmpty()?"CartFoundry documentation book":QString("CartFoundry %1 documentation book").arg(version));resize(1080,780);setAttribute(Qt::WA_DeleteOnClose);
    auto *layout=new QVBoxLayout(this);auto *toolbar=new QHBoxLayout;
    back=new QPushButton("← Back");forward=new QPushButton("Forward →");toolbar->addWidget(back);toolbar->addWidget(forward);
    search=new QLineEdit;search->setObjectName("helpSearch");search->setPlaceholderText("Search the whole book — e.g. pause menu, on_press, save slot");search->setClearButtonEnabled(true);search->setAccessibleName("Search documentation");toolbar->addWidget(search,1);layout->addLayout(toolbar);
    auto *split=new QSplitter;contents=new QListWidget;contents->setObjectName("helpTopics");contents->setAccessibleName("Documentation topics and search results");contents->setWordWrap(true);contents->setMinimumWidth(250);reader=new QTextBrowser;reader->setObjectName("helpReader");reader->setAccessibleName("Documentation article");reader->setOpenLinks(false);reader->setOpenExternalLinks(false);reader->setMinimumWidth(420);split->addWidget(contents);split->addWidget(reader);split->setStretchFactor(1,1);split->setSizes({300,760});layout->addWidget(split,1);
    auto *bottom=new QHBoxLayout;status=new QLabel;bottom->addWidget(status,1);auto *previous=new QPushButton("Previous match"),*next=new QPushButton("Next match");bottom->addWidget(previous);bottom->addWidget(next);auto *close=new QPushButton("Close");bottom->addWidget(close);layout->addLayout(bottom);
    for(const auto &entry:QVector<QPair<QString,QString>>{{"docs/handbook.md","Handbook"},{"docs/editor-authoring.md","Authoring reference"},{"docs/map-authoring.md","Map building"},{"docs/project-format.md","Project format"},{"docs/application-updates.md","Application updates"},{"examples/pokemon/README.md","Pokémon walkthrough"},{"examples/menu-saves/README.md","Menus and saves example"},{"examples/scenes/README.md","Scenes and screens example"}})load(root+"/"+entry.first,entry.second);
    connect(search,&QLineEdit::textChanged,this,[this]{filter();});connect(contents,&QListWidget::currentRowChanged,this,[this](int row){if(row>=0)showTopic(contents->item(row)->data(Qt::UserRole).toInt());});
    connect(back,&QPushButton::clicked,this,[this]{if(historyIndex>0)showTopic(history[--historyIndex],false);});connect(forward,&QPushButton::clicked,this,[this]{if(historyIndex+1<history.size())showTopic(history[++historyIndex],false);});
    auto findMatch=[this](bool previous){auto term=search->text().simplified().section(' ',0,0);if(term.isEmpty())return;auto flags=previous?QTextDocument::FindBackward:QTextDocument::FindFlags{};if(!reader->find(term,flags)){auto cursor=reader->textCursor();cursor.movePosition(previous?QTextCursor::End:QTextCursor::Start);reader->setTextCursor(cursor);reader->find(term,flags);}};
    connect(previous,&QPushButton::clicked,this,[findMatch]{findMatch(true);});connect(next,&QPushButton::clicked,this,[findMatch]{findMatch(false);});connect(close,&QPushButton::clicked,this,&QDialog::close);connect(reader,&QTextBrowser::anchorClicked,this,[this](const QUrl &url){followLink(url);});
    auto *focusSearch=new QShortcut(QKeySequence::Find,this);connect(focusSearch,&QShortcut::activated,this,[this]{search->setFocus();search->selectAll();});
    filter();search->setFocus();
}
void HelpBook::load(const QString &path,const QString &chapter){
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return;auto text=QString::fromUtf8(file.readAll());Topic topic{chapter,QFileInfo(path).absoluteFilePath(),{}, {}};bool fenced=false;
    auto flush=[&]{if(!topic.markdown.trimmed().isEmpty())topics<<topic;};
    for(auto line:text.split('\n')){if(line.trimmed().startsWith("```")||line.trimmed().startsWith("~~~"))fenced=!fenced;
        if(!fenced&&(line.startsWith("# ")||line.startsWith("## "))){flush();auto heading=line.mid(line.indexOf(' ')+1).trimmed();topic={chapter+" · "+heading,QFileInfo(path).absoluteFilePath(),anchorFor(heading),{}};}
        topic.markdown+=line+'\n';
    }flush();
}
void HelpBook::filter(){
    QSignalBlocker block(contents);contents->clear();auto terms=search->text().simplified().split(' ',Qt::SkipEmptyParts);int selected=-1;
    for(int i=0;i<topics.size();++i){bool matches=true;auto haystack=topics[i].title+'\n'+topics[i].markdown;for(auto term:terms)if(!haystack.contains(term,Qt::CaseInsensitive)){matches=false;break;}if(!matches)continue;auto *item=new QListWidgetItem(topics[i].title,contents);item->setData(Qt::UserRole,i);item->setToolTip(topics[i].markdown.left(600));if(i==current)selected=contents->count()-1;}
    status->setText(QString("%1 of %2 topics%3").arg(contents->count()).arg(topics.size()).arg(terms.isEmpty()?QString():" · all search words must match"));
    if(!contents->count()){reader->setPlainText(topics.isEmpty()?"The documentation edition for this application is missing. Reinstall a matching complete application package.":"No matching topics. Try fewer words, a command name, or clear the search to browse the book.");reader->setExtraSelections({});return;}
    contents->setCurrentRow(selected>=0?selected:0);showTopic(contents->currentItem()->data(Qt::UserRole).toInt());
}
void HelpBook::showTopic(int index,bool remember){
    if(index<0||index>=topics.size())return;if(remember&&(historyIndex<0||history[historyIndex]!=index)){history.resize(historyIndex+1);history<<index;historyIndex=history.size()-1;}current=index;
    {QSignalBlocker block(contents);contents->setCurrentRow(-1);for(int i=0;i<contents->count();++i)if(contents->item(i)->data(Qt::UserRole).toInt()==index){contents->setCurrentRow(i);break;}}
    reader->document()->setBaseUrl(QUrl::fromLocalFile(topics[index].source));reader->setMarkdown(topics[index].markdown);highlight();back->setEnabled(historyIndex>0);forward->setEnabled(historyIndex+1<history.size());
}
void HelpBook::highlight(){
    QList<QTextEdit::ExtraSelection> matches;for(auto term:search->text().simplified().split(' ',Qt::SkipEmptyParts)){QTextCursor cursor(reader->document());while(!(cursor=reader->document()->find(term,cursor)).isNull()){QTextEdit::ExtraSelection match;match.cursor=cursor;match.format.setBackground(QColor("#ffe08a"));match.format.setForeground(QColor("#17212b"));matches<<match;}}
    reader->setExtraSelections(matches);if(!matches.isEmpty()){reader->setTextCursor(matches.first().cursor);reader->ensureCursorVisible();}
}
void HelpBook::followLink(const QUrl &url){
    if(url.scheme()=="https"||url.scheme()=="http"){QDesktopServices::openUrl(url);return;}if(current<0)return;
    auto resolved=QUrl::fromLocalFile(topics[current].source).resolved(url);if(!resolved.isLocalFile())return;auto source=QFileInfo(resolved.toLocalFile()).absoluteFilePath();int found=-1;
    for(int i=0;i<topics.size();++i)if(topics[i].source==source){if(found<0)found=i;if(!resolved.fragment().isEmpty()){bool match=topics[i].anchor==resolved.fragment();for(auto line:topics[i].markdown.split('\n'))if(line.startsWith('#')){int space=line.indexOf(' ');if(space>0&&anchorFor(line.mid(space+1).trimmed())==resolved.fragment())match=true;}if(match){found=i;break;}}}
    if(found<0){status->setText("That linked file is not a chapter in this book.");return;}showTopic(found);if(!resolved.fragment().isEmpty())reader->scrollToAnchor(resolved.fragment());
}
}
