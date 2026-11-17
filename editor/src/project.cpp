#include "project.h"
#include "game_script.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <cmath>

namespace cf {
QStringList wrapText(const QString &text, int columns) {
    QStringList lines;
    for (const QString &paragraph : text.split('\n')) {
        QString line;
        for (const QString &word : paragraph.split(QRegularExpression("[ \\t]+"), Qt::SkipEmptyParts)) {
            if (!line.isEmpty() && line.size() + 1 + word.size() > columns) { lines << line; line.clear(); }
            if (!line.isEmpty()) line += ' ';
            line += word;
        }
        lines << line;
    }
    return lines;
}
int Project::indexOf(const QString &id) const {
    for (int i = 0; i < prompts.size(); ++i) if (prompts[i].id == id) return i;
    return -1;
}
QString buttonName(int key){return key==16?"A":key==32?"B":key==64?"Select":"Start";}
TextLayout textLayout(const Prompt &p) {
    CFPrompt code{};code.box_w=p.textbox.width();code.box_h=p.textbox.height();code.choice_count=p.choices.size();code.padding=p.padding;code.choice_spacing=p.choiceSpacing;code.text_flags=(p.border?1:0)|(p.showTitle?2:0)|(p.showFooter?4:0);CFTextLayout layout;cf_text_layout(&code,&layout);return {layout.columns,layout.body_rows,p.textbox.y()+layout.choice_row,p.textbox.y()+layout.footer_row,p.textbox.y()+layout.body_row};
}
const RasterAsset *Project::asset(const QString &id) const {
    for (const auto &a : assets) if (a.id == id) return &a;
    return nullptr;
}
QJsonObject Project::toJson() const {
    QJsonArray nodes;
    for (const auto &p : prompts) {
        QJsonArray choices;
        for (const auto &c : p.choices) choices.append(QJsonObject{{"text", c.text}, {"target", c.target},{"condition",c.condition},{"actions",c.actions}});
        QJsonArray palette;for(auto c:p.textPalette)palette.append(c);QJsonObject style{{"border",p.border},{"showTitle",p.showTitle},{"showFooter",p.showFooter},{"padding",p.padding},{"choiceSpacing",p.choiceSpacing},{"align",p.textAlign},{"cursor",p.cursor},{"disabledMarker",p.disabledMarker},{"footer",p.footer},{"palette",palette}};
        nodes.append(QJsonObject{{"textStyle",style},{"portrait",p.portrait},{"modal",p.modal},{"onEnter",p.onEnter},{"id", p.id}, {"title", p.title}, {"text", p.text},
            {"position", QJsonArray{p.position.x(), p.position.y()}}, {"choices", choices}, {"textbox", QJsonArray{p.textbox.x(), p.textbox.y(), p.textbox.width(), p.textbox.height()}}});
    }
    QJsonArray areaData; for(const auto &a:areas)areaData.append(a.toJson());
    QJsonArray rasters; for (const auto &a : assets) rasters.append(a.toJson());
    return {{"systems",systemsJson()},{"controls",QJsonObject{{"interact",controls.interact},{"close",controls.close},{"restart",controls.restart},{"inventory",controls.inventory}}},{"areas",areaData},{"assets", rasters}, {"world", world.toJson()}, {"format", "cartfoundry"}, {"version", 5}, {"target", target}, {"title", title}, {"entry", entry}, {"prompts", nodes}};
}
bool Project::fromJson(const QJsonObject &json, Project &result, QString &error) {
    auto fail = [&](const QString &s) { error = s; return false; };
    if (json["format"] != "cartfoundry" || (json["version"] != 1 && json["version"] != 2 && json["version"] != 3 && json["version"] != 4 && json["version"] != 5) || (json["target"] != "gbc" && json["target"] != "gb"))
        return fail("Expected a CartFoundry version 1, 2, 3, 4 or 5 Game Boy / Color project.");
    if (!json["title"].isString() || !json["entry"].isString() || !json["prompts"].isArray())
        return fail("Project requires title, entry and a prompts array.");
    Project p; p.target = json["target"].toString(); p.title = json["title"].toString(); p.entry = json["entry"].toString();
    const auto nodes = json["prompts"].toArray();
    if (nodes.isEmpty() || nodes.size() > 64) return fail("Projects support 1 to 64 prompts.");
    QSet<QString> ids;
    for (const auto &value : nodes) {
        if (!value.isObject()) return fail("Each prompt must be an object.");
        const auto o = value.toObject();
        for (const auto &key : {"id", "title", "text"})
            if (!o[key].isString()) return fail(QString("Prompt requires a string: %1").arg(key));
        Prompt n;if(o.contains("portrait")&&!o["portrait"].isString())return fail("Portrait must be an asset ID.");if(o.contains("modal")&&!o["modal"].isBool())return fail("Modal must be true or false.");n.portrait=o["portrait"].toString();n.modal=o["modal"].toBool();if(o.contains("onEnter")&&!o["onEnter"].isString())return fail("Prompt enter script must be text.");n.onEnter=o["onEnter"].toString(); n.id = o["id"].toString(); n.title = o["title"].toString(); n.text = o["text"].toString();
        if (n.id.isEmpty() || ids.contains(n.id)) return fail("Prompt IDs must be nonempty and unique.");
        ids.insert(n.id);
        auto xy = o["position"].toArray();
        if (xy.size() != 2 || !xy[0].isDouble() || !xy[1].isDouble() ||
            !std::isfinite(xy[0].toDouble()) || !std::isfinite(xy[1].toDouble()) ||
            std::abs(xy[0].toDouble()) > 100000 || std::abs(xy[1].toDouble()) > 100000)
            return fail("Prompt position must contain two coordinates within +/-100000.");
        n.position = {xy[0].toDouble(), xy[1].toDouble()};
        if (!o["choices"].isArray() || o["choices"].toArray().size() > 3) return fail("A prompt supports at most 3 choices.");
        for (const auto &cv : o["choices"].toArray()) {
            if (!cv.isObject() || !cv.toObject()["text"].isString() || !cv.toObject()["target"].isString())
                return fail("Each choice needs text and target strings.");
            auto choice=cv.toObject();for(auto key:{"condition","actions"})if(choice.contains(key)&&!choice[key].isString())return fail("Choice conditions/actions must be text.");
            n.choices.append({choice["text"].toString(),choice["target"].toString(),choice["condition"].toString(),choice["actions"].toString()});
        }
        if (o.contains("textbox")) {
            auto r = o["textbox"].toArray(); if (r.size() != 4) return fail("Textbox needs x, y, width and height in tiles.");
            for (auto v : r) if (!v.isDouble() || v.toInt(-999) != v.toDouble()) return fail("Textbox coordinates must be integers.");
            n.textbox = QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt());
            if (n.textbox.x() < 0 || n.textbox.y() < 0 || n.textbox.width() < 6 || n.textbox.height() < 7 || n.textbox.right() >= 20 || n.textbox.bottom() >= 18)
                return fail("Textbox must fit inside the 20 x 18 screen and be at least 6 x 7 tiles.");
        }
        if(o.contains("textStyle")){if(!o["textStyle"].isObject())return fail("Text style must be an object.");auto style=o["textStyle"].toObject();for(auto key:{"border","showTitle","showFooter"})if(!style[key].isBool())return fail("Invalid text style toggles.");n.border=style["border"].toBool();n.showTitle=style["showTitle"].toBool();n.showFooter=style["showFooter"].toBool();for(auto key:{"padding","choiceSpacing","align"})if(!style[key].isDouble()||style[key].toInt()!=style[key].toDouble())return fail("Invalid text layout.");n.padding=style["padding"].toInt();n.choiceSpacing=style["choiceSpacing"].toInt();n.textAlign=style["align"].toInt();for(auto key:{"cursor","disabledMarker","footer"})if(!style[key].isString())return fail("Invalid text style labels.");n.cursor=style["cursor"].toString();n.disabledMarker=style["disabledMarker"].toString();n.footer=style["footer"].toString();if(!style["palette"].isArray())return fail("Invalid text palette.");for(auto c:style["palette"].toArray()){if(!c.isDouble()||c.toInt()!=c.toDouble()||c.toInt()<0||c.toInt()>32767)return fail("Invalid text colour.");n.textPalette<<c.toInt();}if(n.padding<0||n.padding>3||n.choiceSpacing<1||n.choiceSpacing>3||n.textAlign<0||n.textAlign>2||(!n.textPalette.isEmpty()&&n.textPalette.size()!=4))return fail("Invalid text style ranges.");}
        p.prompts.append(n);
    }
    if (json["version"].toInt() >= 2) {
        if (!json["assets"].isArray() || json["assets"].toArray().size() > 32 || !json["world"].isObject()) return fail("Version 2 needs an assets array and world object.");
        QSet<QString> assetIds;
        for (auto v : json["assets"].toArray()) {
            RasterAsset a; if (!v.isObject() || !RasterAsset::fromJson(v.toObject(), a, error)) return false;
            if (assetIds.contains(a.id)) return fail("Asset IDs must be unique.");
            assetIds.insert(a.id); p.assets << a;
        }
        if (!World::fromJson(json["world"].toObject(), p.world, error)) return false;
        if(json.contains("areas")) {
            if(!json["areas"].isArray() || json["areas"].toArray().size()>7)return fail("Use at most 8 areas.");
            QSet<QString> areaIds{p.world.id};
            for(auto value:json["areas"].toArray()){World a;if(!value.isObject()||!World::fromJson(value.toObject(),a,error))return false;if(a.id.isEmpty()||areaIds.contains(a.id))return fail("Area IDs must be unique.");areaIds.insert(a.id);p.areas<<a;}
        }
    }
    if(json.contains("controls")){auto c=json["controls"].toObject();for(auto key:{"interact","close","restart"}){int n=c[key].toInt(-1);if(!c[key].isDouble()||c[key].toDouble()!=n||!QList<int>{16,32,64,128}.contains(n))return fail("Controls must be A, B, Select or Start.");}p.controls={c["interact"].toInt(),c["close"].toInt(),c["restart"].toInt()};}
    if(json.contains("controls")){auto c=json["controls"].toObject();if(c.contains("inventory")){int n=c["inventory"].toInt(-1);if(!c["inventory"].isDouble()||c["inventory"].toDouble()!=n||!QList<int>{16,32,64,128}.contains(n))return fail("Invalid inventory button.");p.controls.inventory=n;}else for(int n:{64,128,32,16})if(n!=p.controls.interact&&n!=p.controls.close&&n!=p.controls.restart){p.controls.inventory=n;break;}}
    if(json.contains("systems")){if(!json["systems"].isObject()||!readSystems(json["systems"].toObject(),p,error))return false;}
    // Incomplete text and wires are editable; export performs the full validation.
    result = p; error.clear(); return true;
}
bool Project::load(const QString &path, Project &p, QString &error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { error = f.errorString(); return false; }
    if (f.size() > 4 * 1024 * 1024) { error = "Project is larger than the 4 MiB format limit."; return false; }
    QJsonParseError pe;
    auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) { error = "Invalid project JSON: " + pe.errorString(); return false; }
    return fromJson(doc.object(), p, error);
}
bool Project::save(const QString &path, QString &error) const {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { error = f.errorString(); return false; }
    auto bytes = QJsonDocument(toJson()).toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size() || !f.commit()) { error = f.errorString(); return false; }
    return true;
}
static bool ascii(const QString &s, bool multiline = false) {
    for (const auto c : s) if ((c.unicode() < 32 || c.unicode() > 126) && !(multiline && c == '\n')) return false;
    return true;
}
QStringList Project::validate() const {
    QStringList errors;
    QSet<int> buttons{controls.interact,controls.close,controls.restart,controls.inventory};
    if(buttons.size()!=4)errors<<"Interact, close, restart and inventory controls must use different buttons.";
    for(auto n:buttons)if(!QList<int>{16,32,64,128}.contains(n))errors<<"Invalid game control button.";
    if (target != "gb" && target != "gbc") errors << "Choose Game Boy or Game Boy Color mode.";
    if (prompts.isEmpty() || prompts.size() > 64) errors << "Use 1 to 64 prompts.";
    if (title.trimmed().isEmpty() || title.size() > 18 || !ascii(title)) errors << "Project title: use 1-18 printable ASCII characters.";
    if (entry.isEmpty() || indexOf(entry) < 0) errors << "Choose a valid starting prompt.";
    QSet<QString> ids;
    int bytes = 0;
    for (const auto &p : prompts) {
        if (p.id.isEmpty() || ids.contains(p.id)) errors << "Prompt IDs must be unique and nonempty.";
        ids.insert(p.id);
        const QString prefix = p.title.isEmpty() ? p.id : p.title;
        if ((p.showTitle&&(p.title.trimmed().isEmpty()||p.title.size()>textLayout(p).columns)) || !ascii(p.title)) errors << prefix + ": title must fit the textbox width and use printable ASCII.";
        if (!ascii(p.text, true)) errors << prefix + ": text supports printable ASCII and newlines.";
        const auto layout = textLayout(p);
        if (p.textbox.x() < 0 || p.textbox.y() < 0 || p.textbox.width() < 6 || p.textbox.height() < 7 || p.textbox.right() >= 20 || p.textbox.bottom() >= 18)
            errors << prefix + ": textbox must fit on the 20 x 18 screen.";
        if(p.padding<0||p.padding>3||p.choiceSpacing<1||p.choiceSpacing>3||p.textAlign<0||p.textAlign>2||(p.border&&p.padding==0)||p.cursor.size()!=1||p.disabledMarker.size()!=1||!ascii(p.cursor)||!ascii(p.disabledMarker)||!ascii(p.footer)||(!p.textPalette.isEmpty()&&p.textPalette.size()!=4))errors<<prefix+": invalid text style.";
        for(auto c:p.textPalette)if(c>32767)errors<<prefix+": text colours must be RGB555.";
        if(p.showFooter&&!p.footer.isEmpty()&&p.footer.size()>layout.columns)errors<<prefix+": footer exceeds textbox width.";
        if (layout.bodyRows < 1) errors << prefix + ": textbox is too short for its choices.";
        CFGameState worst{};for(auto &v:worst.variables)v=-32768;const auto lines = renderGameText(*this,worst,p.text,qMax(1,layout.columns)).split('\n');
        if (lines.size() > layout.bodyRows) errors << prefix + QString(": text needs %1 lines; the textbox fits %2.").arg(lines.size()).arg(qMax(0, layout.bodyRows));
        for (const auto &line : lines) if (line.size() > layout.columns) { errors << prefix + ": a word exceeds the textbox width."; break; }
        if (p.choices.size() > 3) errors << prefix + ": use at most 3 choices.";
        bytes += p.title.size() + p.text.size() + 24;
        for (const auto &c : p.choices) {
            if (c.text.trimmed().isEmpty() || c.text.size() > layout.columns - 1 || !ascii(c.text)) errors << prefix + ": choice labels must fit the textbox width (including the cursor).";
            if (c.target.isEmpty() || indexOf(c.target) < 0) errors << prefix + ": connect every choice to a prompt.";
            bytes += c.text.size() + 4;
        }
    }
    QSet<QString> assetIds;int pixelBudget=0;
    if (assets.size() > 32) errors << "Use at most 32 assets.";
    for (const auto &a : assets) {
        RasterAsset parsed; QString error;
        if (!RasterAsset::fromJson(a.toJson(), parsed, error) || assetIds.contains(a.id)) { errors << "Invalid or duplicate raster asset: " + a.name; return errors; }
        assetIds.insert(a.id);pixelBudget+=a.pixels.size();
    }
    if(pixelBudget>512*1024)errors<<"Embedded image data exceeds 512 KiB. Reduce sheet sizes or remove unused assets.";
    auto maps=allAreas();QSet<QString> areaIds;
    if(maps.size()>8)errors<<"Use at most 8 areas.";
    for(const auto &map:maps){if(map.id.isEmpty()||areaIds.contains(map.id))errors<<"Area IDs must be unique.";areaIds.insert(map.id);if(map.name.trimmed().isEmpty())errors<<"Name every area.";}
    for(const auto &w:maps) if (world.enabled) {
        const auto *ts = asset(w.tileset), *sp = asset(w.player);
        if (!ts || ts->kind != "tileset") errors << "Overworld needs a tileset.";
        if (!sp || sp->kind != "sprite") errors << "Overworld needs a player sprite.";
        if (w.width < 20 || w.height < 18 || w.width > 64 || w.height > 64 || w.tiles.size() != w.width * w.height || w.collision.size() != w.width * w.height)
            errors << "Map dimensions must be 20-64 x 18-64 tiles with matching layer sizes.";
        if (w.spawnX < 0 || w.spawnY < 0 || w.spawnX >= w.width || w.spawnY >= w.height) errors << "Spawn must be inside the map.";
        else if (w.collision.size() == w.width * w.height && w.effectiveCollision()[w.spawnY * w.width + w.spawnX]) errors << "Spawn is on a solid tile.";
        for(auto i=w.exits.begin();i!=w.exits.end();++i)if(i.key()<0||i.key()>=w.width*w.height||!areaIds.contains(i.value()))errors<<w.name+": exit points to a missing area or tile.";
        if(w.exits.size()>64)errors<<"Use at most 64 exits per area.";
        bytes += w.exits.size()*4;
        if (w.interactions.size() > 64) errors << "Use at most 64 interaction tiles.";
        for (auto it = w.interactions.begin(); it != w.interactions.end(); ++it)
            if (it.key() < 0 || it.key() >= w.width * w.height || indexOf(it.value()) < 0) errors << "Map interaction points to a missing prompt or tile.";
        if (ts) {
            for (int tile : w.tiles) if (tile < 0 || tile >= ts->tileCount()) { errors << "Map contains a tile outside its tileset."; break; }
            QSet<QByteArray> unique; auto data = ts->tileData();
            for (int i = 0; i < ts->tileCount(); ++i) unique.insert(data.mid(i * 16, 16));
            if (unique.size() > 128) errors << "Tileset exceeds 128 unique 8x8 tiles after deduplication.";
            bytes += unique.size() * 16;
        }
        BehaviorSpec playerBehavior;QString scriptError;
        if(!parseBehavior(w.playerScript,playerBehavior,scriptError)||playerBehavior.code.kind!=CF_BEHAVIOR_PLAYER)errors<<w.name+": player script needs top_down behavior. "+scriptError;
        if(!playerBehavior.prompt.isEmpty())errors<<w.name+": on_touch belongs on an enemy/character script, not the player.";
        auto validateAnimations=[&](const BehaviorSpec &b,const RasterAsset *sprite,const QString &label){if(!sprite)return;for(auto name:{b.animation,b.moveAnimation,b.idleAnimation,b.actionAnimation})if(!name.isEmpty()&&sprite->animationIndex(name)<0)errors<<label+": animation not found: "+name;};
        validateAnimations(playerBehavior,sp,w.name+" player");
        int hardwareSprites=sp?sp->frameTileCount():0;QSet<QString>actorIds;
        if(w.actors.size()>4)errors<<"Use at most 4 actors per area.";
        for(const auto &actor:w.actors){
            const auto *sprite=asset(actor.sprite);BehaviorSpec behavior;
            if(actor.id.isEmpty()||actorIds.contains(actor.id))errors<<"Actor IDs must be unique within an area.";actorIds.insert(actor.id);
            if(!sprite||sprite->kind!="sprite")errors<<actor.name+": choose an actor sprite.";else{hardwareSprites+=sprite->frameTileCount();bytes+=sprite->tileCount()*16;}
            if(actor.x<0||actor.y<0||actor.x>=w.width||actor.y>=w.height)errors<<actor.name+": actor spawn is outside its area.";
            else if(w.collision.size()==w.width*w.height&&w.effectiveCollision()[actor.y*w.width+actor.x])errors<<actor.name+": actor spawn is on a solid tile.";
            if(!parseBehavior(actor.script,behavior,scriptError)||behavior.code.kind==CF_BEHAVIOR_PLAYER)errors<<actor.name+": "+(scriptError.isEmpty()?"choose an idle, patrol or chase behavior.":scriptError);
            validateAnimations(behavior,sprite,actor.name);
            if(!behavior.prompt.isEmpty()&&indexOf(behavior.prompt)<0)errors<<actor.name+": on_touch points to a missing prompt ID.";
            bytes+=32;
        }
        if(hardwareSprites>40)errors<<w.name+": player and actors exceed 40 hardware sprites. Use smaller sprites or fewer actors.";
        if (sp) bytes += sp->tileCount() * 16;
        bytes += w.width * w.height * 2 + w.interactions.size() * 4;
    }
    if (bytes > 18000) errors << "Project exceeds the prototype's 18 KB data budget (ROM banking comes later).";
    errors.append(validateSystems());
    return errors;
}
QStringList screenLines(const Project &project, int index, int selected) {
    QStringList lines; for (int i = 0; i < 18; ++i) lines << QString(20, ' ');
    if (index < 0 || index >= project.prompts.size()) return lines;
    const auto &p = project.prompts[index]; auto box = p.textbox; auto layout = textLayout(p);
    auto put = [&](int x, int y, QString text) {
        if (y < 0 || y >= 18 || x < 0 || x >= 20) return;
        text = text.left(qMin(layout.columns, 20 - x));
        for (int i = 0; i < text.size(); ++i) lines[y][x+i] = text[i];
    };
    if (box.width() < 6 || box.height() < 7) return lines;
    for (int y = qMax(0, box.y()); y <= qMin(17, box.bottom()); ++y) for (int x = qMax(0, box.x()); x <= qMin(19, box.right()); ++x) {
        bool edgeX = x == box.x() || x == box.right(), edgeY = y == box.y() || y == box.bottom();
        lines[y][x] = !p.border?' ':edgeX && edgeY ? '+' : edgeX ? '|' : edgeY ? '-' : ' ';
    }
    if(p.showTitle)put(box.x()+p.padding,box.y()+p.padding,p.title);auto body=wrapText(p.text,qMax(1,layout.columns));
    for(int i=0;i<qMin(layout.bodyRows,body.size());++i){int offset=p.textAlign==1?(layout.columns-body[i].size())/2:p.textAlign==2?layout.columns-body[i].size():0;put(box.x()+p.padding+qMax(0,offset),layout.bodyRow+i,body[i]);}
    for(int i=0;i<p.choices.size();++i)put(box.x()+p.padding,layout.choiceRow+i*p.choiceSpacing,(i==selected?p.cursor:" ")+p.choices[i].text);
    if(p.showFooter)put(box.x()+p.padding,layout.footerRow,p.footer.isEmpty()?buttonName(project.controls.interact)+":select"+(p.modal?QString():" "+buttonName(project.controls.close)+(project.world.enabled?":close":":reset")):p.footer);
    return lines;
}
Project Project::example() {
    Project p; p.entry = "welcome";
    p.prompts = {
        {"welcome", "THE WORKSHOP", "Welcome, traveller.\nA warm light glows\ninside the foundry.\nWhat will you do?", {60, 130}, {{"Enter the foundry", "foundry"}, {"Follow the path", "path"}}},
        {"foundry", "THE FOUNDRY", "The keeper smiles.\nEvery adventure\nstarts with a\nsingle choice.", {420, 50}, {{"Ask about a quest", "quest"}, {"Go outside", "welcome"}}},
        {"path", "THE OLD PATH", "You find a sign:\nTHIS WAY TO\nSOMETHING NEW.\nWhere does it lead?", {420, 390}, {{"Follow the sign", "quest"}, {"Turn back", "welcome"}}},
        {"quest", "A NEW BEGINNING", "Your story begins\nhere. Rewire these\nchoices to change\nits course.", {800, 220}, {{"Start again", "welcome"}}}
    };
    return p;
}
}
