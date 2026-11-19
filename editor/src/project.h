#pragma once
#include <QJsonObject>
#include <QMap>
#include <QRect>
#include "assets.h"
#include "behavior.h"
#include <QPointF>
#include <QStringList>
#include <QVector>

namespace cf {
struct Choice { QString text; QString target; QString condition,actions; };
struct Prompt {
    QString id, title, text;
    QPointF position;
    QVector<Choice> choices;
    QRect textbox{0, 0, 20, 18};
    QString onEnter,portrait;bool modal=false;
    bool border=true,showTitle=true,showFooter=true;int padding=1,choiceSpacing=1,textAlign=0;QString cursor=">",disabledMarker="x",footer;QVector<quint16> textPalette;
};
struct Actor {
    QString id,name,sprite,script;
    int x=4,y=4;
    int health=3;QString onInteract,onHit,onDefeat,libraryId;
    QJsonObject toJson() const;
};
struct TextField {QString id,initial;int limit=12;};
struct Keyboard {QString id,name="Enter name",characters="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -";int columns=10,x=0,y=6;};
struct SaveConfig {
 bool enabled=false,variables=true,text=true,storages=true,timers=true,random=true,position=true;
 int slotCount=3;QString id;QStringList excludedVariables,excludedText,excludedStorages;
};
struct Variable {QString id;int initial=0;};
struct Item {QString id,name,description,icon;int stackLimit=10;bool consumable=false;QString useScript;};
struct Storage {QString id,name;int capacity=8;QMap<QString,int> contents;};
struct GameScript {QString id,name,source;};
struct SoundNote {int note=60,ticks=15,volume=-1,noise=-1;};
struct SoundTrack {
 bool enabled=true,dac=true,envelopeUp=false,sweepDown=false,noiseNarrow=false,drumKit=true;
 int duty=2,volume=10,pan=3,envelopePace=0,sweepPace=0,sweepShift=0,length=0,waveLevel=1,noiseShift=4,noiseDivider=3;
 QVector<int> wave{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
 QVector<SoundNote> notes{{0,60}};
};
struct SoundClip {QString id,name;int bpm=120,duty=2,volume=10;QVector<SoundNote> notes{{60,15},{64,15},{67,15},{0,15}};QVector<SoundTrack> channels;bool effect=false;int masterLeft=7,masterRight=7;};
QJsonObject soundJson(const SoundClip &);
bool readSound(const QJsonObject &,SoundClip &,QString &);
QStringList validateSound(const SoundClip &);
void expandSoundChannels(SoundClip &);
SoundClip foundryDemoSong();
struct Timer {QString id,name,source;int interval=60;bool repeat=false,autostart=false,pauseDialogue=true;};
struct NpcTemplate {QString id,name;Actor actor;};
struct ScreenElement {QString id,name,type="text",text,image,condition,actions;QRect rect{2,2,16,2};int align=0;};
struct Scene {QString id,name;QString kind="screen",startArea,onSceneEnter,onBack;QVector<ScreenElement> elements;quint16 background=quint16(29|(29<<5)|(25<<10));};
struct MapGroup {QString id,name,scene;};
struct WorldObject {QString id,name,kind="storage",storage,onInteract,onHit;int x=4,y=4,tile=0;bool solid=true;};
struct TerrainEvent {int tile=0,every=1,chance=25;QString script;};
struct Controls { int interact=16,close=32,restart=128,inventory=64; };
struct World {
    QString id = "area_start", name = "Starting Area", chapter = "Chapter 1";
    QMap<int, QString> exits;
    QString mapId="map_main",onStart;
    QVector<WorldObject> objects;
    QVector<TerrainEvent> terrainEvents;
    int interactionRange=16,attackRange=24;
    QString playerScript = "behavior top_down\nspeed 1\nevery 1\n";
    QVector<Actor> actors;
    bool enabled = false;
    int width = 40, height = 32, spawnX = 4, spawnY = 4;
    QString tileset, player;
    QVector<int> tiles = QVector<int>(40 * 32, 0);
    QByteArray collision = QByteArray(40 * 32, 0);
    QMap<int, QString> interactions;
    QByteArray effectiveCollision() const;
    QVector<int> effectiveTiles() const;
    void resize(int w, int h);
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &, World &, QString &);
};
struct TextLayout { int columns, bodyRows, choiceRow, footerRow,bodyRow; };
QString buttonName(int key);
TextLayout textLayout(const Prompt &);
struct Project {
    QString title = "First Conversation";
    QString target = "gbc";
    Controls controls;
    QVector<Variable> variables;
    QVector<TextField> textFields;QVector<Keyboard> keyboards;SaveConfig saveConfig;QString pauseMenu;
    int textIndex(const QString &)const;int keyboardIndex(const QString &)const;
    QJsonObject menuJson()const;
    static bool readMenus(const QJsonObject &,Project &,QString &);
    QStringList validateMenus()const;
    QVector<Item> items;
    QVector<Storage> storages{{"inventory","Inventory",8,{}}};
    QVector<GameScript> scripts;
    QVector<SoundClip> sounds;
    QVector<Timer> timers;int rngSeed=44257;
    QVector<NpcTemplate> npcs;
    QVector<Scene> scenes{{"scene_main","Main Scene"}};
    QVector<MapGroup> maps{{"map_main","Overworld","scene_main"}};
    QJsonObject systemsJson() const;
    static bool readSystems(const QJsonObject &,Project &,QString &);
    QStringList validateSystems() const;
    int variableIndex(QString id) const;
    int itemIndex(const QString &) const;
    int storageIndex(const QString &) const;
    QString defaultScene="scene_main";int splashTicks=120;
    QString entry;
    QVector<Prompt> prompts;
    QVector<RasterAsset> assets;
    World world;
    QVector<World> areas; // additional areas; world is the starting area
    QVector<World> allAreas() const;
    QVector<World> runtimeAreas() const;
    QString areaScene(const QString &) const;
    int sceneIndex(const QString &) const;
    bool hasWorldScenes() const;
    void migrateScenes();
    Project isolatedArea(int index) const;
    Project isolatedPrompt(const QString &id) const;
    const RasterAsset *asset(const QString &id) const;
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &, Project &, QString &error);
    static bool load(const QString &, Project &, QString &error);
    bool save(const QString &, QString &error) const;
    QStringList validate() const;
    int indexOf(const QString &) const;
    static Project blank();
    static Project scenesExample();
    static Project menusExample();
    static Project example();
    static Project overworldExample();
    static Project pokemonExample();
};
QStringList wrapText(const QString &, int columns = 18);
QStringList screenLines(const Project &, int prompt, int choice);
}
