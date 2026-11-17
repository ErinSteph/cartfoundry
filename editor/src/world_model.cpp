#include "project.h"
#include <QJsonArray>
#include <QSet>
namespace cf {
QByteArray World::effectiveCollision()const{auto data=collision;for(auto &o:objects)if(o.solid&&o.x>=0&&o.y>=0&&o.x<width&&o.y<height&&o.y*width+o.x<data.size())data[o.y*width+o.x]=1;return data;}
QVector<int> World::effectiveTiles()const{auto data=tiles;for(auto &o:objects)if(o.x>=0&&o.y>=0&&o.x<width&&o.y<height&&o.y*width+o.x<data.size())data[o.y*width+o.x]=o.tile;return data;}
void World::resize(int w, int h) {
    if (w < 20 || w > 64 || h < 18 || h > 64) return;
    QVector<int> newTiles(w * h, 0); QByteArray newCollision(w * h, 0); QMap<int,QString> newInteractions, newExits;
    for (int y = 0; y < qMin(h, height); ++y) for (int x = 0; x < qMin(w, width); ++x) {
        int a = y * width + x, b = y * w + x;
        if (a < tiles.size()) newTiles[b] = tiles[a]; if (a < collision.size()) newCollision[b] = collision[a];
        if (interactions.contains(a)) newInteractions[b] = interactions[a];
        if (exits.contains(a)) newExits[b] = exits[a];
    }
    width = w; height = h; tiles = newTiles; collision = newCollision; interactions = newInteractions; exits = newExits;
    spawnX = qBound(0, spawnX, w - 1); spawnY = qBound(0, spawnY, h - 1);
    for(auto &obj:objects){obj.x=qBound(0,obj.x,w-1);obj.y=qBound(0,obj.y,h-1);}
    for(auto &actor:actors){actor.x=qBound(0,actor.x,w-1);actor.y=qBound(0,actor.y,h-1);}
}
QJsonObject Actor::toJson() const{return {{"health",health},{"onInteract",onInteract},{"onHit",onHit},{"onDefeat",onDefeat},{"libraryId",libraryId},{"id",id},{"name",name},{"sprite",sprite},{"script",script},{"x",x},{"y",y}};}
QJsonObject World::toJson() const {
    QJsonArray cells, triggers, doors; for (auto tile : tiles) cells.append(tile);
    for (auto i = interactions.begin(); i != interactions.end(); ++i) triggers.append(QJsonObject{{"cell", i.key()}, {"prompt", i.value()}});
    for (auto i = exits.begin(); i != exits.end(); ++i) doors.append(QJsonObject{{"cell",i.key()},{"area",i.value()}});
    QJsonArray terrain;for(auto &e:terrainEvents)terrain.append(QJsonObject{{"tile",e.tile},{"every",e.every},{"chance",e.chance},{"script",e.script}});
    QJsonArray objectsData;for(auto &o:objects)objectsData.append(QJsonObject{{"id",o.id},{"name",o.name},{"kind",o.kind},{"storage",o.storage},{"onInteract",o.onInteract},{"onHit",o.onHit},{"x",o.x},{"y",o.y},{"tile",o.tile},{"solid",o.solid}});
    QJsonArray entities;for(auto &a:actors)entities.append(a.toJson());
    return {{"terrainEvents",terrain},{"objects",objectsData},{"mapId",mapId},{"onStart",onStart},{"interactionRange",interactionRange},{"attackRange",attackRange},{"playerScript",playerScript},{"actors",entities},{"id",id},{"name",name},{"chapter",chapter},{"exits",doors},{"enabled", enabled}, {"width", width}, {"height", height}, {"spawnX", spawnX}, {"spawnY", spawnY}, {"tileset", tileset}, {"player", player}, {"tiles", cells}, {"collision", QString::fromLatin1(collision.toBase64())}, {"interactions", triggers}};
}
bool World::fromJson(const QJsonObject &o, World &out, QString &error) {
    auto fail = [&] { error = "Invalid overworld dimensions, layers, spawn or interaction data."; return false; };
    World w;
    for(auto key:{"id","name","chapter"}) if(o.contains(key) && !o[key].isString()) return fail();
    w.id=o["id"].toString("area_start"); w.name=o["name"].toString("Starting Area"); w.chapter=o["chapter"].toString("Chapter 1");
    auto integer = [&](const char *key, int low, int high, int &dest) { int v = o[key].toInt(-1); if (!o[key].isDouble() || o[key].toDouble() != v || v < low || v > high) return false; dest = v; return true; };
    if (!o["enabled"].isBool() || !integer("width",20,64,w.width) || !integer("height",18,64,w.height) || !integer("spawnX",0,w.width-1,w.spawnX) || !integer("spawnY",0,w.height-1,w.spawnY) || !o["tileset"].isString() || !o["player"].isString()) return fail();
    w.enabled = o["enabled"].toBool(); w.tileset = o["tileset"].toString(); w.player = o["player"].toString();
    auto cells = o["tiles"].toArray(); if (cells.size() != w.width*w.height) return fail(); w.tiles.clear();
    for (auto c : cells) { int v = c.toInt(-1); if (v < 0 || v > 255 || c.toDouble() != v) return fail(); w.tiles << v; }
    if (!o["collision"].isString()) return fail();
    auto decoded = QByteArray::fromBase64Encoding(o["collision"].toString().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.size() != w.width*w.height) return fail(); w.collision = decoded.decoded;
    for (auto v : w.collision) if (v != 0 && v != 1) return fail();
    if (!o["interactions"].isArray() || o["interactions"].toArray().size() > 64) return fail(); w.interactions.clear();
    for (auto value : o["interactions"].toArray()) {
        auto i = value.toObject(); int c = i["cell"].toInt(-1);
        if (!value.isObject() || c < 0 || c >= w.width*w.height || i["cell"].toDouble() != c || !i["prompt"].isString() || w.interactions.contains(c)) return fail();
        w.interactions[c] = i["prompt"].toString();
    }
    if(o.contains("exits")) {
        if(!o["exits"].isArray() || o["exits"].toArray().size()>64)return fail();
        for(auto value:o["exits"].toArray()){auto v=value.toObject();int c=v["cell"].toInt(-1);if(!value.isObject()||c<0||c>=w.width*w.height||v["cell"].toDouble()!=c||!v["area"].isString()||w.exits.contains(c))return fail();w.exits[c]=v["area"].toString();}
    }
    if(o.contains("playerScript")){if(!o["playerScript"].isString())return fail();w.playerScript=o["playerScript"].toString();}
    if(o.contains("actors")){
        if(!o["actors"].isArray()||o["actors"].toArray().size()>4)return fail();QSet<QString> ids;
        for(auto value:o["actors"].toArray()){auto v=value.toObject();Actor a;for(auto key:{"id","name","sprite","script"})if(!v[key].isString())return fail();
            a.id=v["id"].toString();a.name=v["name"].toString();a.sprite=v["sprite"].toString();a.script=v["script"].toString();for(auto key:{"onInteract","onHit","onDefeat","libraryId"})if(v.contains(key)&&!v[key].isString())return fail();a.onInteract=v["onInteract"].toString();a.onHit=v["onHit"].toString();a.onDefeat=v["onDefeat"].toString();a.libraryId=v["libraryId"].toString();if(v.contains("health")){a.health=v["health"].toInt(-1);if(a.health<1||a.health>255||v["health"].toDouble()!=a.health)return fail();}a.x=v["x"].toInt(-1);a.y=v["y"].toInt(-1);
            if(a.id.isEmpty()||ids.contains(a.id)||a.x<0||a.y<0||a.x>=w.width||a.y>=w.height||v["x"].toDouble()!=a.x||v["y"].toDouble()!=a.y)return fail();ids.insert(a.id);w.actors<<a;
        }
    }
    for(auto key:{"mapId","onStart"})if(o.contains(key)&&!o[key].isString())return fail();w.mapId=o["mapId"].toString("map_main");w.onStart=o["onStart"].toString();
    for(auto key:{"interactionRange","attackRange"})if(o.contains(key)&&(!o[key].isDouble()||o[key].toInt()!=o[key].toDouble()))return fail();w.interactionRange=o["interactionRange"].toInt(16);w.attackRange=o["attackRange"].toInt(24);
    if(o.contains("objects")){if(!o["objects"].isArray()||o["objects"].toArray().size()>8)return fail();QSet<QString>ids;for(auto value:o["objects"].toArray()){auto v=value.toObject();WorldObject obj;for(auto key:{"id","name","kind","storage","onInteract","onHit"})if(!v[key].isString())return fail();obj.id=v["id"].toString();obj.name=v["name"].toString();obj.kind=v["kind"].toString();obj.storage=v["storage"].toString();obj.onInteract=v["onInteract"].toString();obj.onHit=v["onHit"].toString();obj.x=v["x"].toInt(-1);obj.y=v["y"].toInt(-1);obj.tile=v["tile"].toInt(-1);if(obj.id.isEmpty()||ids.contains(obj.id)||obj.x<0||obj.x>=w.width||obj.y<0||obj.y>=w.height||obj.tile<0||obj.tile>255||!v["solid"].isBool()||v["x"].toDouble()!=obj.x||v["y"].toDouble()!=obj.y||v["tile"].toDouble()!=obj.tile)return fail();obj.solid=v["solid"].toBool();ids.insert(obj.id);w.objects<<obj;}}
    if(o.contains("terrainEvents")){if(!o["terrainEvents"].isArray()||o["terrainEvents"].toArray().size()>8)return fail();for(auto value:o["terrainEvents"].toArray()){auto v=value.toObject();TerrainEvent e;for(auto key:{"tile","every","chance"})if(!v[key].isDouble()||v[key].toInt()!=v[key].toDouble())return fail();e.tile=v["tile"].toInt();e.every=v["every"].toInt();e.chance=v["chance"].toInt();if(e.tile<0||e.tile>255||e.every<1||e.every>255||e.chance<0||e.chance>100||!v["script"].isString())return fail();e.script=v["script"].toString();w.terrainEvents<<e;}}
    out = w; return true;
}
QVector<World> Project::allAreas() const { auto result=areas;result.prepend(world);return result; }
int Project::sceneIndex(const QString &id)const{for(int i=0;i<scenes.size();++i)if(scenes[i].id==id)return i;return -1;}
QString Project::areaScene(const QString &id)const{for(auto &w:allAreas())if(w.id==id)for(auto &m:maps)if(m.id==w.mapId)return m.scene;return {};}
bool Project::hasWorldScenes()const{for(auto &s:scenes)if(s.kind=="world")return true;return false;}
QVector<World> Project::runtimeAreas()const{QVector<World> result;for(auto &w:allAreas()){int index=sceneIndex(areaScene(w.id));if(index>=0&&scenes[index].kind=="world")result<<w;}return result;}
void Project::migrateScenes(){if(scenes.isEmpty())scenes<<Scene{"scene_main","Main Scene"};defaultScene=world.enabled?areaScene(world.id):scenes[0].id;if(sceneIndex(defaultScene)<0)defaultScene=scenes[0].id;for(auto &s:scenes){s.kind="screen";s.onSceneEnter.clear();s.startArea.clear();if(world.enabled)for(auto &w:allAreas())if(areaScene(w.id)==s.id){s.kind="world";s.startArea=w.id;break;}}if(!world.enabled&&indexOf(entry)>=0)scenes[sceneIndex(defaultScene)].onSceneEnter="dialogue "+entry;else if(!world.onStart.trimmed().isEmpty()){scenes[sceneIndex(defaultScene)].onSceneEnter=world.onStart;world.onStart.clear();}}
Project Project::scenesExample(){auto p=overworldExample();p.title="Scenes & Screens";Scene title{"scene_title","Title menu"};ScreenElement panel;panel.id="panel";panel.name="Menu panel";panel.type="panel";panel.rect={1,1,18,16};title.elements<<panel;ScreenElement text;text.id="title";text.name="Game title";text.text="FOUNDRY FIELDS";text.align=1;text.rect={2,3,16,2};title.elements<<text;ScreenElement start;start.id="start";start.name="Play";start.type="button";start.text="Start game";start.rect={3,8,14,1};start.actions="scene scene_main";title.elements<<start;start.id="about";start.name="About";start.text="About";start.rect.moveTop(10);start.actions="scene scene_about";title.elements<<start;Scene about{"scene_about","About screen"};text.id="about_text";text.name="About text";text.text="Made in\nCartFoundry.\n\nPress B to return.";text.rect={1,4,18,7};text.align=1;about.elements<<text;about.onBack="scene scene_title";p.scenes.prepend(title);p.scenes<<about;p.defaultScene=title.id;p.scenes[p.sceneIndex("scene_main")].onSceneEnter="dialogue welcome";p.prompts[0].choices<<Choice{"Back to title","welcome","","scene scene_title"};return p;}
Project Project::blank(){Project p;p.title="New Game";p.entry.clear();ScreenElement title;title.id="title";title.name="Title";title.text="New Game";title.rect={2,3,16,2};title.align=1;p.scenes[0].elements<<title;return p;}
static bool hasEvents(const Project &p){if(!p.items.isEmpty()||!p.scripts.isEmpty()||!p.timers.isEmpty())return true;for(auto &n:p.prompts){if(!n.onEnter.isEmpty())return true;for(auto &c:n.choices)if(!c.actions.isEmpty())return true;}for(auto &w:p.allAreas()){if(!w.onStart.isEmpty()||!w.objects.isEmpty()||!w.terrainEvents.isEmpty())return true;for(auto &a:w.actors)if(!a.onInteract.isEmpty()||!a.onHit.isEmpty()||!a.onDefeat.isEmpty())return true;}return false;}
static QVector<Prompt> reachablePrompts(const Project &p,QStringList roots){QSet<QString> seen;for(int i=0;i<roots.size();++i){QString id=roots[i];if(seen.contains(id))continue;seen.insert(id);int n=p.indexOf(id);if(n>=0)for(auto &c:p.prompts[n].choices)if(!seen.contains(c.target))roots<<c.target;}QVector<Prompt> result;for(auto &node:p.prompts)if(seen.contains(node.id))result<<node;return result;}
Project Project::isolatedPrompt(const QString &id)const{auto p=*this;p.entry=id;QString name="test_scene";while(p.sceneIndex(name)>=0)name+="_";Scene test{name,"Conversation test"};test.onSceneEnter="dialogue "+id;p.scenes<<test;p.defaultScene=name;return p;}

Project Project::isolatedArea(int index) const {
    auto p=*this;auto maps=allAreas();if(index>=0&&index<maps.size())p.world=maps[index];p.world.enabled=true;p.world.exits.clear();p.areas.clear();
    QStringList roots=p.world.interactions.values();QSet<QString> required{p.world.tileset,p.world.player};for(auto &prompt:p.prompts)if(!prompt.portrait.isEmpty())required.insert(prompt.portrait);
    for(auto &a:p.world.actors){required.insert(a.sprite);BehaviorSpec behavior;QString error;if(parseBehavior(a.script,behavior,error)&&!behavior.prompt.isEmpty())roots<<behavior.prompt;}
    p.prompts=hasEvents(*this)?prompts:reachablePrompts(p,roots);p.npcs.clear();if(p.prompts.isEmpty()){Prompt n;n.id="area_test";n.title="AREA TEST";n.text="Explore this area.";p.prompts<<n;}p.entry=p.prompts[0].id;
    for(auto &item:p.items)if(!item.icon.isEmpty())required.insert(item.icon);
    for(auto &s:p.scenes){s.kind="screen";s.startArea.clear();s.elements.clear();s.onSceneEnter.clear();s.onBack.clear();}p.defaultScene=p.areaScene(p.world.id);if(p.sceneIndex(p.defaultScene)<0)p.defaultScene=p.scenes[0].id;auto &testScene=p.scenes[p.sceneIndex(p.defaultScene)];testScene.kind="world";testScene.startArea=p.world.id;
    QVector<RasterAsset> assets;for(auto &a:p.assets)if(required.contains(a.id))assets<<a;p.assets=assets;return p;
}
Project Project::overworldExample() {
    auto p = example(); p.title = "Foundry Fields";
    RasterAsset ts; ts.id = "field_tiles"; ts.name = "Foundry terrain"; ts.kind = "tileset"; ts.width = 32; ts.height = 8;
    ts.palette = {toRgb555(QColor("#98bc72")),toRgb555(QColor("#447457")),toRgb555(QColor("#cfbb83")),toRgb555(QColor("#31474f"))};
    ts.pixels = QByteArray(32*8, 0);
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 32; ++x) {
        int tile = x/8, px = x%8, c = 0;
        if (tile == 0) c = ((px == 1 && y == 2) || (px == 5 && y == 6)) ? 1 : 0;
        if (tile == 1) c = (y == 0 || y == 7 || px == 0 || px == 7) ? 3 : 1;
        if (tile == 2) c = y == 0 || px == 0 || (px == 5 && y == 5) ? 0 : 2;
        if (tile == 3) c = px == 1 || px == 6 || y == 1 || y == 4 ? 3 : 2;
        ts.pixels[y*32+x] = char(c);
    }
    RasterAsset sp; sp.id = "traveller"; sp.name = "Traveller"; sp.kind = "sprite"; sp.width = 16; sp.height = 16;
    sp.palette = {0,toRgb555(QColor("#f5dba8")),toRgb555(QColor("#e58557")),toRgb555(QColor("#243b53"))}; sp.pixels = QByteArray(256,0);
    for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
        int c = 0;
        if (x >= 4 && x <= 11 && y >= 1 && y <= 7) c = y < 3 ? 2 : 1;
        if (x >= 3 && x <= 12 && y >= 8 && y <= 12) c = 2;
        if (y >= 13 && y <= 14 && ((x >= 4 && x <= 6) || (x >= 9 && x <= 11))) c = 3;
        if (y == 5 && (x == 6 || x == 10)) c = 3;
        sp.pixels[y*16+x] = char(c);
    }
    for(auto &n:p.prompts)n.textbox={0,6,20,12};
    p.prompts[0].text="Welcome to the\nfoundry fields.\nWhere will you go?";
    p.prompts[2].text="A weathered sign:\nTHIS WAY TO\nSOMETHING NEW.\nFollow its arrow?";
    auto still=sp;sp.width=32;sp.height=32;sp.columns=2;sp.rows=2;sp.pixels=QByteArray(1024,0);
    for(int frame=0;frame<4;++frame)for(int y=0;y<16;++y)for(int x=0;x<16;++x){int c=quint8(still.pixels[y*16+x]);if(frame==1&&y>=13)c=((x>=3&&x<=5)||(x>=10&&x<=12))?3:0;if(frame==3&&y>=13)c=((x>=5&&x<=7)||(x>=8&&x<=10))?3:0;if(frame==2&&y>=6&&y<=8&&(x==2||x==13))c=1;sp.pixels[(frame/2*16+y)*32+frame%2*16+x]=char(c);}
    sp.animations={{"idle",0,0,30,true},{"walk",0,3,24,true},{"wave",1,3,30,false}};
    RasterAsset guard=sp;guard.id="guardian_sprite";guard.name="Guardian";guard.palette[2]=toRgb555(QColor("#7767b5"));p.assets = {ts,sp,guard}; auto &w = p.world; w.name="Foundry Fields"; w.enabled = true; w.tileset = ts.id; w.player = sp.id; w.spawnX = 6; w.spawnY = 8;w.playerScript+="animation idle\non_move walk\non_idle idle\non_interact wave\n";
    for (int y = 0; y < w.height; ++y) for (int x = 0; x < w.width; ++x) {
        int i = y*w.width+x; bool wall = x == 0 || y == 0 || x == w.width-1 || y == w.height-1 || (x >= 14 && x <= 17 && y >= 9 && y <= 16);
        w.tiles[i] = wall ? 1 : (y >= 7 && y <= 9) || (x >= 23 && x <= 25) ? 2 : 0; w.collision[i] = wall;
    }
    int sign = 7*w.width+6; w.tiles[sign] = 3; w.collision[sign] = 1; w.interactions[sign] = "welcome";
    World second=w;second.id="area_garden";second.name="Walled Garden";second.chapter="Chapter 2";second.resize(24,20);second.interactions.clear();second.exits.clear();second.spawnX=4;second.spawnY=4;
    for(int y=0;y<second.height;++y)for(int x=0;x<second.width;++x){int cell=y*second.width+x;bool wall=x==0||y==0||x==second.width-1||y==second.height-1;second.tiles[cell]=wall?1:0;second.collision[cell]=wall;}
    w.tiles[8*w.width+28]=3;w.interactions.remove(8*w.width+28);w.exits[8*w.width+28]=second.id;
    second.tiles[3*second.width+4]=3;second.exits[3*second.width+4]=w.id;second.collision[3*second.width+4]=1;Actor enemy{"patroller","Field keeper",guard.id,behaviorTemplate("patrol")+"animation walk\n",11,6};w.actors<<enemy;
    Actor chaser{"chaser","Garden guardian",guard.id,behaviorTemplate("chase"),14,10};chaser.script+="animation walk\n";chaser.script.replace("on_touch none","on_touch quest");second.actors<<chaser;
    p.variables={{"coins",0},{"guards_defeated",0}};
    p.items={{"key","Foundry key","A key from the field chest.","",1,false,"dialogue quest"},{"tonic","Tonic","Use to earn five coins.","",5,true,"add coins 5"}};
    p.storages<<Storage{"field_chest","Field chest",4,{{"key",1},{"tonic",2}}};
    p.scripts={{"guard_reward","Guard reward","add guards_defeated 1\nadd coins 3\n"}};
    w.objects<<WorldObject{"chest_fields","Field chest","storage","field_chest","","",9,10,3,true};
    w.actors[0].onInteract="dialogue welcome";w.actors[0].onDefeat="call guard_reward";
    p.npcs<<NpcTemplate{"field_keeper","Field keeper",w.actors[0]};
    p.prompts[0].choices[0].condition="coins >= 3";p.prompts[0].choices[1].actions="add coins 1";
    p.scenes<<Scene{"scene_garden","Garden"};p.maps<<MapGroup{"map_garden","Garden map","scene_garden"};second.mapId="map_garden";
    p.areas<<second;p.migrateScenes();
    return p;
}
}
