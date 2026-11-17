#include "world_editor.h"
#include "importer.h"
#include "animation_editor.h"
#include "cf_raycast.h"
#include "screen_model.h"
#include <QtWidgets>
#include <memory>
namespace cf {
static void drawWorld(QPainter &p,const Project &project,int cameraX,int cameraY) {
    const auto &w=project.world;const auto *ts=project.asset(w.tileset);if(!ts)return;
    auto image=ts->image(project.target=="gb");if(image.isNull())return;
    int columns=ts->width/8;auto tiles=w.effectiveTiles();
    for(int y=cameraY/8;y<qMin(w.height,(cameraY+144)/8+1);++y) for(int x=cameraX/8;x<qMin(w.width,(cameraX+160)/8+1);++x) {
        int tile=tiles[y*w.width+x];p.drawImage(QRect(x*8-cameraX,y*8-cameraY,8,8),image,QRect(tile%columns*8,tile/columns*8,8,8));
    }
}
static void textOverlay(QPainter &p,const Project &project,int prompt,int choice,const GameSession *session=nullptr) {
    if(prompt<0||prompt>=project.prompts.size())return;
    auto rendered=project;rendered.world.enabled=true;if(session)rendered.prompts[prompt].text=renderGameText(project,session->state,project.prompts[prompt].text,textLayout(project.prompts[prompt]).columns);auto box=project.prompts[prompt].textbox;auto lines=screenLines(rendered,prompt,choice);
    if(session){auto layout=textLayout(project.prompts[prompt]);for(int c=0;c<project.prompts[prompt].choices.size();++c)if(!session->enabled(prompt,c))lines[layout.choiceRow+c*project.prompts[prompt].choiceSpacing][box.x()+project.prompts[prompt].padding]=(project.prompts[prompt].disabledMarker.isEmpty()?QChar('x'):project.prompts[prompt].disabledMarker[0]);}
    p.drawImage(0,0,textScreenImage(lines,box,project.target=="gb",project.prompts[prompt].textPalette));
    if(auto a=project.asset(project.prompts[prompt].portrait)){p.fillRect(152-a->frameWidth(),0,a->frameWidth(),a->frameHeight(),QColor::fromRgbF((a->palette[0]&31)/31.,((a->palette[0]>>5)&31)/31.,((a->palette[0]>>10)&31)/31.));p.drawImage(152-a->frameWidth(),0,a->frameImage(0,project.target=="gb"));}
}
MapCanvas::MapCanvas(QWidget *p):QWidget(p){setMouseTracking(true);}
void MapCanvas::reload(){setFixedSize(project.world.width*8*zoom,project.world.height*8*zoom);update();}
void MapCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);p.scale(zoom,zoom);auto &w=project.world;p.fillRect(0,0,w.width*8,w.height*8,QColor("#283641"));
    const auto *ts=project.asset(w.tileset);
    if(ts){auto image=ts->image(project.target=="gb");int columns=ts->width/8;auto tiles=w.effectiveTiles();
        for(int y=0;y<w.height;++y)for(int x=0;x<w.width;++x){int tile=tiles[y*w.width+x];p.drawImage(QRect(x*8,y*8,8,8),image,QRect(tile%columns*8,tile/columns*8,8,8));}}
    for(int y=0;y<w.height;++y)for(int x=0;x<w.width;++x){int cell=y*w.width+x;
        if(tool==1 && w.collision[cell])p.fillRect(x*8,y*8,8,8,QColor(230,75,73,150));
        if(w.exits.contains(cell))p.fillRect(x*8+1,y*8+1,6,6,QColor("#7ba9f5"));
        if(w.interactions.contains(cell)){p.fillRect(x*8+2,y*8+2,4,4,QColor("#f8bd6d"));}
    }
    p.setPen(QPen(QColor(20,30,40,75),.3));for(int x=0;x<=w.width;++x)p.drawLine(x*8,0,x*8,w.height*8);for(int y=0;y<=w.height;++y)p.drawLine(0,y*8,w.width*8,y*8);
    if(painting&&brushMode==1&&tool<2){auto r=QRect(strokeStart,strokeEnd).normalized();p.setPen(QPen(QColor("#ffb879"),1));p.setBrush(QColor(255,180,120,60));p.drawRect(r.x()*8,r.y()*8,r.width()*8,r.height()*8);}
    p.setPen(QPen(QColor("#4ce2ca"),1));p.setBrush(Qt::NoBrush);p.drawRect(w.spawnX*8+1,w.spawnY*8+1,6,6);
    for(auto &actor:w.actors){const auto *a=project.asset(actor.sprite);if(a)p.drawImage(actor.x*8+4-a->frameWidth()/2,actor.y*8+8-a->frameHeight(),a->frameImage(0,project.target=="gb"));}
    const auto *sp=project.asset(w.player);if(sp)p.drawImage(w.spawnX*8+4-sp->frameWidth()/2,w.spawnY*8+8-sp->frameHeight(),sp->frameImage(0,project.target=="gb"));
}
void MapCanvas::paintCell(QPoint pos,bool erase){
    int x=pos.x()/(8*zoom),y=pos.y()/(8*zoom);auto &w=project.world;if(pos.x()<0||pos.y()<0||x>=w.width||y>=w.height)return;int i=y*w.width+x;
    if(tool==0){int tile=erase?0:brush;if(w.tiles[i]!=tile){w.tiles[i]=tile;changed=true;}}
    if(tool==1){char v=erase?0:1;if(w.collision[i]!=v){w.collision[i]=v;changed=true;}}
    if(tool==2){if(w.spawnX!=x||w.spawnY!=y){w.spawnX=x;w.spawnY=y;changed=true;}}
    if(tool==4){if(erase){changed|=w.exits.remove(i)>0;}else if(!exitTarget.isEmpty()&&w.exits.value(i)!=exitTarget){w.exits[i]=exitTarget;changed=true;}}
    if(tool==3){if(erase){changed|=w.interactions.remove(i)>0;}else if(!trigger.isEmpty()&&w.interactions.value(i)!=trigger){w.interactions[i]=trigger;changed=true;}}
    update();
}
void MapCanvas::mousePressEvent(QMouseEvent *e){if(e->button()!=Qt::LeftButton&&e->button()!=Qt::RightButton)return;int x=e->pos().x()/(8*zoom),y=e->pos().y()/(8*zoom);auto &w=project.world;if(x<0||y<0||x>=w.width||y>=w.height)return;if(e->modifiers()&Qt::AltModifier){brush=w.effectiveTiles()[y*w.width+x];if(onPick)onPick(brush);return;}painting=true;changed=false;erasing=e->button()==Qt::RightButton;strokeStart=strokeEnd={x,y};if(brushMode==2&&tool<2){int target=tool==0?w.tiles[y*w.width+x]:w.collision[y*w.width+x];QVector<QPoint> queue{{x,y}};QByteArray seen(w.width*w.height,0);seen[y*w.width+x]=1;for(int i=0;i<queue.size();++i){auto cell=queue[i];paintCell(QPoint((cell.x()*8+4)*zoom,(cell.y()*8+4)*zoom),erasing);for(auto d:{QPoint(-1,0),QPoint(1,0),QPoint(0,-1),QPoint(0,1)}){auto n=cell+d;if(n.x()<0||n.y()<0||n.x()>=w.width||n.y()>=w.height)continue;int index=n.y()*w.width+n.x();if(!seen[index]&&(tool==0?w.tiles[index]:w.collision[index])==target){seen[index]=1;queue<<n;}}}}else if(brushMode!=1||tool>=2)paintCell(e->pos(),erasing);update();}
void MapCanvas::mouseMoveEvent(QMouseEvent *e){auto cell=QPoint(qBound(0,e->pos().x()/(8*zoom),project.world.width-1),qBound(0,e->pos().y()/(8*zoom),project.world.height-1));setToolTip(QString("Tile %1, %2 · Alt-click picks a tile").arg(cell.x()).arg(cell.y()));if(!painting)return;if(brushMode==1&&tool<2){strokeEnd=cell;update();return;}if(brushMode==2&&tool<2)return;int x=strokeEnd.x(),y=strokeEnd.y(),dx=qAbs(cell.x()-x),dy=-qAbs(cell.y()-y),sx=x<cell.x()?1:-1,sy=y<cell.y()?1:-1,error=dx+dy;while(true){paintCell(QPoint((x*8+4)*zoom,(y*8+4)*zoom),erasing);if(x==cell.x()&&y==cell.y())break;int e2=2*error;if(e2>=dy){error+=dy;x+=sx;}if(e2<=dx){error+=dx;y+=sy;}}strokeEnd=cell;}
void MapCanvas::mouseReleaseEvent(QMouseEvent *){if(painting&&brushMode==1&&tool<2){auto rect=QRect(strokeStart,strokeEnd).normalized();for(int y=rect.top();y<=rect.bottom();++y)for(int x=rect.left();x<=rect.right();++x)paintCell(QPoint((x*8+4)*zoom,(y*8+4)*zoom),erasing);}painting=false;if(changed&&onStroke)onStroke(project.world);changed=false;update();}
void MapCanvas::wheelEvent(QWheelEvent *e){if(e->modifiers()&(Qt::ControlModifier|Qt::MetaModifier)){zoom=qBound(1,zoom+(e->angleDelta().y()>0?1:-1),16);reload();if(onZoom)onZoom(zoom);e->accept();}else QWidget::wheelEvent(e);}
WorldPlay::WorldPlay(QWidget *p):QWidget(p){setFocusPolicy(Qt::StrongFocus);setFixedSize(480,432);auto *timer=new QTimer(this);timer->setTimerType(Qt::PreciseTimer);auto clock=std::make_shared<QElapsedTimer>();clock->start();auto previous=std::make_shared<qint64>(0);auto debt=std::make_shared<double>(0);connect(timer,&QTimer::timeout,this,[this,clock,previous,debt]{qint64 now=clock->nsecsElapsed();double dt=(now-*previous)/1e6;*previous=now;if(!hasFocus()||(!screenMode&&!project.world.enabled)||((dialog||menu>=0)&&project.timers.isEmpty())){*debt=0;return;}*debt=qMin(4*1000.0/60,*debt+dt);while(*debt>=1000.0/60){tick();*debt-=1000.0/60;}});timer->start(4);}
void WorldPlay::setProject(const Project &p){project=p;activeScene=p.sceneIndex(p.areaScene(p.world.id));screenMode=false;reset();if(activeScene>=0)session.run(project.scenes[activeScene].onSceneEnter);requests();}
void WorldPlay::normalizeButtons(){if(!screenMode||activeScene<0)return;auto&s=project.scenes[activeScene];CFCondition c{};QString error;if(screenCursor<0||screenCursor>=s.elements.size()||!GameCompiler(project).condition(s.elements[screenCursor].condition,c,error)||!cf_game_condition(&session.state,&c))chooseButton(1);}
void WorldPlay::chooseButton(int delta){if(activeScene<0)return;auto&s=project.scenes[activeScene];int next=screenCursor;for(int i=0;i<s.elements.size();++i){next=next<0?(delta<0?s.elements.size()-1:0):(next+s.elements.size()+delta)%s.elements.size();CFCondition c{};QString error;auto&e=s.elements[next];if(e.type=="button"&&GameCompiler(project).condition(e.condition,c,error)&&cf_game_condition(&session.state,&c)){screenCursor=next;return;}}screenCursor=-1;}
void WorldPlay::startScene(const QString &id,bool fresh){int index=project.sceneIndex(id);if(index<0)return;if(fresh)session.configure(project);activeScene=index;screenMode=project.scenes[index].kind=="screen";held.clear();dialog=false;menu=-1;screenCursor=-1;session.state.request_prompt=session.state.request_storage=session.state.request_scene=session.state.request_area=255;session.state.request_world=0;if(!screenMode){auto areas=project.allAreas();for(auto&a:areas)if(a.id==project.scenes[index].startArea){project.world=a;project.world.enabled=true;project.areas.clear();for(auto&other:areas)if(other.id!=a.id)project.areas<<other;break;}reset(false);}else chooseButton(1);session.run(project.scenes[index].onSceneEnter);normalizeButtons();if(fresh)requests();update();}
void WorldPlay::reset(bool fresh,bool enter){if(fresh)session.configure(project);auto &w=project.world;solids=w.effectiveCollision();health.clear();for(auto &a:w.actors)health<<a.health;menu=-1;state={int16_t(w.spawnX*8),int16_t(w.spawnY*8),0,0,0,-1};cf_world_camera(&state,w.width,w.height);flow={0,0};dialog=false;held.clear();movementClock=0;touchCooldown=60;
    BehaviorSpec spec;QString error;parseBehavior(w.playerScript,spec,error);playerBehavior=spec.code;actorStates.resize(w.actors.size());actorBehaviors.clear();
    const auto *sprite=project.asset(w.player);playerSheet=sprite?sprite->image(project.target=="gb"):QImage();playerClips=sprite?sprite->clips():QVector<CFAnimation>{{0,0,1,1}};playerBindings=sprite?animationBindings(spec,*sprite):CFAnimationBindings{0,255,255,255};cf_animation_init(&playerAnimation,playerClips.data(),playerClips.size(),playerBindings);playerMoving=false;
    actorAnimations.resize(w.actors.size());actorClips.clear();actorBindings.clear();actorSheets.clear();
    for(int i=0;i<w.actors.size();++i){cf_actor_init(&actorStates[i],w.actors[i].x*8,w.actors[i].y*8);BehaviorSpec b;parseBehavior(w.actors[i].script,b,error);b.code.touch_prompt=b.prompt.isEmpty()?255:project.indexOf(b.prompt);actorBehaviors<<b.code;
        const auto *a=project.asset(w.actors[i].sprite);actorSheets.append(a?a->image(project.target=="gb"):QImage());actorClips.append(a?a->clips():QVector<CFAnimation>{{0,0,1,1}});actorBindings<< (a?animationBindings(b,*a):CFAnimationBindings{0,255,255,255});cf_animation_init(&actorAnimations[i],actorClips[i].data(),actorClips[i].size(),actorBindings[i]);
    }stepRules.clear();stepCode.resize(w.terrainEvents.size());for(int i=0;i<w.terrainEvents.size();++i){auto &e=w.terrainEvents[i];if(!GameCompiler(project).program(e.script,stepCode[i],error))stepCode[i].clear();stepRules<<CFStepRule{uint8_t(e.tile),uint8_t(e.every),uint8_t(e.chance),{uint8_t(stepCode[i].size()),stepCode[i].data()}};}cf_game_step_init(&stepState,(state.y+4)/8*w.width+(state.x+4)/8);if(enter)session.run(w.onStart);update();}
