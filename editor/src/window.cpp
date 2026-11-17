#include "window.h"
#include "graph.h"
#include "cf_flow.h"
#include "world_editor.h"
#include "emulator_widget.h"
#include "script_editor.h"
#include "animation_editor.h"
#include "data_editor.h"
#include "pixel_editor.h"
#include "event_tools.h"
#include "screen_builder.h"
#include "script_workspace.h"
#include <QtWidgets>
#include <QtConcurrent>
#include <QUuid>
namespace cf {
class PreviewScreen : public QLabel {
public:
    using QLabel::QLabel;
    bool dmg = false;QImage portraitImage;QVector<quint16> palette;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.fillRect(rect(), QColor("#44515c"));
        QRect inner = rect().adjusted(7, 7, -7, -7);
        painter.setViewport(inner); painter.setWindow(0, 0, 160, 144);
        painter.fillRect(0, 0, 160, 144, (dmg ? QColor("#e0edc4") : QColor("#efefce")));
        auto image = textScreenImage(text().split('\n'),QRect(0,0,20,18),dmg,palette);
        painter.drawImage(0,0,image);if(!portraitImage.isNull())painter.drawImage(152-portraitImage.width(),0,portraitImage);
    }
};
static QLabel *caption(const QString &text) {
    auto *l = new QLabel(text); l->setObjectName("caption"); return l;
}
static QPushButton *button(const QString &text, QBoxLayout *layout, std::function<void()> fn) {
    auto *b = new QPushButton(text); layout->addWidget(b); QObject::connect(b, &QPushButton::clicked, b, fn); return b;
}
Window::Window() {
    setWindowTitle("CartFoundry"); resize(1480, 980); setMinimumSize(1120, 780);
    setWindowIcon(QIcon(resourceRoot() + "/images/cf_icon.png"));
    auto *file = menuBar()->addMenu("&File");
    auto *a = file->addAction("&New Project"); a->setShortcut(QKeySequence::New); connect(a, &QAction::triggered, this, [this] { if (mayDiscard()) fresh(); });
    a = file->addAction("&Open Project…"); a->setShortcut(QKeySequence::Open); connect(a, &QAction::triggered, this, [this] {
        if (!mayDiscard()) return;
        auto f = QFileDialog::getOpenFileName(this, "Open CartFoundry project", {}, "CartFoundry (*.cartfoundry)"); if (!f.isEmpty()) openProject(f);
    });
    a = file->addAction("&Save"); a->setShortcut(QKeySequence::Save); connect(a, &QAction::triggered, this, [this] { save(); });
    a = file->addAction("Save &As…"); a->setShortcut(QKeySequence::SaveAs); connect(a, &QAction::triggered, this, [this] { save(true); });
    file->addSeparator();auto *settings=file->addAction("Project Settings…");connect(settings,&QAction::triggered,this,[this]{projectSettings();});
    file->addSeparator(); a = file->addAction("Export Game Boy Color ROM…"); a->setShortcut(QKeySequence("Ctrl+B")); connect(a, &QAction::triggered, this, [this] { exportRom(); });
    auto *targetMenu=menuBar()->addMenu("&Target");
    for(auto pair:{qMakePair(QString("Game Boy"),QString("gb")),qMakePair(QString("Game Boy Color"),QString("gbc"))}) {
        auto *mode=targetMenu->addAction(pair.first);connect(mode,&QAction::triggered,this,[this,pair]{mutate([&]{project.target=pair.second;});});
    }
    auto *edit = menuBar()->addMenu("&Edit");
    undoAction = edit->addAction("Undo project change"); undoAction->setShortcut(QKeySequence::Undo);
    redoAction = edit->addAction("Redo project change"); redoAction->setShortcut(QKeySequence::Redo);
    connect(undoAction, &QAction::triggered, this, [this] { if (undo.isEmpty()) return; redo << project; project = undo.takeLast(); resetPreview(); refresh(); });
    connect(redoAction, &QAction::triggered, this, [this] { if (redo.isEmpty()) return; undo << project; project = redo.takeLast(); resetPreview(); refresh(); });
    auto *root = new QWidget; auto *vertical = new QVBoxLayout(root); vertical->setContentsMargins(0, 0, 0, 0); vertical->setSpacing(0); setCentralWidget(root);
    auto *bar = new QWidget; bar->setObjectName("topbar"); auto *toolbar = new QHBoxLayout(bar); toolbar->setContentsMargins(20, 12, 20, 12);
    auto *brand = new QLabel; QPixmap brandImage(resourceRoot() + "/images/cf_text.png");
    if (!brandImage.isNull()) brand->setPixmap(brandImage.scaled(180, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)); else brand->setText("CARTFOUNDRY");
    toolbar->addWidget(brand); toolbar->addSpacing(20); toolbar->addWidget(caption("GAME BOY ENGINE & EDITOR"));
    targetMode = new QComboBox; targetMode->addItem("Game Boy Color", "gbc"); targetMode->addItem("Game Boy", "gb"); toolbar->addWidget(targetMode); toolbar->addStretch();
    button("New", toolbar, [this] { if (mayDiscard()) fresh(); });
    button("Open…", toolbar, [this] { if (!mayDiscard()) return; auto f = QFileDialog::getOpenFileName(this, "Open project", {}, "CartFoundry (*.cartfoundry)"); if (!f.isEmpty()) openProject(f); });
    button("Save", toolbar, [this] { save(); });
    buildButton = button("Build ROM…", toolbar, [this] { exportRom(); }); buildButton->setObjectName("primary");
    auto *runButton = button("Build & Play",toolbar,[this]{buildAndPlay(project);}); runButton->setObjectName("primary");
    vertical->addWidget(bar);
    workspace = new QTabWidget; vertical->addWidget(workspace, 1);
    auto *split = new QSplitter; workspace->addTab(split, "Conversations & textboxes");
    worldEditor = new WorldEditor; workspace->addTab(worldEditor, "Sprites, tilesets & overworld");
    animations = new AnimationEditor;workspace->addTab(animations,"Sprite animations");animations->onEdit=[this](const Project &p){mutate([&]{project=p;});};
    scripts = new ScriptEditor; workspace->addTab(scripts,"Controls & behaviours");scripts->onEdit=[this](const Project &p){mutate([&]{project=p;});};
    dataEditor = new DataEditor; workspace->addTab(dataEditor,"Project & game data"); dataEditor->onEdit=[this](const Project &p){mutate([&]{project=p;});};
    dataEditor->onOpenArea=[this](int i){worldEditor->selectArea(i);workspace->setCurrentWidget(worldEditor);};
    pixels=new PixelEditor;workspace->addTab(pixels,"Pixel art");pixels->onEdit=[this](const Project &p){mutate([&]{project=p;});};
    screens=new ScreenBuilder;workspace->addTab(screens,"Scenes & screen builder");screens->onEdit=[this](const Project&p){mutate([&]{project=p;});};screens->onTest=[this](const Project&p){buildAndPlay(p);};
    scriptWorkspace=new ScriptWorkspace;workspace->addTab(scriptWorkspace,"Scripts");scriptWorkspace->onEdit=[this](const Project&p){mutate([&]{project=p;});};
    dataEditor->onOpenScene=[this](int i){screens->selectScene(i);workspace->setCurrentWidget(screens);};
    emulator = new EmulatorWidget; workspace->addTab(emulator, "ROM emulator");
    worldEditor->onTestArea = [this](const Project &p){buildAndPlay(p);};
    worldEditor->onEdit = [this](const Project &p) { mutate([&] { project = p; }); };
    auto *previewClock=new QTimer(this);previewClock->setTimerType(Qt::PreciseTimer);auto elapsed=std::make_shared<QElapsedTimer>();elapsed->start();auto last=std::make_shared<qint64>(0);auto debt=std::make_shared<double>(0);connect(previewClock,&QTimer::timeout,this,[this,elapsed,last,debt]{auto now=elapsed->nsecsElapsed();double dt=(now-*last)/1e6;*last=now;if(workspace->currentIndex()!=0||!isActiveWindow()||project.timers.isEmpty()){*debt=0;return;}*debt=qMin(8*1000.0/60,*debt+dt);while(*debt>=1000.0/60){session.tick(true);*debt-=1000.0/60;}if(session.state.dirty||session.state.request_world||session.state.request_prompt!=255||session.state.request_storage!=255)updatePreview();});previewClock->start(4);
    auto *worldAction = file->addAction("New Overworld Example");
    connect(worldAction, &QAction::triggered, this, [this] { if (!mayDiscard()) return; project = Project::overworldExample(); path.clear(); selected = project.entry; undo.clear(); redo.clear(); saved = bytes(); resetPreview(); refresh(); workspace->setCurrentIndex(1); });
    auto *pokemonAction=file->addAction("New Pokemon Example");connect(pokemonAction,&QAction::triggered,this,[this]{if(!mayDiscard())return;project=Project::pokemonExample();path.clear();selected=project.entry;undo.clear();redo.clear();saved=bytes();resetPreview();refresh();workspace->setCurrentWidget(worldEditor);});
    auto *helpMenu=menuBar()->addMenu("&Help");auto *guide=helpMenu->addAction("Pokemon example guide");connect(guide,&QAction::triggered,this,[this]{QFile file(resourceRoot()+"/examples/pokemon/README.md");if(!file.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Guide unavailable",file.errorString());return;}QDialog dialog(this);dialog.setWindowTitle("Pocket Meadow — build it by hand");dialog.resize(950,800);auto *layout=new QVBoxLayout(&dialog);auto *browser=new QTextBrowser;browser->setOpenExternalLinks(true);browser->setMarkdown(QString::fromUtf8(file.readAll()));layout->addWidget(browser);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();});
    auto *coverage=helpMenu->addAction("Authoring controls & engine coverage");connect(coverage,&QAction::triggered,this,[this]{QFile file(resourceRoot()+"/docs/editor-authoring.md");if(!file.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Guide unavailable",file.errorString());return;}QDialog dialog(this);dialog.setWindowTitle("Authoring controls & engine coverage");dialog.resize(950,800);auto *layout=new QVBoxLayout(&dialog);auto *browser=new QTextBrowser;browser->setOpenExternalLinks(true);browser->setMarkdown(QString::fromUtf8(file.readAll()));layout->addWidget(browser);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();});
    auto *left = new QWidget; left->setMinimumWidth(185); left->setMaximumWidth(260); auto *ll = new QVBoxLayout(left); ll->setContentsMargins(16, 18, 16, 16);
    ll->addWidget(caption("PROJECT")); projectTitle = new QLineEdit; projectTitle->setPlaceholderText("Project title"); ll->addWidget(projectTitle);
    ll->addWidget(caption("CONVERSATION PREVIEW START")); entry = new QComboBox; ll->addWidget(entry); ll->addSpacing(16);
    ll->addWidget(caption("PROMPTS")); promptList = new QListWidget; ll->addWidget(promptList, 1);
    button("+ Add prompt", ll, [this] { addPrompt(); });
    removeButton = button("Delete prompt", ll, [this] { deletePrompt(); }); ll->addSpacing(12);
    auto *help = new QLabel("Connect orange choice ports\nto mint prompt ports.\n\nDrag cards to arrange.\nScroll to pan.\nCtrl / Cmd + scroll to zoom."); help->setObjectName("hint"); help->setWordWrap(true); ll->addWidget(help);
    split->addWidget(left);
    auto *center = new QWidget; auto *cl = new QVBoxLayout(center); cl->setContentsMargins(0, 0, 0, 0); cl->setSpacing(0);
    auto *graphbar = new QHBoxLayout; graphbar->setContentsMargins(18, 10, 18, 10); graphbar->addWidget(caption("CONVERSATION FLOW")); graphbar->addStretch(); button("Frame all", graphbar, [this] { graph->frameAll(); }); cl->addLayout(graphbar);
    graph = new GraphView; cl->addWidget(graph, 1);
    validation = new QLabel; validation->setWordWrap(true); validation->setMargin(14); validation->setObjectName("validation"); cl->addWidget(validation);
    log = new QPlainTextEdit; log->setReadOnly(true); log->setMaximumHeight(125); log->setPlaceholderText("Build output will appear here."); cl->addWidget(log);
    split->addWidget(center);
    auto *scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setMinimumWidth(330); scroll->setMaximumWidth(400);
    auto *right = new QWidget; auto *rl = new QVBoxLayout(right); rl->setContentsMargins(18, 18, 18, 18); rl->setSpacing(10);
    rl->addWidget(caption("PROMPT INSPECTOR"));
    promptTitle = new QLineEdit; promptTitle->setPlaceholderText("Prompt title (18 columns)"); rl->addWidget(promptTitle);
    body = new QPlainTextEdit; body->setPlaceholderText("Prompt text…"); body->setFixedHeight(135); rl->addWidget(body);
    rl->addWidget(caption("TEXTBOX (TILE UNITS)"));
    auto *boxForm = new QGridLayout;
    boxX = new QSpinBox; boxY = new QSpinBox; boxW = new QSpinBox; boxH = new QSpinBox;
    boxX->setRange(0,14); boxY->setRange(0,11); boxW->setRange(6,20); boxH->setRange(7,18);
    int boxColumn = 0;
    for (auto pair : {qMakePair(QString("X"),boxX), qMakePair(QString("Y"),boxY), qMakePair(QString("Width"),boxW), qMakePair(QString("Height"),boxH)}) {
        pair.second->setAccessibleName("Textbox " + pair.first); boxForm->addWidget(new QLabel(pair.first),0,boxColumn); boxForm->addWidget(pair.second,1,boxColumn++);
        connect(pair.second,&QSpinBox::valueChanged,this,[this](int){ if(filling)return; int i=project.indexOf(selected); if(i<0)return;
            mutate([&]{ int w=boxW->value(),h=boxH->value();int x=qMin(boxX->value(),20-w),y=qMin(boxY->value(),18-h);project.prompts[i].textbox={x,y,w,h}; },true);
        });
    }
    rl->addLayout(boxForm);
    auto *textOptions=new QGroupBox("Text style & layout");textSettingsLayout=new QVBoxLayout(textOptions);rl->addWidget(textOptions);
    promptId=new QLineEdit;promptId->setPlaceholderText("Prompt ID for scripts");rl->addWidget(new QLabel("Prompt ID (set before wiring scripts)"));rl->addWidget(promptId);connect(promptId,&QLineEdit::editingFinished,this,[this]{if(filling)return;int i=project.indexOf(selected);if(i<0||promptId->text()==selected)return;QString id=promptId->text().trimmed();if(id.isEmpty()||project.indexOf(id)>=0){fillInspector();return;}mutate([&]{QString old=selected;project.prompts[i].id=id;for(auto &n:project.prompts)for(auto &c:n.choices)if(c.target==old)c.target=id;if(project.entry==old)project.entry=id;for(auto *w:{&project.world})for(auto it=w->interactions.begin();it!=w->interactions.end();++it)if(it.value()==old)it.value()=id;for(auto &w:project.areas)for(auto it=w.interactions.begin();it!=w.interactions.end();++it)if(it.value()==old)it.value()=id;selected=id;},true);});
    modal=new QCheckBox("Modal dialogue (disable Back, Bag and position reset)");rl->addWidget(modal);connect(modal,&QCheckBox::toggled,this,[this](bool v){if(!filling){int i=project.indexOf(selected);if(i>=0)mutate([&]{project.prompts[i].modal=v;});}});portrait=new QComboBox;rl->addWidget(new QLabel("Portrait above textbox"));rl->addWidget(portrait);connect(portrait,&QComboBox::activated,this,[this](int n){int i=project.indexOf(selected);if(i>=0)mutate([&]{project.prompts[i].portrait=portrait->itemData(n).toString();});});
    rl->addWidget(caption("ON ENTER SCRIPT"));onEnter=new QPlainTextEdit;onEnter->setAccessibleName("Prompt entry script");onEnter->setPlaceholderText("On enter script (e.g. add visits 1)");onEnter->setMaximumHeight(80);rl->addWidget(eventTools(onEnter,[this]{return project;}));connect(onEnter,&QPlainTextEdit::textChanged,this,[this]{if(!filling){int i=project.indexOf(selected);if(i>=0)mutate([&]{project.prompts[i].onEnter=onEnter->toPlainText();});}});
    rl->addWidget(caption("CHOICES & DESTINATIONS")); choicesWidget = new QWidget; choicesLayout = new QVBoxLayout(choicesWidget); choicesLayout->setContentsMargins(0, 0, 0, 0); choicesLayout->setSpacing(8); rl->addWidget(choicesWidget);
    button("+ Add choice", rl, [this] {
        int i = project.indexOf(selected); if (i < 0 || project.prompts[i].choices.size() >= 3) return;
        mutate([&] { project.prompts[i].choices.append(Choice{"New choice", QString()}); }, true);
    });
    rl->addSpacing(12); rl->addWidget(caption("PLAY PREVIEW"));
    previewLabel = new QLabel; previewLabel->setObjectName("hint"); rl->addWidget(previewLabel);
    screen = new PreviewScreen; screen->setObjectName("screen"); screen->setAlignment(Qt::AlignTop | Qt::AlignLeft); screen->setTextInteractionFlags(Qt::NoTextInteraction);
    QFont mono("Courier New"); mono.setStyleHint(QFont::Monospace); mono.setPixelSize(14); screen->setFont(mono);
    screen->setFixedSize(300, 272); rl->addWidget(screen, 0, Qt::AlignHCenter);
    auto *controls = new QHBoxLayout;
    button("↑", controls, [this] { previewInput(1); }); button("↓", controls, [this] { previewInput(2); });
    auto *accept = button("A", controls, [this] { previewInput(4); }); accept->setToolTip("Choose the highlighted option");
    button("B / Reset", controls, [this] { previewInput(8); }); button("Select",controls,[this]{previewInput(16);}); rl->addLayout(controls);
    button("Play from selected prompt", rl, [this] { previewMenu=-1;session.configure(project);CFState state{};session.enterPrompt(state,project.indexOf(selected));previewPrompt=state.prompt;previewChoice=state.choice;updatePreview(); });
    button("Build & Play this conversation", rl, [this] { buildAndPlay(project.isolatedPrompt(selected)); });
    auto *note = new QLabel("Text-flow simulation · 20 × 18 tiles\nThe exported ROM runs our C engine."); note->setObjectName("hint"); note->setWordWrap(true); rl->addWidget(note); rl->addStretch();
    scroll->setWidget(right); split->addWidget(scroll); split->setStretchFactor(1, 1); split->setSizes({205, 900, 350});
    connect(targetMode, &QComboBox::activated, this, [this](int i) { mutate([&] { project.target = targetMode->itemData(i).toString(); }); });
    connect(projectTitle, &QLineEdit::textEdited, this, [this](const QString &s) { mutate([&] { project.title = s; }); });
    connect(entry, &QComboBox::activated, this, [this](int i) { mutate([&] { project.entry = entry->itemData(i).toString(); resetPreview(); }); });
    connect(promptList, &QListWidget::currentRowChanged, this, [this](int i) { if (!filling && i >= 0 && i < project.prompts.size()) select(project.prompts[i].id); });
    connect(promptTitle, &QLineEdit::textEdited, this, [this](const QString &s) { int i = project.indexOf(selected); if (i >= 0) mutate([&] { project.prompts[i].title = s; }); });
    connect(body, &QPlainTextEdit::textChanged, this, [this] { if (filling) return; int i = project.indexOf(selected); if (i >= 0) mutate([&] { project.prompts[i].text = body->toPlainText(); }); });
    graph->onSelect = [this](const QString &id) { select(id); };
    graph->onConnect = [this](const QString &id, int c, const QString &to) { int i = project.indexOf(id); if (i >= 0) mutate([&] { project.prompts[i].choices[c].target = to; }, true); };
    graph->onMove = [this](const QVector<QPointF> &positions) { mutate([&] { for (int i = 0; i < positions.size(); ++i) project.prompts[i].position = positions[i]; }); };
    connect(&watcher, &QFutureWatcher<BuildResult>::finished, this, [this] {
        auto result = watcher.result(); building = false; buildButton->setEnabled(true); buildButton->setText("Build ROM…");
        log->setPlainText(result.log);
        if(result.success && runAfterBuild) { workspace->setCurrentWidget(emulator); emulator->load(result.romPath); }
        runAfterBuild = false;
        statusBar()->showMessage(result.success ? "ROM exported successfully" : "Build failed — see output", 10000);
    });
    fresh(); QTimer::singleShot(0, graph, [this] { graph->frameAll(); });
}
QByteArray Window::bytes() const { return QJsonDocument(project.toJson()).toJson(QJsonDocument::Compact); }
bool Window::dirty() const { return bytes() != saved; }
bool Window::mayDiscard() {
    if (!dirty()) return true;
    auto answer = QMessageBox::question(this, "Unsaved project", "Save your project changes?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) return save();
    return answer == QMessageBox::Discard;
}
void Window::closeEvent(QCloseEvent *e) {
    if (building) { QMessageBox::information(this, "Build in progress", "Wait for the ROM build to finish before closing CartFoundry."); e->ignore(); return; }
    if (mayDiscard()) e->accept(); else e->ignore();
}
void Window::projectSettings(){QDialog dialog(this);dialog.setWindowTitle("Project Settings");dialog.resize(560,340);auto *form=new QFormLayout(&dialog);auto *title=new QLineEdit(project.title);form->addRow("Game title",title);auto *target=new QComboBox;target->addItem("Game Boy","gb");target->addItem("Game Boy Color","gbc");target->setCurrentIndex(target->findData(project.target));form->addRow("Target",target);auto *scene=new QComboBox;for(auto &s:project.scenes)scene->addItem(s.name+" ["+s.id+"]",s.id);scene->setCurrentIndex(scene->findData(project.defaultScene));form->addRow("Default Scene",scene);auto *duration=new QDoubleSpinBox;duration->setRange(1.0/60,10);duration->setDecimals(3);duration->setValue(project.splashTicks/60.0);duration->setSuffix(" seconds");form->addRow("Platform splash duration",duration);auto *help=new QLabel("Startup order: GB/GBC splash → Default Scene → onSceneEnter.\nWorld scenes first load their Initial Area and its onAreaEnter script.\nDialogue starts only from an explicit dialogue event.\nSelect a scene in Scenes & screen builder to edit its entry script.");help->setWordWrap(true);form->addRow(help);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()==QDialog::Accepted)mutate([&]{project.title=title->text();project.target=target->currentData().toString();project.defaultScene=scene->currentData().toString();project.splashTicks=qBound(1,qRound(duration->value()*60),600);});}
void Window::fresh() {
    project = Project::blank(); path.clear(); selected = project.entry; undo.clear(); redo.clear(); saved = bytes(); resetPreview(); refresh();workspace->setCurrentWidget(screens);
}
bool Window::openProject(const QString &f) {
    Project p; QString error;
    if (!Project::load(f, p, error)) { QMessageBox::warning(this, "Cannot open project", error); return false; }
    project = p; path = f; selected = project.entry; undo.clear(); redo.clear(); saved = bytes(); resetPreview(); refresh();
    QTimer::singleShot(0, graph, [this] { graph->frameAll(); }); return true;
}
bool Window::save(bool saveAs) {
    QString f = path;
    if (f.isEmpty() || saveAs) f = QFileDialog::getSaveFileName(this, "Save CartFoundry project", path.isEmpty() ? "game.cartfoundry" : path, "CartFoundry (*.cartfoundry)");
    if (f.isEmpty()) return false;
    if (!f.endsWith(".cartfoundry", Qt::CaseInsensitive)) f += ".cartfoundry";
    QString error;
    if (!project.save(f, error)) { QMessageBox::warning(this, "Cannot save project", error); return false; }
    path = f; saved = bytes(); refresh(false); statusBar()->showMessage("Saved " + f, 5000); return true;
}
void Window::mutate(const std::function<void()> &fn, bool refill) {
    auto before = project; const auto old = bytes(); fn();
    if (bytes() == old) return;
    undo << before; if (undo.size() > 200) undo.removeFirst(); redo.clear(); refresh(refill);
}
void Window::refresh(bool refill) {
    filling = true;
    targetMode->setCurrentIndex(targetMode->findData(project.target));
    if (project.indexOf(selected) < 0) selected = project.prompts.isEmpty() ? QString() : project.prompts.first().id;
    if (projectTitle->text() != project.title) projectTitle->setText(project.title);
    entry->clear(); promptList->clear();
    for (const auto &p : project.prompts) { entry->addItem(p.title, p.id); promptList->addItem((p.id == project.entry ? "▶  " : "    ") + p.title); }
    entry->setCurrentIndex(project.indexOf(project.entry)); promptList->setCurrentRow(project.indexOf(selected));
    graph->setProject(project, selected); worldEditor->setProject(project); scripts->setProject(project); animations->setProject(project); dataEditor->setProject(project); pixels->setProject(project);screens->setProject(project);scriptWorkspace->setProject(project);session.configure(project,false); filling = false;
    if (refill) fillInspector();
    auto errors = project.validate();
    validation->setText(errors.isEmpty() ? QString("✓ Ready to build   ·   %1 prompts   ·   %2").arg(project.prompts.size()).arg(project.target == "gb" ? "Game Boy" : "Game Boy Color") : "Build needs attention:\n" + errors.mid(0, 4).join('\n') + (errors.size() > 4 ? QString("\n…and %1 more issues").arg(errors.size() - 4) : QString()));
    validation->setProperty("valid", errors.isEmpty()); validation->style()->unpolish(validation); validation->style()->polish(validation);
    undoAction->setEnabled(!undo.isEmpty()); redoAction->setEnabled(!redo.isEmpty()); removeButton->setEnabled(project.prompts.size() > 1);
    setWindowTitle((dirty() ? "• " : "") + project.title + " — CartFoundry"); updatePreview();
}
void Window::select(const QString &id) {
    selected = id; filling = true; promptList->setCurrentRow(project.indexOf(id)); filling = false;
    graph->selectPrompt(selected); fillInspector();
}
void Window::fillInspector() {
    filling = true;
    while(auto *item=textSettingsLayout->takeAt(0)){if(item->widget())item->widget()->deleteLater();delete item;}
    int i = project.indexOf(selected);if(i>=0){auto *widget=new QWidget;auto *form=new QFormLayout(widget);auto &prompt=project.prompts[i];auto toggle=[&](QString label,bool value,std::function<void(Prompt &,bool)> fn){auto *w=new QCheckBox;w->setChecked(value);form->addRow(label,w);connect(w,&QCheckBox::toggled,this,[this,i,fn](bool value){if(!filling)mutate([&]{fn(project.prompts[i],value);});});};auto number=[&](QString label,int value,int min,int max,std::function<void(Prompt &,int)> fn){auto *w=new QSpinBox;w->setRange(min,max);w->setValue(value);form->addRow(label,w);connect(w,&QSpinBox::valueChanged,this,[this,i,fn](int value){if(!filling)mutate([&]{fn(project.prompts[i],value);});});};auto label=[&](QString title,QString value,int limit,std::function<void(Prompt &,QString)> fn){auto *w=new QLineEdit(value);w->setMaxLength(limit);form->addRow(title,w);connect(w,&QLineEdit::textEdited,this,[this,i,fn](QString value){if(!filling)mutate([&]{fn(project.prompts[i],value);});});};toggle("Draw border",prompt.border,[](Prompt &p,bool v){p.border=v;});toggle("Show title",prompt.showTitle,[](Prompt &p,bool v){p.showTitle=v;});toggle("Show footer",prompt.showFooter,[](Prompt &p,bool v){p.showFooter=v;});number("Inner padding (tiles)",prompt.padding,0,3,[](Prompt &p,int v){p.padding=v;});number("Choice row spacing",prompt.choiceSpacing,1,3,[](Prompt &p,int v){p.choiceSpacing=v;});auto *align=new QComboBox;align->addItems({"Left","Centre","Right"});align->setCurrentIndex(prompt.textAlign);form->addRow("Body alignment",align);connect(align,&QComboBox::activated,this,[this,i](int v){mutate([&]{project.prompts[i].textAlign=v;});});label("Choice cursor",prompt.cursor,1,[](Prompt &p,QString v){p.cursor=v;});label("Locked-choice marker",prompt.disabledMarker,1,[](Prompt &p,QString v){p.disabledMarker=v;});label("Footer override (empty = controls)",prompt.footer,64,[](Prompt &p,QString v){p.footer=v;});auto *colors=new QHBoxLayout;for(int index:{0,3}){auto *button=new QPushButton(index==0?"Paper colour…":"Ink colour…");colors->addWidget(button);connect(button,&QPushButton::clicked,this,[this,i,index]{auto palette=project.prompts[i].textPalette;if(palette.isEmpty())palette={quint16(29|(29<<5)|(25<<10)),quint16(23|(24<<5)|(20<<10)),quint16(12|(15<<5)|(18<<10)),quint16(4|(7<<5)|(11<<10))};auto color=QColorDialog::getColor(rgb555(palette[index]),this,"Text colour (GBC)");if(color.isValid())mutate([&]{palette[index]=toRgb555(color);project.prompts[i].textPalette=palette;});});}auto *reset=new QPushButton("Default colours");colors->addWidget(reset);connect(reset,&QPushButton::clicked,this,[this,i]{mutate([&]{project.prompts[i].textPalette.clear();});});form->addRow(colors);textSettingsLayout->addWidget(widget);}
    promptTitle->setEnabled(i >= 0); body->setEnabled(i >= 0);
    auto box = i < 0 ? QRect(0,0,20,18) : project.prompts[i].textbox;
    boxX->setValue(box.x()); boxY->setValue(box.y()); boxW->setValue(box.width()); boxH->setValue(box.height());
    promptTitle->setText(i < 0 ? "" : project.prompts[i].title); body->setPlainText(i < 0 ? "" : project.prompts[i].text);
    promptId->setText(i<0?QString():project.prompts[i].id);modal->setChecked(i>=0&&project.prompts[i].modal);portrait->clear();portrait->addItem("None",QString());for(auto &a:project.assets)if(a.kind=="sprite")portrait->addItem(a.name,a.id);portrait->setCurrentIndex(i<0?0:qMax(0,portrait->findData(project.prompts[i].portrait)));
    onEnter->setPlainText(i<0?QString():project.prompts[i].onEnter);
    while (auto *item = choicesLayout->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
    if (i >= 0) for (int c = 0; c < project.prompts[i].choices.size(); ++c) {
        auto *row = new QWidget; auto *v = new QVBoxLayout(row); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(4);
        auto *h = new QHBoxLayout; auto *label = new QLineEdit(project.prompts[i].choices[c].text); label->setPlaceholderText("Choice label (17 columns)"); h->addWidget(label);
        auto *remove = new QPushButton("−"); remove->setFixedWidth(30); h->addWidget(remove); v->addLayout(h);
        auto *target = new QComboBox; target->addItem("Not connected", QString());
        for (const auto &p : project.prompts) target->addItem(p.title, p.id);
        target->setCurrentIndex(qMax(0, target->findData(project.prompts[i].choices[c].target)));
        v->addWidget(target);
        auto *condition=new QLineEdit(project.prompts[i].choices[c].condition);condition->setPlaceholderText("Enabled when: coins >= 5 / has inventory key >= 1");v->addWidget(conditionTools(condition,[this]{return project;}));
        auto *actions=new QPlainTextEdit(project.prompts[i].choices[c].actions);actions->setPlaceholderText("On choose script: add coins -5");actions->setMaximumHeight(65);v->addWidget(eventTools(actions,[this]{return project;}));
        connect(condition,&QLineEdit::textChanged,this,[this,i,c](QString text){mutate([&]{project.prompts[i].choices[c].condition=text;});});
        connect(actions,&QPlainTextEdit::textChanged,this,[this,i,c,actions]{mutate([&]{project.prompts[i].choices[c].actions=actions->toPlainText();});});
        choicesLayout->addWidget(row);
        connect(label, &QLineEdit::textEdited, this, [this, i, c](const QString &s) { mutate([&] { project.prompts[i].choices[c].text = s; }); });
        connect(target, &QComboBox::activated, this, [this, i, c, target](int n) { mutate([&] { project.prompts[i].choices[c].target = target->itemData(n).toString(); }); });
        connect(remove, &QPushButton::clicked, this, [this, i, c] { mutate([&] { project.prompts[i].choices.removeAt(c); }, true); });
    }
    filling = false;
}
void Window::addPrompt() {
    if (project.prompts.size() >= 64) { statusBar()->showMessage("The prototype supports up to 64 prompts.", 5000); return; }
    mutate([&] {
        Prompt p; p.id = "prompt_" + QUuid::createUuid().toString(QUuid::Id128); p.title = "NEW PROMPT"; p.text = "Your story here.";
        p.position = graph->mapToScene(graph->viewport()->rect().center()) - QPointF(130, 80); project.prompts << p; selected = p.id;
    }, true);
}
void Window::deletePrompt() {
    int i = project.indexOf(selected); if (i < 0 || project.prompts.size() < 2) return;
    const auto id = selected;
    if (QMessageBox::question(this, "Delete prompt", "Delete this prompt and disconnect choices leading to it?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    mutate([&] {
        project.prompts.removeAt(i);
        for (auto &p : project.prompts) for (auto &c : p.choices) if (c.target == id) c.target.clear();
        if (project.entry == id) project.entry = project.prompts.first().id;
        selected = project.prompts.first().id; resetPreview();
    }, true);
}
void Window::exportRom() {
    if (building) return;
    auto errors = project.validate(); if (!errors.isEmpty()) { log->setPlainText(errors.join('\n')); statusBar()->showMessage("Resolve project issues before exporting.", 5000); return; }
    QString ext = project.target == "gb" ? ".gb" : ".gbc";
    QString initial = path.isEmpty() ? "game" + ext : QFileInfo(path).absolutePath() + "/" + QFileInfo(path).completeBaseName() + ext;
    auto output = QFileDialog::getSaveFileName(this, "Export " + targetMode->currentText() + " ROM", initial, project.target == "gb" ? "Game Boy ROM (*.gb)" : "Game Boy Color ROM (*.gbc)"); if (output.isEmpty()) return;
    if (!output.endsWith(ext, Qt::CaseInsensitive)) output += ext;
    if (QFileInfo(output).absoluteFilePath() == QFileInfo(path).absoluteFilePath()) { log->setPlainText("Choose a ROM path different from the project path."); return; }
    runAfterBuild = false; building = true; buildButton->setEnabled(false); buildButton->setText("Building…"); log->setPlainText("Compiling the current project with the bundled GBDK toolchain…");
    const auto snapshot = project; const auto resources = resourceRoot();
    watcher.setFuture(QtConcurrent::run([snapshot, output, resources] { return buildRom(snapshot, output, resources); }));
}
void Window::buildAndPlay(const Project &snapshot) {
    if(building)return;auto errors=snapshot.validate();if(!errors.isEmpty()){QMessageBox::warning(this,"Cannot test yet",errors.join('\n'));return;}
    QString folder=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/rom-tests";
    QString output=folder+(snapshot.target=="gb"?"/test.gb":"/test.gbc");
    emulator->pause();runAfterBuild=true;building=true;buildButton->setEnabled(false);buildButton->setText("Building…");
    statusBar()->showMessage("Building a test ROM for the embedded emulator…");
    const auto resources=resourceRoot();watcher.setFuture(QtConcurrent::run([snapshot,output,resources]{return buildRom(snapshot,output,resources);}));
}
void Window::previewRequests(){if(session.state.request_world){session.state.request_world=0;previewMenu=-1;session.state.request_prompt=session.state.request_storage=255;}
    for(int i=0;i<4&&session.state.request_prompt<project.prompts.size();++i){int target=session.state.request_prompt;session.state.request_prompt=255;CFState state{};session.enterPrompt(state,target);previewPrompt=state.prompt;previewChoice=state.choice;previewMenu=-1;}
    session.state.request_prompt=255;
    if(session.state.request_storage<project.storages.size()){previewMenu=session.state.request_storage;previewReadOnly=session.state.request_readonly;session.state.request_readonly=0;session.state.request_storage=255;previewSide=previewItem=0;}
}
void Window::resetPreview() { previewMenu=-1;session.configure(project);CFState state{};session.enterPrompt(state,project.indexOf(project.entry));previewPrompt=project.prompts.isEmpty()?-1:state.prompt;previewChoice=state.choice;previewRequests(); }
void Window::previewInput(int input) {
    if(previewMenu<0&&previewPrompt>=0&&previewPrompt<project.prompts.size()&&project.prompts[previewPrompt].modal&&(input==8||input==16))return;
    if(previewMenu>=0){
        if(input==8){previewMenu=-1;updatePreview();return;}
        if(input==16&&previewMenu&&!previewReadOnly){previewSide=!previewSide;previewItem=0;}
        int source=previewMenu&&previewSide?0:previewMenu;QVector<int> ids;for(int i=0;i<project.items.size();++i)if(session.state.counts[source][i])ids<<i;
        if(!ids.isEmpty()){previewItem=qBound(0,previewItem,int(ids.size())-1);if(input==1)previewItem=(previewItem+ids.size()-1)%ids.size();if(input==2)previewItem=(previewItem+1)%ids.size();if(input==4){int item=ids[previewItem];if(previewMenu&&!previewReadOnly)session.state.variables[31]=cf_storage_transfer(&session.state,&session.definition,source,previewSide?previewMenu:0,item,1);else if(session.run(project.items[item].useScript)&&!previewReadOnly&&project.items[item].consumable)cf_storage_change(&session.state,&session.definition,0,item,-1);}}
    }else if(input==16){previewMenu=previewSide=previewItem=0;previewReadOnly=false;}
    else if(input==CF_KEY_RESET){CFState state{};session.enterPrompt(state,project.indexOf(project.entry));previewPrompt=project.prompts.isEmpty()?-1:state.prompt;previewChoice=state.choice;}
    else{CFState state{uint8_t(previewPrompt<0?255:previewPrompt),uint8_t(previewChoice)};session.step(state,input);previewPrompt=state.prompt;previewChoice=state.choice;}
    updatePreview();
}
void Window::updatePreview() {
    previewRequests();
    if (!project.prompts.isEmpty()&&previewPrompt >= project.prompts.size()) resetPreview();
    
    static_cast<PreviewScreen *>(screen)->dmg = project.target == "gb";
    auto rendered=project;for(auto &n:rendered.prompts)n.text=renderGameText(project,session.state,n.text,textLayout(n).columns);auto lines=screenLines(rendered,previewPrompt,previewChoice);auto *preview=static_cast<PreviewScreen *>(screen);preview->portraitImage={};preview->palette=previewPrompt>=0&&previewPrompt<project.prompts.size()?project.prompts[previewPrompt].textPalette:QVector<quint16>{};if(previewMenu<0&&previewPrompt>=0&&previewPrompt<project.prompts.size())if(auto a=project.asset(project.prompts[previewPrompt].portrait))preview->portraitImage=a->frameImage(0,project.target=="gb");
    if(previewPrompt>=0&&previewPrompt<project.prompts.size()){auto box=project.prompts[previewPrompt].textbox;auto layout=textLayout(project.prompts[previewPrompt]);for(int c=0;c<project.prompts[previewPrompt].choices.size();++c)if(!session.enabled(previewPrompt,c)){int row=layout.choiceRow+c*project.prompts[previewPrompt].choiceSpacing;if(row<lines.size()&&box.x()+project.prompts[previewPrompt].padding<lines[row].size())lines[row][box.x()+project.prompts[previewPrompt].padding]=(project.prompts[previewPrompt].disabledMarker.isEmpty()?QChar('x'):project.prompts[previewPrompt].disabledMarker[0]);}}
    if(previewMenu>=0&&previewMenu<project.storages.size()){
        lines.fill(QString(20,' '),18);lines[0]=lines[17]="+------------------+";for(int y=1;y<17;++y){lines[y][0]='|';lines[y][19]='|';}
        auto write=[&](int row,QString value){value=value.left(18);for(int i=0;i<value.size();++i)lines[row][i+1]=value[i];};int source=previewMenu&&previewSide?0:previewMenu;write(1,project.storages[source].name);QVector<int> ids;for(int i=0;i<project.items.size();++i)if(session.state.counts[source][i])ids<<i;previewItem=qBound(0,previewItem,qMax(0,int(ids.size())-1));if(ids.isEmpty())write(4,"Empty");int start=qMax(0,previewItem-9);for(int i=start;i<ids.size()&&i<start+10;++i)write(3+i-start,QString("%1%2 %3").arg(i==previewItem?">":" ").arg(project.items[ids[i]].name.left(12)).arg(session.state.counts[source][ids[i]]));write(15,previewReadOnly?"A:inspect":previewMenu?"A:move Select:swap":"A:use");write(16,"B:close");
    }
    screen->setText(lines.join('\n'));screen->update();session.state.dirty=0;
    previewLabel->setText(previewPrompt >= 0 && previewPrompt < project.prompts.size() ? "Now playing: " + project.prompts[previewPrompt].title : "Conversation preview only — game startup is in Project Settings");
}
}
