#include "menu_model.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <cstring>
namespace cf {
int Project::textIndex(const QString &id)const{for(int i=0;i<textFields.size();++i)if(textFields[i].id==id)return i;return -1;}
int Project::keyboardIndex(const QString &id)const{for(int i=0;i<keyboards.size();++i)if(keyboards[i].id==id)return i;return -1;}
QJsonObject Project::menuJson()const{
 QJsonArray texts,keys;for(auto &t:textFields)texts<<QJsonObject{{"id",t.id},{"initial",t.initial},{"limit",t.limit}};
 for(auto &k:keyboards)keys<<QJsonObject{{"id",k.id},{"name",k.name},{"characters",k.characters},{"columns",k.columns},{"x",k.x},{"y",k.y}};
 auto &c=saveConfig;return {{"textFields",texts},{"keyboards",keys},{"pauseMenu",pauseMenu},{"save",QJsonObject{{"enabled",c.enabled},{"id",c.id},{"slots",c.slotCount},{"variables",c.variables},{"text",c.text},{"storages",c.storages},{"timers",c.timers},{"random",c.random},{"position",c.position},{"excludedVariables",QJsonArray::fromStringList(c.excludedVariables)},{"excludedText",QJsonArray::fromStringList(c.excludedText)},{"excludedStorages",QJsonArray::fromStringList(c.excludedStorages)}}}};
}
bool Project::readMenus(const QJsonObject &o,Project &p,QString &error){
 auto fail=[&]{error="Invalid menu, keyboard, text field or save configuration.";return false;};
 auto integer=[](QJsonValue v,int low,int high){return v.isDouble()&&v.toInt(low-1)==v.toDouble()&&v.toInt()>=low&&v.toInt()<=high;};
 p.textFields.clear();p.keyboards.clear();p.saveConfig={};p.pauseMenu.clear();
 if(o.contains("pauseMenu")){if(!o["pauseMenu"].isString())return fail();p.pauseMenu=o["pauseMenu"].toString();}
 if(o.contains("textFields")){if(!o["textFields"].isArray()||o["textFields"].toArray().size()>8)return fail();for(auto v:o["textFields"].toArray()){auto t=v.toObject();if(!t["id"].isString()||!t["initial"].isString()||!integer(t["limit"],1,20))return fail();p.textFields<<TextField{t["id"].toString(),t["initial"].toString(),t["limit"].toInt()};}}
 if(o.contains("keyboards")){if(!o["keyboards"].isArray()||o["keyboards"].toArray().size()>8)return fail();for(auto v:o["keyboards"].toArray()){auto k=v.toObject();if(!k["id"].isString()||!k["name"].isString()||!k["characters"].isString()||!integer(k["columns"],1,10)||!integer(k["x"],0,18)||!integer(k["y"],5,14))return fail();p.keyboards<<Keyboard{k["id"].toString(),k["name"].toString(),k["characters"].toString(),k["columns"].toInt(),k["x"].toInt(),k["y"].toInt()};}}
 if(o.contains("save")){if(!o["save"].isObject())return fail();auto v=o["save"].toObject();auto &c=p.saveConfig;if(!v["id"].isString()||!integer(v["slots"],1,3))return fail();c.id=v["id"].toString();c.slotCount=v["slots"].toInt();
 auto flag=[&](const char *name,bool &b){if(!v[name].isBool())return false;b=v[name].toBool();return true;};
 if(!flag("enabled",c.enabled)||!flag("variables",c.variables)||!flag("text",c.text)||!flag("storages",c.storages)||!flag("timers",c.timers)||!flag("random",c.random)||!flag("position",c.position))return fail();
 auto list=[&](const char *name,QStringList &out){if(!v[name].isArray())return false;for(auto e:v[name].toArray()){if(!e.isString())return false;out<<e.toString();}return true;};
 if(!list("excludedVariables",c.excludedVariables)||!list("excludedText",c.excludedText)||!list("excludedStorages",c.excludedStorages))return fail();}
 return true;
}
CFSaveDef saveDefinition(const Project &p){CFSaveDef d{};if(!p.saveConfig.enabled)return d;auto &c=p.saveConfig;d.slot_count=c.slotCount;
 for(int i=0;i<p.variables.size()&&i<31;++i)if(c.variables&&!c.excludedVariables.contains(p.variables[i].id))d.variables|=quint32(1)<<i;
 for(int i=0;i<p.textFields.size()&&i<8;++i)if(c.text&&!c.excludedText.contains(p.textFields[i].id))d.texts|=1<<i;
 for(int i=0;i<p.storages.size()&&i<8;++i)if(c.storages&&!c.excludedStorages.contains(p.storages[i].id))d.storages|=1<<i;
 d.flags=(c.timers?CF_SAVE_TIMERS:0)|(c.random?CF_SAVE_RANDOM:0)|(c.position?CF_SAVE_POSITION:0);
 QJsonArray schema;schema<<c.id<<int(d.variables)<<int(d.texts)<<int(d.storages)<<int(d.flags);
 for(auto &v:p.variables)schema<<v.id;for(auto &t:p.textFields)schema<<QJsonArray{t.id,t.limit};for(auto &i:p.items)schema<<QJsonArray{i.id,i.stackLimit};for(auto &s:p.storages)schema<<QJsonArray{s.id,s.capacity};for(auto &t:p.timers)schema<<QJsonArray{t.id,t.interval};for(auto &s:p.scenes)schema<<QJsonArray{s.id,s.kind};for(auto &a:p.runtimeAreas())schema<<QJsonArray{a.id,a.width,a.height};
 auto hash=QCryptographicHash::hash(QJsonDocument(schema).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);for(int i=0;i<4;++i)d.schema|=quint32(quint8(hash[i]))<<(i*8);return d;
}
void fillWorstText(const Project &p,CFGameState &state){state.save_status=5;for(auto &v:state.variables)v=-32768;for(int i=0;i<p.textFields.size()&&i<8;++i){int limit=qBound(1,p.textFields[i].limit,20);std::memset(state.texts[i],'W',limit);state.texts[i][limit]=0;}}
QStringList Project::validateMenus()const{
 QStringList errors;QSet<QString> ids;QRegularExpression pattern("^[A-Za-z_][A-Za-z0-9_]{0,31}$");for(auto &v:variables)ids.insert(v.id);ids.insert("result");ids.insert("save_status");
 if(textFields.size()>8||keyboards.size()>8)errors<<"Use up to eight text fields and eight keyboards.";
 auto ascii=[](QString s){for(auto c:s)if(c.unicode()<32||c.unicode()>126)return false;return true;};
 for(auto &t:textFields){if(!pattern.match(t.id).hasMatch()||ids.contains(t.id))errors<<"Text fields need unique IDs, distinct from numeric variables.";ids.insert(t.id);if(t.limit<1||t.limit>20||t.initial.size()>t.limit||!ascii(t.initial))errors<<t.id+": text fields use 1–20 printable ASCII characters.";}
 ids.clear();for(auto &k:keyboards){if(!pattern.match(k.id).hasMatch()||ids.contains(k.id))errors<<"Keyboards need unique IDs.";ids.insert(k.id);QSet<QChar> chars;for(auto c:k.characters)chars.insert(c);if(k.name.isEmpty()||k.name.size()>18||!ascii(k.name)||k.characters.isEmpty()||k.characters.size()>80||chars.size()!=k.characters.size()||!ascii(k.characters)||k.columns<1||k.columns>10||k.x<0||k.x+k.columns*2>20||k.y<5||k.y+(k.characters.size()+3+qMax(1,k.columns)-1)/qMax(1,k.columns)>16)errors<<k.id+": keyboard needs a printable title, 1–80 unique ASCII keys, and a grid fitting above row 16.";}
 if(!pauseMenu.isEmpty()&&(sceneIndex(pauseMenu)<0||scenes[sceneIndex(pauseMenu)].kind!="screen"))errors<<"Pause menu must reference a screen scene.";
 if(saveConfig.enabled){if(saveConfig.id.isEmpty())errors<<"Save builder: create a save identity.";if(saveConfig.slotCount<1||saveConfig.slotCount>3)errors<<"Save builder supports 1–3 journalled slots.";}
 return errors;
}
void generateSaveScripts(Project &p){auto &c=p.saveConfig;c.enabled=true;if(c.id.isEmpty())c.id=QUuid::createUuid().toString(QUuid::WithoutBraces);for(int i=1;i<=c.slotCount;++i)for(auto op:{QString("write"),QString("read"),QString("check"),QString("erase")}){QString id="save_"+op+"_"+QString::number(i);bool exists=false;for(auto &s:p.scripts)if(s.id==id)exists=true;if(!exists&&p.scripts.size()<32)p.scripts<<GameScript{id,id,"# Generated by Save builder\nsave "+op+" "+QString::number(i)+"\n"};}}
bool addMenuPreset(Project &project,const QString &preset,QString &selectedScene,QString &error){
 auto p=project;auto field=[&](QString id,int limit){if(p.textIndex(id)>=0)return true;if(p.variableIndex(id)>=0||p.textFields.size()>=8)return false;p.textFields<<TextField{id,"",limit};return true;};
 auto keyboard=[&](QString id,bool digits){if(p.keyboardIndex(id)>=0)return true;if(p.keyboards.size()>=8)return false;Keyboard k;k.id=id;if(digits){k.name="Enter number";k.characters="0123456789";k.columns=5;}p.keyboards<<k;return true;};
 if(!field("player_name",12)||!field("save_name",12)||!keyboard("name_keyboard",false)){error="The presets need free text/keyboard entries and IDs player_name, save_name.";return false;}
 if(preset=="keyboard"||preset=="digits"){if(preset=="digits"&&!keyboard("number_keyboard",true)){error="Keyboard limit reached.";return false;}project=p;selectedScene.clear();return true;}
 if(!p.saveConfig.enabled&&p.scripts.size()+12>32){error="Leave room for 12 generated save scripts.";return false;}generateSaveScripts(p);
 auto make=[&](QString id,QString name){Scene s{id,name};s.onBack="return";ScreenElement panel;panel.id="frame";panel.type="panel";panel.name="Frame";panel.rect={0,0,20,18};s.elements<<panel;ScreenElement title;title.id="title";title.name="Title";title.text=name;title.rect={2,1,16,1};s.elements<<title;return s;};
 auto button=[](Scene &s,QString id,QString title,int row,QString action){ScreenElement e;e.id=id;e.name=title;e.type="button";e.text=title;e.rect={2,row,16,1};e.actions=action;s.elements<<e;};
 auto insert=[&](Scene s){if(p.sceneIndex(s.id)>=0)return true;if(p.scenes.size()>=32)return false;p.scenes<<s;return true;};
 for(auto op:{QString("write"),QString("read")}){auto s=make(op=="write"?"save_menu":"load_menu",op=="write"?"Save game":"Load game");
  if(op=="write")button(s,"rename","Name this save",3,"keyboard name_keyboard save_name 12");
  for(int i=1;i<=p.saveConfig.slotCount;++i)button(s,"slot_"+QString::number(i),"Slot "+QString::number(i),5+(i-1)*2,"call save_"+op+"_"+QString::number(i)+ (op=="write"?"\nif result == 1\nreturn\nend":""));
  ScreenElement label;label.id="save_name";label.name="Save name";label.text="{save_name}";label.rect={2,12,16,1};s.elements<<label;label.id="status";label.name="Save status";label.text="{save_status}";label.rect={1,14,18,1};s.elements<<label;button(s,"back","Back",15,"return");if(!insert(s)){error="Scene limit reached.";return false;}}
 if(preset=="pause"){auto s=make("pause_menu","Paused");button(s,"resume","Resume",4,"return");button(s,"save","Save game",6,"menu save_menu");button(s,"load","Load game",8,"menu load_menu");button(s,"name","Name character",10,"keyboard name_keyboard player_name 12");button(s,"save_name","Name this save",12,"keyboard name_keyboard save_name 12");if(!insert(s)){error="Scene limit reached.";return false;}p.pauseMenu=s.id;selectedScene=s.id;}
 else selectedScene=preset=="load"?"load_menu":"save_menu";
 auto errors=p.validateMenus();if(!errors.isEmpty()){error=errors.join('\n');return false;}project=p;return true;
}
Project Project::menusExample(){auto p=overworldExample();p.title="Names & Saves";QString scene,error;addMenuPreset(p,"pause",scene,error);p.saveConfig.id="cartfoundry_menu_example_v1";p.textFields[p.textIndex("save_name")].initial="Adventure";p.textFields<<TextField{"default_name","Player",12};Prompt intro;intro.id="name_intro";intro.title="Welcome";intro.text="Hello {player_name}!\nStart: pause menu.";intro.textbox={0,8,20,10};intro.onEnter="keyboard name_keyboard player_name 12\nif result == 0\ntext copy player_name default_name\nend";intro.choices={{"Continue","welcome","","resume"}};p.prompts<<intro;p.scenes[p.sceneIndex(p.defaultScene)].onSceneEnter="dialogue name_intro";return p;}

}
