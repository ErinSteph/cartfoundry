#include "importer.h"
#include "project.h"
#include <QtWidgets>
#include <QImageReader>
namespace cf {
bool removeAsset(QWidget *parent, const Project &project, const QString &id, Project &result) {
    const auto *asset=project.asset(id);
    if(!asset)return false;
    QStringList uses;
    for(const auto &world:project.allAreas()){
        if(world.player==id)uses<<world.name+": player sprite";
        if(world.tileset==id)uses<<world.name+": tileset";
        for(const auto &actor:world.actors)if(actor.sprite==id)uses<<world.name+" / "+actor.name+": character sprite";
    }
    for(const auto &npc:project.npcs)if(npc.actor.sprite==id)uses<<"NPC library / "+npc.name;
    for(const auto &prompt:project.prompts)if(prompt.portrait==id)uses<<"Portrait / "+prompt.title;
    for(const auto &item:project.items)if(item.icon==id)uses<<"Item icon / "+item.name;
    for(const auto &scene:project.scenes)for(const auto &element:scene.elements)
        if(element.image==id)uses<<scene.name+" / "+element.name+": image";
    if(!uses.isEmpty()){
        QMessageBox message(QMessageBox::Information,"Asset is still in use",
            "Change or remove these uses before removing “"+asset->name+"”.",QMessageBox::Ok,parent);
        message.setInformativeText(uses.join('\n'));message.exec();return false;
    }
    if(QMessageBox::question(parent,"Remove asset", "Remove “"+asset->name+"” and its animations from this project?\nYou can undo this project change.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return false;
    result=project;
    for(int i=0;i<result.assets.size();++i)if(result.assets[i].id==id){result.assets.removeAt(i);break;}
    return true;
}

bool importAsset(QWidget *parent, const QString &kind, RasterAsset &result, bool placeAfterImport) {
    auto path = QFileDialog::getOpenFileName(parent, "Import image as " + kind, {}, "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (path.isEmpty()) return false;
    QImageReader reader(path); auto size = reader.size();
    if (!size.isValid() || size.width() > 8192 || size.height() > 8192 || qint64(size.width())*size.height() > 32*1024*1024) {
        QMessageBox::warning(parent, "Cannot import image", "Choose an image up to 8192 pixels per side and 32 megapixels."); return false;
    }
    reader.setAutoTransform(true); auto source = reader.read();
    if (source.isNull()) { QMessageBox::warning(parent, "Cannot read image", reader.errorString()); return false; }
    QDialog dialog(parent); dialog.setWindowTitle("Image → " + kind); dialog.resize(820,680);
    auto *root = new QVBoxLayout(&dialog); auto *columns = new QHBoxLayout; auto *settings = new QFormLayout;
    auto *name = new QLineEdit(QFileInfo(path).completeBaseName()); settings->addRow("Asset name",name);
    auto *width = new QSpinBox, *height = new QSpinBox; int max = kind == "sprite" ? 32 : 128;
    for (auto *spin : {width,height}) { spin->setRange(8,max); spin->setSingleStep(8); }
    width->setValue(qBound(8,((source.width()+7)/8)*8,max));
    height->setValue(qBound(8,((source.height()+7)/8)*8,max));
    auto *sheetColumns=new QSpinBox,*sheetRows=new QSpinBox;sheetColumns->setRange(1,8);sheetRows->setRange(1,8);
    if(kind=="sprite"){settings->addRow("Sheet width (frames)",sheetColumns);settings->addRow("Sheet height (frames)",sheetRows);}
    settings->addRow(kind=="sprite"?"Frame width (pixels)":"Width (pixels)",width);settings->addRow(kind=="sprite"?"Frame height (pixels)":"Height (pixels)",height);
    auto *fit = new QComboBox; fit->addItems({"Fit inside (keep shape)","Fill and crop","Stretch","Keep original pixels (pad only)"}); fit->setCurrentIndex(3); settings->addRow("Scale mode",fit);
    auto *filter = new QComboBox; filter->addItems({"Nearest neighbour","Smooth downscale"}); settings->addRow("Resampling",filter);
    auto *remove = new QCheckBox("Remove background colour"); remove->setEnabled(kind == "sprite"); settings->addRow(remove);
    QColor key = source.pixelColor(0,0); key.setAlpha(255);
    auto *keyButton = new QPushButton(key.name()); settings->addRow("Background / key",keyButton);
    auto *tolerance = new QSpinBox; tolerance->setRange(0,255); tolerance->setValue(24); settings->addRow("Key tolerance",tolerance);
    auto *desc = new QLabel(kind == "sprite" ? "Transparency uses colour 0.\nThree opaque colours remain.\n8–32 px, in multiples of 8.\nFrames read left to right,\nthen top to bottom. Each\nframe uses 8×8 sprites." : "Four opaque colours.\n8×8 tiles in row order.\nUp to 128 unique tiles after\ndeduplication at export."); desc->setWordWrap(true); settings->addRow(desc);
    columns->addLayout(settings); auto *previewColumn = new QVBoxLayout; previewColumn->addWidget(new QLabel("CONVERTED PREVIEW"));
    auto *preview = new QLabel; preview->setMinimumSize(360,360); preview->setAlignment(Qt::AlignCenter); previewColumn->addWidget(preview,1);
    auto *info = new QLabel; info->setWordWrap(true); previewColumn->addWidget(info); columns->addLayout(previewColumn,1); root->addLayout(columns,1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); buttons->button(QDialogButtonBox::Ok)->setText(placeAfterImport?"Import && place":"Import asset"); root->addWidget(buttons);
    if(placeAfterImport)root->addWidget(new QLabel("Next: click the map to place this sprite. Escape returns to selection."));
    auto *reduction=new QGroupBox("Reduce visible colours");
    auto *reductionLayout=new QVBoxLayout(reduction);
    auto *colorInfo=new QLabel; colorInfo->setWordWrap(true); reductionLayout->addWidget(colorInfo);
    auto *paletteRow=new QHBoxLayout; QVector<QPushButton*> paletteButtons;
    const int available=kind=="sprite"?3:4;
    for(int i=0;i<available;++i){auto *b=new QPushButton;paletteButtons<<b;paletteRow->addWidget(b);}
    reductionLayout->addLayout(paletteRow);
    auto *reset=new QPushButton("Reset to automatic palette");reductionLayout->addWidget(reset);
    previewColumn->addWidget(reduction);
    QVector<quint16> customPalette;
    RasterAsset converted;
    auto update = [&] {
        ImportOptions o; o.width=width->value();o.height=height->value();o.transparent=kind=="sprite";o.fit=ImportOptions::Fit(fit->currentIndex());o.smooth=filter->currentIndex()==1;
        if(kind=="sprite"){o.columns=sheetColumns->value();o.rows=sheetRows->value();}
        o.palette=customPalette;o.removeBackground=remove->isChecked();o.background=key;o.tolerance=tolerance->value(); int colors=0; converted=convertImage(source,o,&colors);
        auto image=converted.image(); QImage board(360,360,QImage::Format_ARGB32);
        { QPainter p(&board); for(int y=0;y<360;y+=12)for(int x=0;x<360;x+=12)p.fillRect(x,y,12,12,(x/12+y/12)%2?QColor("#889099"):QColor("#bcc2c6"));
          if(!image.isNull()){auto scaled=image.scaled(336,336,Qt::KeepAspectRatio,Qt::FastTransformation);p.drawImage((360-scaled.width())/2,(360-scaled.height())/2,scaled);} }
        preview->setPixmap(QPixmap::fromImage(board));
        bool originalFits=source.width()/o.columns<=o.width && source.height()/o.rows<=o.height;
        bool valid=!image.isNull() && width->value()%8==0 && height->value()%8==0 && (o.fit!=ImportOptions::Preserve || originalFits);
        reduction->setVisible(colors>available);
        colorInfo->setText(QString("%1 visible GBC colours → %2. Click a colour to tune the result. Unused palette entries and transparent pixels do not count.").arg(colors).arg(available));
        for(int i=0;i<paletteButtons.size();++i){auto c=converted.palette.size()==4?rgb555(converted.palette[i+(o.transparent?1:0)]):QColor(Qt::black);paletteButtons[i]->setText(c.name());paletteButtons[i]->setStyleSheet(QString("background:%1;color:%2").arg(c.name(),c.lightness()<128?"white":"black"));}
        buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
        info->setText(valid ? QString("%1 × %2 px per frame · %3 frames · %4 hardware tiles per frame\nOriginal pixels are kept in pad-only mode. One shared palette; GBC RGB555 precision.").arg(o.width).arg(o.height).arg(converted.frameCount()).arg(converted.frameTileCount()) : "Choose a grid that divides the source evenly and 8-pixel frame dimensions large enough for the original pixels. Resizing requires an explicit scale mode.");
    };
    for(int i=0;i<paletteButtons.size();++i)QObject::connect(paletteButtons[i],&QPushButton::clicked,&dialog,[&,i]{
        if(converted.palette.size()!=4)return;
        auto colors=customPalette;
        if(colors.isEmpty())for(int j=0;j<available;++j)colors<<converted.palette[j+(kind=="sprite"?1:0)];
        auto c=QColorDialog::getColor(rgb555(colors[i]),&dialog,"Reduction palette colour");
        if(c.isValid()){colors[i]=toRgb555(c);customPalette=colors;update();}
    });
    QObject::connect(reset,&QPushButton::clicked,&dialog,[&]{customPalette.clear();update();});
    for(auto *s:{sheetColumns,sheetRows})QObject::connect(s,&QSpinBox::valueChanged,&dialog,[&]{
        width->setValue(qBound(8,((source.width()/sheetColumns->value()+7)/8)*8,max));
        height->setValue(qBound(8,((source.height()/sheetRows->value()+7)/8)*8,max));
    });
    for(auto *s:{width,height,tolerance,sheetColumns,sheetRows}) QObject::connect(s,&QSpinBox::valueChanged,&dialog,update);
    for(auto *c:{fit,filter}) QObject::connect(c,&QComboBox::currentIndexChanged,&dialog,update);
    QObject::connect(remove,&QCheckBox::toggled,&dialog,update);
    QObject::connect(keyButton,&QPushButton::clicked,&dialog,[&]{auto c=QColorDialog::getColor(key,&dialog,"Background colour");if(c.isValid()){key=c;keyButton->setText(c.name());update();}});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    update();if(dialog.exec()!=QDialog::Accepted)return false;
    converted.id="asset_"+QUuid::createUuid().toString(QUuid::Id128);converted.name=name->text().trimmed().isEmpty()?"Imported "+kind:name->text().trimmed();converted.kind=kind;result=converted;return true;
}
}
