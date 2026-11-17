#include "importer.h"
#include <QtWidgets>
#include <QImageReader>
namespace cf {
bool importAsset(QWidget *parent, const QString &kind, RasterAsset &result) {
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
    width->setValue(kind == "sprite" ? 16 : qBound(8,(source.width()/8)*8,128));
    height->setValue(kind == "sprite" ? 16 : qBound(8,(source.height()/8)*8,128));
    auto *sheetColumns=new QSpinBox,*sheetRows=new QSpinBox;sheetColumns->setRange(1,8);sheetRows->setRange(1,8);
    if(kind=="sprite"){settings->addRow("Sheet width (frames)",sheetColumns);settings->addRow("Sheet height (frames)",sheetRows);}
    settings->addRow(kind=="sprite"?"Frame width (pixels)":"Width (pixels)",width);settings->addRow(kind=="sprite"?"Frame height (pixels)":"Height (pixels)",height);
    auto *fit = new QComboBox; fit->addItems({"Fit inside (keep shape)","Fill and crop","Stretch"}); settings->addRow("Scale mode",fit);
    auto *filter = new QComboBox; filter->addItems({"Nearest neighbour","Smooth downscale"}); settings->addRow("Resampling",filter);
    auto *remove = new QCheckBox("Remove background colour"); remove->setEnabled(kind == "sprite"); settings->addRow(remove);
    QColor key = source.pixelColor(0,0); key.setAlpha(255);
    auto *keyButton = new QPushButton(key.name()); settings->addRow("Background / key",keyButton);
    auto *tolerance = new QSpinBox; tolerance->setRange(0,255); tolerance->setValue(24); settings->addRow("Key tolerance",tolerance);
    auto *desc = new QLabel(kind == "sprite" ? "Transparency uses colour 0.\nThree opaque colours remain.\n8–32 px, in multiples of 8.\nFrames read left to right,\nthen top to bottom. Each\nframe uses 8×8 sprites." : "Four opaque colours.\n8×8 tiles in row order.\nUp to 128 unique tiles after\ndeduplication at export."); desc->setWordWrap(true); settings->addRow(desc);
    columns->addLayout(settings); auto *previewColumn = new QVBoxLayout; previewColumn->addWidget(new QLabel("CONVERTED PREVIEW"));
    auto *preview = new QLabel; preview->setMinimumSize(360,360); preview->setAlignment(Qt::AlignCenter); previewColumn->addWidget(preview,1);
    auto *info = new QLabel; info->setWordWrap(true); previewColumn->addWidget(info); columns->addLayout(previewColumn,1); root->addLayout(columns,1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); buttons->button(QDialogButtonBox::Ok)->setText("Import asset"); root->addWidget(buttons);
    RasterAsset converted;
    auto update = [&] {
        ImportOptions o; o.width=width->value();o.height=height->value();o.transparent=kind=="sprite";o.fit=ImportOptions::Fit(fit->currentIndex());o.smooth=filter->currentIndex()==1;
        if(kind=="sprite"){o.columns=sheetColumns->value();o.rows=sheetRows->value();}
        o.removeBackground=remove->isChecked();o.background=key;o.tolerance=tolerance->value(); converted=convertImage(source,o);
        auto image=converted.image(); QImage board(360,360,QImage::Format_ARGB32);
        { QPainter p(&board); for(int y=0;y<360;y+=12)for(int x=0;x<360;x+=12)p.fillRect(x,y,12,12,(x/12+y/12)%2?QColor("#889099"):QColor("#bcc2c6"));
          if(!image.isNull()){auto scaled=image.scaled(336,336,Qt::KeepAspectRatio,Qt::FastTransformation);p.drawImage((360-scaled.width())/2,(360-scaled.height())/2,scaled);} }
        preview->setPixmap(QPixmap::fromImage(board));
        bool valid=!image.isNull() && width->value()%8==0 && height->value()%8==0;
        buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
        info->setText(valid ? QString("%1 × %2 px per frame · %3 frames · %4 hardware tiles per frame\nSource grid must divide the image evenly. One shared four-colour palette.").arg(o.width).arg(o.height).arg(converted.frameCount()).arg(converted.frameTileCount()) : "Use 8-pixel frame dimensions and a grid that divides the source image evenly.");
    };
    for(auto *s:{width,height,tolerance,sheetColumns,sheetRows}) QObject::connect(s,&QSpinBox::valueChanged,&dialog,update);
    for(auto *c:{fit,filter}) QObject::connect(c,&QComboBox::currentIndexChanged,&dialog,update);
    QObject::connect(remove,&QCheckBox::toggled,&dialog,update);
    QObject::connect(keyButton,&QPushButton::clicked,&dialog,[&]{auto c=QColorDialog::getColor(key,&dialog,"Background colour");if(c.isValid()){key=c;keyButton->setText(c.name());update();}});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    update();if(dialog.exec()!=QDialog::Accepted)return false;
    converted.id="asset_"+QUuid::createUuid().toString(QUuid::Id128);converted.name=name->text().trimmed().isEmpty()?"Imported "+kind:name->text().trimmed();converted.kind=kind;result=converted;return true;
}
}