void WorldPlay::tick(){if(!hasFocus()||(!screenMode&&!project.world.enabled))return;session.tick(dialog||menu>=0);if(session.state.dirty||session.state.request_world||session.state.request_prompt!=255||session.state.request_storage!=255)requests();if(dialog||menu>=0||screenMode)return;
    int dx=held.contains(Qt::Key_Left)||held.contains(Qt::Key_A)?-1:held.contains(Qt::Key_Right)||held.contains(Qt::Key_D)?1:0;
    int dy=held.contains(Qt::Key_Up)||held.contains(Qt::Key_W)?-1:held.contains(Qt::Key_Down)||held.contains(Qt::Key_S)?1:0;auto &w=project.world;
    auto old=QPoint(state.x,state.y);
    if(++movementClock>=playerBehavior.interval){movementClock=0;if(playerBehavior.kind==CF_BEHAVIOR_PLAYER)for(int i=0;i<playerBehavior.speed;++i)cf_world_move(&state,w.width,w.height,reinterpret_cast<const uint8_t *>(solids.constData()),dx,dy);}
    if(!dx&&!dy)playerMoving=false;else if(!movementClock)playerMoving=old!=QPoint(state.x,state.y);
    cf_animation_update(&playerAnimation,playerClips.data(),playerClips.size(),playerBindings,playerMoving,0);
    if(touchCooldown)--touchCooldown;
    for(int i=0;i<actorStates.size();++i){
        if(!health.value(i))continue;bool touch=cf_actor_tick(&actorStates[i],&actorBehaviors[i],&state,w.width,w.height,reinterpret_cast<const uint8_t *>(solids.constData()));
        cf_animation_update(&actorAnimations[i],actorClips[i].data(),actorClips[i].size(),actorBindings[i],actorStates[i].moving,0);
        if(touch&&!touchCooldown&&actorBehaviors[i].touch_prompt<project.prompts.size()){dialog=true;session.enterPrompt(flow,actorBehaviors[i].touch_prompt);requests();touchCooldown=60;state.camera_x&=~7;state.camera_y&=~7;}
    }
    if(!stepRules.isEmpty()){int cell=(state.y+4)/8*w.width+(state.x+4)/8;auto tiles=w.effectiveTiles();if(cell>=0&&cell<tiles.size())cf_game_step(&stepState,&session.state,&session.definition,stepRules.data(),stepRules.size(),cell,tiles[cell]);}requests();
    update();}
