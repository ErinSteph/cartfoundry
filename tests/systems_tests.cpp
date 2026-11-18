#include "game_session.h"
#include "cf_raycast.h"
#include <QtTest>
#include <QJsonArray>
class SystemsTests:public QObject{
 Q_OBJECT
private slots:
 void soundEvents(){auto p=cf::Project::example();cf::GameSession s;s.configure(p);
 QVERIFY(s.run("sound loop pulse\nsound play blip"));
 QCOMPARE(s.state.sound.voices[0].clip,uint8_t(4));QCOMPARE(s.state.sound.voices[1].clip,uint8_t(1));
 for(int n=0;n<16;++n)cf_game_tick(&s.state,&s.definition,1);
 QCOMPARE(s.state.sound.voices[1].clip,uint8_t(0));QCOMPARE(s.state.sound.voices[0].clip,uint8_t(4));
 for(int n=16;n<80;++n)cf_game_tick(&s.state,&s.definition,0);
 QCOMPARE(s.state.sound.voices[0].step,uint8_t(0));QCOMPARE(s.state.sound.voices[0].frequency,uint16_t(131));
 QVERIFY(s.run("sound play hit"));QVERIFY(s.run("sound stop"));
 for(auto &v:s.state.sound.voices){QCOMPARE(v.clip,uint8_t(0));QCOMPARE(v.frequency,uint16_t(0));}
 QVERIFY(!s.run("sound play unknown"));QVERIFY(!s.run("sound loop"));QVERIFY(!s.run("sound stop blip"));
 QVERIFY(s.run("sound loop alarm"));s.reset();QCOMPARE(s.state.sound.voices[0].clip,uint8_t(0));
 }

 void compilerAndVariables(){auto p=cf::Project::example();p.variables={{"coins",5},{"visits",0}};p.items={{"key","Key","","",1,false,""}};p.storages<<cf::Storage{"chest","Chest",2,{{"key",1}}};p.scripts={{"reward","Reward","add coins 2\n"}};
 p.prompts[0].onEnter="add visits 1";p.prompts[0].choices[0].condition="coins >= 7";p.prompts[0].choices[1].actions="call reward";p.prompts[0].choices[1].target=p.prompts[0].id;
 cf::GameSession session;session.configure(p);CFState flow{};session.enterPrompt(flow,0);QCOMPARE(flow.choice,uint8_t(1));QCOMPARE(session.state.variables[1],1);session.step(flow,CF_KEY_ACCEPT);QCOMPARE(session.state.variables[0],7);QCOMPARE(session.state.variables[1],2);QCOMPARE(flow.choice,uint8_t(0));
 QVERIFY(session.run("if coins >= 7\n if has chest key >= 1\n move chest inventory key 1\n else\n set coins 0\n end\nelse\nset coins -1\nend"));QCOMPARE(session.state.counts[0][0],uint16_t(1));QCOMPARE(session.state.counts[1][0],uint16_t(0));
 QVERIFY(!session.run("take chest key 1"));QCOMPARE(session.state.variables[31],0);QVERIFY(session.run("if result == 0\nset coins 32767\nadd coins 20\nend"));QCOMPARE(session.state.variables[0],32767);
 cf::Project roundtrip;QString error;QVERIFY2(cf::Project::fromJson(p.toJson(),roundtrip,error),qPrintable(error));QCOMPARE(p.toJson(),roundtrip.toJson());QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));
 cf::GameCompiler compiler(p);QVector<CFInstruction> code;QVERIFY(!compiler.program("set unknown 1",code,error));QVERIFY(!compiler.program("if coins > 1\nadd coins 1",code,error));p.scripts[0].source="call reward";QVERIFY(!cf::GameCompiler(p).program("call reward",code,error));}
 void capacityAndTransfer(){auto p=cf::Project::example();p.items={{"key","Key","","",2,false,""},{"coin","Coin","","",10,false,""}};p.storages[0].capacity=1;p.storages[0].contents={{"key",2}};p.storages<<cf::Storage{"chest","Chest",2,{{"coin",5}}};cf::GameSession s;s.configure(p);QVERIFY(!cf_storage_transfer(&s.state,&s.definition,1,0,1,1));QCOMPARE(s.state.counts[1][1],uint16_t(5));QVERIFY(cf_storage_change(&s.state,&s.definition,0,0,-2));QVERIFY(cf_storage_transfer(&s.state,&s.definition,1,0,1,5));QCOMPARE(s.state.counts[0][1],uint16_t(5));QVERIFY(!cf_storage_change(&s.state,&s.definition,0,1,6));QVERIFY(!cf_storage_transfer(&s.state,&s.definition,1,0,1,1));}
 void raycast(){QByteArray solid(20*18,0);CFHitbox target{40,32,8,8,CF_HIT_ACTOR,3};CFRayHit hit;auto cast=[&]{cf_raycast(20,36,70,36,20,18,reinterpret_cast<const uint8_t *>(solid.constData()),&target,1,&hit);};cast();QCOMPARE(hit.kind,uint8_t(CF_HIT_ACTOR));QCOMPARE(hit.index,uint8_t(3));QCOMPARE(hit.distance,uint16_t(20));solid[4*20+4]=1;cast();QCOMPARE(hit.kind,uint8_t(CF_HIT_WALL));target.x=32;target.kind=CF_HIT_OBJECT;cast();QCOMPARE(hit.kind,uint8_t(CF_HIT_OBJECT));solid.fill(0);target={40,40,8,8,CF_HIT_ACTOR,0};cf_raycast(20,20,60,60,20,18,reinterpret_cast<const uint8_t *>(solid.constData()),&target,1,&hit);QCOMPARE(hit.kind,uint8_t(CF_HIT_ACTOR));}
 void migrationAndHierarchy(){QString error;cf::Project p;QVERIFY2(cf::Project::load(QString(CF_SOURCE_DIR)+"/tests/fixtures/legacy-overworld-v2.cartfoundry",p,error),qPrintable(error));QCOMPARE(p.controls.inventory,64);p.scenes<<cf::Scene{"town","Town"};p.maps<<cf::MapGroup{"market","Market","town"};p.world.mapId="market";p.scenes[0].kind="world";p.scenes[0].startArea=p.areas[0].id;p.scenes.last().kind="world";p.scenes.last().startArea=p.world.id;p.defaultScene="town";p.storages<<cf::Storage{"chest","Chest",8,{}};p.world.objects<<cf::WorldObject{"chest_obj","Chest","storage","chest","","",8,8,3,true};QCOMPARE(p.world.effectiveCollision()[8*p.world.width+8],char(1));QCOMPARE(p.world.effectiveTiles()[8*p.world.width+8],3);cf::Project q;QVERIFY(cf::Project::fromJson(p.toJson(),q,error));QCOMPARE(q.toJson(),p.toJson());QVERIFY2(q.validate().isEmpty(),qPrintable(q.validate().join('\n')));p.world.mapId="missing";QVERIFY(!p.validate().isEmpty());auto bad=q.toJson();auto systems=bad["systems"].toObject();systems["storages"]=QJsonArray{};bad["systems"]=systems;QVERIFY(!cf::Project::fromJson(bad,p,error));}
};
QTEST_GUILESS_MAIN(SystemsTests)
#include "systems_tests.moc"