void WorldPlay::requests(){for(int i=0;i<8&&session.state.request_scene<project.scenes.size();++i){auto id=project.scenes[session.state.request_scene].id;session.state.request_scene=255;startScene(id,false);}session.state.request_scene=255;if(session.state.request_area!=255){int index=session.state.request_area;session.state.request_area=255;auto areas=session.data().runtimeAreas();if(index<areas.size()){auto target=areas[index];auto owner=project.areaScene(target.id);bool changed=project.sceneIndex(owner)!=activeScene;auto all=project.allAreas();project.world=target;project.world.enabled=true;project.areas.clear();for(auto&a:all)if(a.id!=target.id)project.areas<<a;activeScene=project.sceneIndex(owner);screenMode=false;reset(false);if(changed)session.run(project.scenes[activeScene].onSceneEnter);}}if(session.state.request_world){session.state.request_world=0;session.state.request_prompt=session.state.request_storage=255;dialog=false;menu=-1;touchCooldown=60;cf_world_camera(&state,project.world.width,project.world.height);session.state.dirty=0;update();return;}for(int i=0;i<4&&session.state.request_prompt<project.prompts.size();++i){int target=session.state.request_prompt;session.state.request_prompt=255;session.enterPrompt(flow,target);dialog=true;menu=-1;state.camera_x&=~7;state.camera_y&=~7;}session.state.request_prompt=255;if(session.state.request_world){requests();return;}if(session.state.request_storage<project.storages.size()){menu=session.state.request_storage;menuReadOnly=session.state.request_readonly;session.state.request_readonly=0;session.state.request_storage=255;menuChoice=menuSide=0;}normalizeButtons();session.state.dirty=0;update();}
void WorldPlay::interact(bool combat){auto &w=project.world;QVector<CFHitbox> targets;QVector<QString> exits,prompts;
for(int i=0;i<w.objects.size();++i){auto &o=w.objects[i];targets<<CFHitbox{int16_t(o.x*8),int16_t(o.y*8),8,8,CF_HIT_OBJECT,uint8_t(i)};}
for(int i=0;i<actorStates.size();++i)if(health.value(i))targets<<CFHitbox{actorStates[i].body.x,actorStates[i].body.y,8,8,CF_HIT_ACTOR,uint8_t(i)};
if(!combat){for(auto it=w.exits.begin();it!=w.exits.end();++it){targets<<CFHitbox{int16_t(it.key()%w.width*8),int16_t(it.key()/w.width*8),8,8,CF_HIT_EXIT,uint8_t(exits.size())};exits<<it.value();}for(auto it=w.interactions.begin();it!=w.interactions.end();++it){targets<<CFHitbox{int16_t(it.key()%w.width*8),int16_t(it.key()/w.width*8),8,8,CF_HIT_PROMPT,uint8_t(prompts.size())};prompts<<it.value();}}
int distance=combat?w.attackRange:w.interactionRange;CFRayHit hit{};cf_raycast(state.x+4,state.y+4,state.x+4+state.facing_x*distance,state.y+4+state.facing_y*distance,w.width,w.height,reinterpret_cast<const uint8_t *>(solids.constData()),targets.data(),targets.size(),&hit);
if(hit.kind==CF_HIT_OBJECT){auto &o=w.objects[hit.index];if(combat)session.run(o.onHit);else if(!o.onInteract.trimmed().isEmpty())session.run(o.onInteract);else if(o.kind=="storage")session.state.request_storage=project.storageIndex(o.storage);}
if(hit.kind==CF_HIT_ACTOR){auto &a=w.actors[hit.index];if(!combat)cf_animation_update(&actorAnimations[hit.index],actorClips[hit.index].data(),actorClips[hit.index].size(),actorBindings[hit.index],actorStates[hit.index].moving,1);if(combat){--health[hit.index];session.run(a.onHit);if(!health[hit.index])session.run(a.onDefeat);}else if(!a.onInteract.trimmed().isEmpty())session.run(a.onInteract);else if(actorBehaviors[hit.index].touch_prompt<project.prompts.size())session.state.request_prompt=actorBehaviors[hit.index].touch_prompt;}
if(hit.kind==CF_HIT_PROMPT)session.state.request_prompt=project.indexOf(prompts[hit.index]);
if(hit.kind==CF_HIT_EXIT){auto maps=project.allAreas();for(auto area:maps)if(area.id==exits[hit.index]){bool changed=project.areaScene(area.id)!=project.scenes.value(activeScene).id;activeScene=project.sceneIndex(project.areaScene(area.id));project.world=area;project.areas.clear();for(auto other:maps)if(other.id!=area.id)project.areas<<other;reset(false);if(changed&&activeScene>=0)session.run(project.scenes[activeScene].onSceneEnter);requests();return;}}
requests();}
void WorldPlay::menuInput(int key){if(key==8){menu=-1;update();return;}if(key==16&&menu&&!menuReadOnly){menuSide=!menuSide;menuChoice=0;}int source=menu&&menuSide?0:menu;QVector<int> ids;for(int i=0;i<project.items.size();++i)if(session.state.counts[source][i])ids<<i;
if(!ids.isEmpty()){menuChoice=qBound(0,menuChoice,int(ids.size())-1);if(key==1)menuChoice=(menuChoice+ids.size()-1)%ids.size();if(key==2)menuChoice=(menuChoice+1)%ids.size();if(key==4){int item=ids[menuChoice];if(menu&&!menuReadOnly)session.state.variables[31]=cf_storage_transfer(&session.state,&session.definition,source,menuSide?menu:0,item,1);else if(session.run(project.items[item].useScript)&&!menuReadOnly&&project.items[item].consumable)cf_storage_change(&session.state,&session.definition,0,item,-1);}}requests();update();}
void WorldPlay::input(int key){
if(screenMode&&!dialog&&menu<0){normalizeButtons();if(key==1)chooseButton(-1);if(key==2)chooseButton(1);if(key==4&&screenCursor>=0)session.run(project.scenes[activeScene].elements[screenCursor].actions);if(key==8)session.run(project.scenes[activeScene].onBack);requests();update();return;}

if(menu>=0){menuInput(key);return;}if(key==16){if(dialog&&project.prompts.value(flow.prompt).modal)return;menu=0;menuReadOnly=false;menuChoice=menuSide=0;update();return;}
if(key==8){if(dialog){if(project.prompts.value(flow.prompt).modal)return;dialog=false;touchCooldown=60;cf_world_camera(&state,project.world.width,project.world.height);}else interact(true);update();return;}
if(!dialog){if(key==4){cf_animation_update(&playerAnimation,playerClips.data(),playerClips.size(),playerBindings,playerMoving,1);interact();}update();return;}
session.step(flow,key);requests();update();}
void WorldPlay::paintEvent(QPaintEvent *){
    QPainter p(this);p.scale(3,3);p.fillRect(0,0,160,144,QColor("#1b2b34"));if(screenMode&&activeScene>=0)p.drawImage(0,0,sceneImage(project,project.scenes[activeScene],&session.state,screenCursor));else drawWorld(p,project,state.camera_x,state.camera_y);
    if(!screenMode&&!dialog)for(int i=0;i<actorStates.size();++i){if(!health.value(i))continue;const auto *sprite=project.asset(project.world.actors[i].sprite);if(sprite)p.drawImage(actorStates[i].body.x+4-sprite->frameWidth()/2-state.camera_x,actorStates[i].body.y+8-sprite->frameHeight()-state.camera_y,actorSheets[i].copy(sprite->frameRect(actorAnimations[i].state.frame)).mirrored(cf_animation_transform(&actorAnimations[i],actorClips[i].data(),actorClips[i].size(),actorBehaviors[i].flags,actorStates[i].body.facing_x)&1,cf_animation_transform(&actorAnimations[i],actorClips[i].data(),actorClips[i].size(),actorBehaviors[i].flags,actorStates[i].body.facing_x)&2));}
    const auto *sp=project.asset(project.world.player);if(sp&&!screenMode&&!dialog)p.drawImage(state.x+4-sp->frameWidth()/2-state.camera_x,state.y+8-sp->frameHeight()-state.camera_y,playerSheet.copy(sp->frameRect(playerAnimation.state.frame)).mirrored(cf_animation_transform(&playerAnimation,playerClips.data(),playerClips.size(),playerBehavior.flags,state.facing_x)&1,cf_animation_transform(&playerAnimation,playerClips.data(),playerClips.size(),playerBehavior.flags,state.facing_x)&2));
    if(dialog)textOverlay(p,project,flow.prompt,flow.choice,&session);
    if(menu>=0){QStringList lines;lines<<project.storages[menu&&menuSide?0:menu].name<<"";int source=menu&&menuSide?0:menu;QVector<int> ids;for(int i=0;i<project.items.size();++i)if(session.state.counts[source][i])ids<<i;menuChoice=qBound(0,menuChoice,qMax(0,int(ids.size())-1));int start=qMax(0,menuChoice-9);if(ids.isEmpty())lines<<"Empty";for(int row=start;row<ids.size()&&row<start+10;++row)lines<<QString("%1%2 %3").arg(row==menuChoice?">":" ").arg(project.items[ids[row]].name.left(12)).arg(session.state.counts[source][ids[row]]);while(lines.size()<15)lines<<"";lines<<(menuReadOnly?"Choose: inspect":menu?"Choose: move item":"Choose: use item")<<"Back: close";p.drawImage(0,0,textScreenImage(lines,QRect(0,0,20,18),project.target=="gb"));}
    if(!hasFocus()){p.fillRect(0,0,160,14,QColor(10,20,30,205));p.setPen(Qt::white);QFont f;f.setPixelSize(6);p.setFont(f);p.drawText(QRect(0,0,160,14),Qt::AlignCenter,"CLICK TO PLAY · ARROWS / WASD · Z=A · X=B");}
    setAccessibleName(dialog?"Overworld dialogue: "+project.prompts.value(flow.prompt).title:QString("Overworld player at %1, %2; camera %3, %4").arg(state.x).arg(state.y).arg(state.camera_x).arg(state.camera_y));
}
void WorldPlay::keyPressEvent(QKeyEvent *e){if(e->isAutoRepeat()){e->accept();return;}held.insert(e->key());int button=e->key()==Qt::Key_Z?16:e->key()==Qt::Key_X?32:(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter)?128:e->key()==Qt::Key_Backspace?64:0;
    if(button==project.controls.interact)input(4);if(button==project.controls.close)input(8);if(button==project.controls.inventory)input(16);if(button==project.controls.restart){if(screenMode){startScene(project.scenes[activeScene].id,false);return;}if(dialog&&project.prompts.value(flow.prompt).modal)return;reset(false,false);update();}
    if(e->key()==Qt::Key_Escape)input(8);if((dialog||menu>=0||screenMode)&&e->key()==Qt::Key_Up)input(1);if((dialog||menu>=0||screenMode)&&e->key()==Qt::Key_Down)input(2);e->accept();}
void WorldPlay::keyReleaseEvent(QKeyEvent *e){if(!e->isAutoRepeat())held.remove(e->key());e->accept();}
void WorldPlay::focusOutEvent(QFocusEvent *e){held.clear();QWidget::focusOutEvent(e);update();}
WorldEditor::WorldEditor(QWidget *parent):QWidget(parent){
    auto *root=new QHBoxLayout(this);auto *side=new QVBoxLayout;
    areaSelector=new QComboBox;side->addWidget(new QLabel("CHAPTER / AREA"));side->addWidget(areaSelector);
    areaName=new QLineEdit;chapter=new QLineEdit;areaName->setPlaceholderText("Area name");chapter->setPlaceholderText("Chapter");side->addWidget(areaName);side->addWidget(chapter);
    auto *areaButtons=new QHBoxLayout;auto *addArea=new QPushButton("+ Area"),*duplicate=new QPushButton("Duplicate"),*makeStart=new QPushButton("Use as scene start");areaButtons->addWidget(addArea);areaButtons->addWidget(duplicate);areaButtons->addWidget(makeStart);side->addLayout(areaButtons);
    auto *testArea=new QPushButton("Build & Play This Area");testArea->setObjectName("primary");side->addWidget(testArea);
    connect(testArea,&QPushButton::clicked,this,[this]{if(onTestArea)onTestArea(project.isolatedArea(activeArea));});
    connect(areaSelector,&QComboBox::activated,this,[this](int i){activeArea=i;setProject(project);});
    connect(areaName,&QLineEdit::editingFinished,this,[this]{if(!filling)editWorld([&](World&w){w.name=areaName->text();});});
    connect(chapter,&QLineEdit::editingFinished,this,[this]{if(!filling)editWorld([&](World&w){w.chapter=chapter->text();});});
    auto create=[this](bool copy){if(project.areas.size()>=7){QMessageBox::information(this,"Area limit","Use at most 8 areas.");return;}auto p=project;World w=activeWorld();w.id="area_"+QUuid::createUuid().toString(QUuid::Id128);w.name=copy?w.name+" copy":"New Area";w.exits.clear();if(!copy){w.tiles.fill(0);w.collision.fill(0);w.interactions.clear();}p.areas<<w;activeArea=p.areas.size();if(onEdit)onEdit(p);};
    connect(addArea,&QPushButton::clicked,this,[create]{create(false);});connect(duplicate,&QPushButton::clicked,this,[create]{create(true);});
    connect(makeStart,&QPushButton::clicked,this,[this]{auto p=project;auto w=activeWorld();int scene=p.sceneIndex(p.areaScene(w.id));if(scene<0)return;p.scenes[scene].kind="world";p.scenes[scene].startArea=w.id;if(onEdit)onEdit(p);});
    auto *deleteArea=new QPushButton("Delete selected area");side->addWidget(deleteArea);
    connect(deleteArea,&QPushButton::clicked,this,[this]{if(project.areas.isEmpty())return;auto p=project;QString id=activeWorld().id;bool enabled=p.world.enabled;if(activeArea==0){p.world=p.areas.takeFirst();p.world.enabled=enabled;}else p.areas.removeAt(activeArea-1);auto clean=[&](World &w){for(auto i=w.exits.begin();i!=w.exits.end();)if(i.value()==id)i=w.exits.erase(i);else ++i;};clean(p.world);for(auto &w:p.areas)clean(w);activeArea=0;if(onEdit)onEdit(p);});
    enabled=new QCheckBox;enabled->hide();auto *startup=new QLabel("Game startup: File → Project Settings → Default Scene.\nUse as scene start sets the initial area of this area’s parent scene.");startup->setWordWrap(true);side->addWidget(startup);
    auto *form=new QFormLayout;tileset=new QComboBox;player=new QComboBox;width=new QSpinBox;height=new QSpinBox;
    width->setRange(20,64);height->setRange(18,64);form->addRow("Tileset",tileset);form->addRow("Player sprite",player);form->addRow("Map width (tiles)",width);form->addRow("Map height (tiles)",height);side->addLayout(form);
    auto *resize=new QPushButton("Resize map (keep overlapping tiles)");side->addWidget(resize);
    auto *importSprite=new QPushButton("Import sprite…"),*importTiles=new QPushButton("Import tileset…"),*replace=new QPushButton("Replace selected asset…");side->addWidget(importSprite);side->addWidget(importTiles);
    assetList=new QListWidget;assetList->setMaximumHeight(130);side->addWidget(assetList);side->addWidget(replace);
    auto *tool=new QComboBox;tool->addItems({"Paint tiles","Paint collision (red = solid)","Place player spawn","Place prompt interaction","Place area exit"});side->addWidget(tool);auto *brushMode=new QComboBox;brushMode->addItems({"Pencil","Rectangle fill","Flood fill"});side->addWidget(brushMode);connect(brushMode,&QComboBox::activated,this,[this](int i){canvas->brushMode=i;});
    prompt=new QComboBox;side->addWidget(prompt);exitArea=new QComboBox;side->addWidget(exitArea);side->addWidget(new QLabel("Drag paints · Right drag erases · Alt-click picks"));
    tileList=new QListWidget;tileList->setViewMode(QListView::IconMode);tileList->setIconSize({48,48});tileList->setGridSize({66,76});tileList->setMinimumWidth(250);side->addWidget(tileList,1);
    auto *sidebar=new QWidget;sidebar->setLayout(side);sidebar->setMinimumWidth(275);auto *sideScroll=new QScrollArea;sideScroll->setWidgetResizable(true);sideScroll->setWidget(sidebar);sideScroll->setMinimumWidth(305);sideScroll->setMaximumWidth(345);root->addWidget(sideScroll);
    auto *tabs=new QTabWidget;canvas=new MapCanvas;auto *scroll=new QScrollArea;scroll->setWidget(canvas);tabs->addTab(scroll,"Map & collision layers");tilesheet=new SheetCanvas;tabs->addTab(sheetViewer(tilesheet),"Tilesheet");tilesheet->onFrame=[this](int i,bool){canvas->brush=i;tileList->setCurrentRow(i);tilesheet->first=tilesheet->last=tilesheet->current=i;tilesheet->update();};
    auto *mapZoom=new QSpinBox;mapZoom->setRange(1,16);mapZoom->setValue(canvas->zoom);canvas->onZoom=[mapZoom](int n){QSignalBlocker block(mapZoom);mapZoom->setValue(n);};canvas->onPick=[this](int n){tileList->setCurrentRow(n);};mapZoom->setPrefix("Map zoom ");mapZoom->setSuffix("×");side->insertWidget(side->count()-1,mapZoom);connect(mapZoom,&QSpinBox::valueChanged,this,[this](int n){canvas->zoom=n;canvas->reload();});auto *tileZoom=new QSpinBox;tileZoom->setRange(1,16);tileZoom->setValue(6);tileZoom->setPrefix("Tile thumbnails ");tileZoom->setSuffix("×");side->insertWidget(side->count()-1,tileZoom);connect(tileZoom,&QSpinBox::valueChanged,this,[this](int n){tileList->setIconSize(QSize(8*n,8*n));tileList->setGridSize(QSize(8*n+18,8*n+28));});
    auto *playPage=new QWidget;auto *pl=new QVBoxLayout(playPage);pl->addWidget(new QLabel("Click the viewport, then use arrows / WASD. Z=A, X=B, Enter=Start, Backspace=Select.\nB attacks or closes. Select opens inventory / switches container sides.\nPlayer collision is an 8×8 foot box; the camera follows within map bounds."));
    play=new WorldPlay;pl->addWidget(play,0,Qt::AlignCenter);auto *controls=new QHBoxLayout;
    for(auto pair:{qMakePair(QString("A / Interact"),4),qMakePair(QString("B / Attack / Close"),8)}){auto *b=new QPushButton(pair.first);controls->addWidget(b);connect(b,&QPushButton::clicked,this,[this,pair]{play->input(pair.second);play->setFocus();});}
    auto *reset=new QPushButton("Reset position");controls->addWidget(reset);connect(reset,&QPushButton::clicked,play,[this]{play->reset(false,false);play->setFocus();});pl->addLayout(controls);pl->addStretch();tabs->addTab(playPage,"Play overworld");
    auto *center=new QVBoxLayout;center->addWidget(tabs,1);status=new QLabel;status->setWordWrap(true);center->addWidget(status);root->addLayout(center,1);
    connect(enabled,&QCheckBox::toggled,this,[this](bool b){if(!filling){auto p=project;p.world.enabled=b;if(onEdit)onEdit(p);}});
    connect(tileset,&QComboBox::activated,this,[this](int i){editWorld([&](World&w){w.tileset=tileset->itemData(i).toString();auto *a=project.asset(w.tileset);if(a)for(auto &t:w.tiles)if(t>=a->tileCount())t=0;});});
    connect(player,&QComboBox::activated,this,[this](int i){editWorld([&](World&w){w.player=player->itemData(i).toString();});});
    connect(resize,&QPushButton::clicked,this,[this]{editWorld([&](World&w){w.resize(width->value(),height->value());});});
    connect(importSprite,&QPushButton::clicked,this,[this]{doImport("sprite");});connect(importTiles,&QPushButton::clicked,this,[this]{doImport("tileset");});
    connect(replace,&QPushButton::clicked,this,[this]{int i=assetList->currentRow();if(i>=0&&i<project.assets.size())doImport(project.assets[i].kind,true);});
    connect(tool,&QComboBox::currentIndexChanged,this,[this](int i){canvas->tool=i;canvas->update();});
    connect(exitArea,&QComboBox::activated,this,[this](int i){canvas->exitTarget=exitArea->itemData(i).toString();});
    connect(prompt,&QComboBox::activated,this,[this](int i){canvas->trigger=prompt->itemData(i).toString();});
    connect(tileList,&QListWidget::currentRowChanged,this,[this](int i){if(i>=0)canvas->brush=i;});
    canvas->onStroke=[this](const World &w){editWorld([&](World &v){v=w;});};
    connect(tabs,&QTabWidget::currentChanged,this,[this](int i){if(i==2){play->reset();play->setFocus();}});
}
World &WorldEditor::activeWorld(){return activeArea>0 && activeArea<=project.areas.size()?project.areas[activeArea-1]:project.world;}
Project WorldEditor::viewingProject() const {auto p=project;auto maps=p.allAreas();if(activeArea<0||activeArea>=maps.size())return p;p.world=maps.takeAt(activeArea);p.world.enabled=true;p.areas=maps;return p;}
void WorldEditor::editWorld(const std::function<void(World &)> &f){if(filling)return;auto snapshot=project;f(activeArea>0?snapshot.areas[activeArea-1]:snapshot.world);if(onEdit)onEdit(snapshot);}
void WorldEditor::setProject(const Project &p){
    filling=true;QString currentPrompt=prompt->currentData().toString();int currentAsset=assetList->currentRow(),currentTile=tileList->currentRow();project=p;activeArea=qBound(0,activeArea,p.areas.size());auto &w=activeWorld();
    areaSelector->clear();exitArea->clear();for(auto a:p.allAreas()){areaSelector->addItem(a.chapter+" / "+a.name,a.id);exitArea->addItem(a.name,a.id);}areaSelector->setCurrentIndex(activeArea);
    areaName->setText(w.name);chapter->setText(w.chapter);canvas->exitTarget=exitArea->currentData().toString();
    enabled->setChecked(p.world.enabled);width->setValue(w.width);height->setValue(w.height);tileset->clear();player->clear();tileset->addItem("Choose tileset",QString());player->addItem("Choose player",QString());assetList->clear();
    for(auto &a:p.assets){if(a.kind=="tileset")tileset->addItem(a.name,a.id);else player->addItem(a.name,a.id);assetList->addItem(a.name+" · "+a.kind+QString(" %1×%2").arg(a.width).arg(a.height));}
    assetList->setCurrentRow(p.assets.isEmpty() ? -1 : qBound(0,currentAsset,p.assets.size()-1));tileset->setCurrentIndex(qMax(0,tileset->findData(w.tileset)));player->setCurrentIndex(qMax(0,player->findData(w.player)));
    prompt->clear();for(auto &node:p.prompts)prompt->addItem(node.title,node.id);prompt->setCurrentIndex(qMax(0,prompt->findData(currentPrompt)));canvas->trigger=prompt->currentData().toString();
    tilesheet->asset={};tilesheet->sheetImage={};tileList->clear();if(auto *a=p.asset(w.tileset)){tilesheet->asset=*a;tilesheet->asset.columns=a->width/8;tilesheet->asset.rows=a->height/8;tilesheet->sheetImage=a->image(p.target=="gb");tilesheet->dmg=p.target=="gb";auto image=a->image(p.target=="gb");for(int i=0;i<a->tileCount();++i){auto tile=image.copy(i%(a->width/8)*8,i/(a->width/8)*8,8,8);auto *item=new QListWidgetItem(QIcon(QPixmap::fromImage(tile.scaled(48,48))),QString::number(i));tileList->addItem(item);}}
    tilesheet->refreshSize();tileList->setCurrentRow(tileList->count() ? qBound(0,currentTile,tileList->count()-1) : -1);canvas->project=viewingProject();canvas->reload();play->setProject(viewingProject());
    status->setText(QString("%1 × %2 tiles · %3 × %4 px · %5 interaction tiles · %6\nCollision and navigation run the same C code in the preview and ROM.").arg(w.width).arg(w.height).arg(w.width*8).arg(w.height*8).arg(w.interactions.size()).arg(p.target=="gb"?"Game Boy · four shades":"Game Boy Color · colour palettes"));auto issues=p.validate();if(!issues.isEmpty())status->setText(status->text()+"\n"+issues.mid(0,2).join("\n"));filling=false;
}
void WorldEditor::doImport(const QString &kind,bool replace){
    int index=replace?assetList->currentRow():-1;if(!replace&&project.assets.size()>=32){QMessageBox::information(this,"Asset limit","This version supports up to 32 embedded assets.");return;}
    RasterAsset asset;if(!importAsset(this,kind,asset))return;auto p=project;
    if(index>=0&&index<p.assets.size()){asset.id=p.assets[index].id;p.assets[index]=asset;}else p.assets<<asset;
    auto &world=activeArea>0?p.areas[activeArea-1]:p.world;
    if(kind=="sprite"&&world.player.isEmpty())world.player=asset.id;if(kind=="tileset"&&world.tileset.isEmpty())world.tileset=asset.id;
    if(kind=="tileset") {if(p.world.tileset==asset.id)for(auto &tile:p.world.tiles)if(tile>=asset.tileCount())tile=0;for(auto &a:p.areas)if(a.tileset==asset.id)for(auto &tile:a.tiles)if(tile>=asset.tileCount())tile=0;}
    if(onEdit)onEdit(p);
}
}
