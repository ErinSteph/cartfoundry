#include "behavior.h"
#include "assets.h"
#include <QRegularExpression>
#include <QSet>
namespace cf {
bool parseBehavior(const QString &script,BehaviorSpec &out,QString &error){
    BehaviorSpec spec;QSet<QString>seen;int lineNo=0;bool kind=false;
    for(QString line:script.split('\n')){++lineNo;line=line.section('#',0,0).trimmed();if(line.isEmpty())continue;auto words=line.split(QRegularExpression("\\s+"));
        auto fail=[&](QString reason){error=QString("Line %1: %2").arg(lineNo).arg(reason);return false;};
        if(words.size()!=2)return fail("use a command and one value.");QString key=words[0],value=words[1];if(seen.contains(key))return fail("duplicate command: "+key);seen.insert(key);
        if(key=="behavior"){QStringList kinds{"idle","top_down","patrol","chase"};int i=kinds.indexOf(value);if(i<0)return fail("behavior must be idle, top_down, patrol or chase.");spec.code.kind=i;kind=true;}
        else if(key=="axis"){if(value!="x"&&value!="y")return fail("axis must be x or y.");spec.code.axis=value=="y";}
        else if(key=="animation" || key=="on_move" || key=="on_idle" || key=="on_interact"){
            QString name=value=="none"?QString():value;if(key=="animation")spec.animation=name;if(key=="on_move")spec.moveAnimation=name;if(key=="on_idle")spec.idleAnimation=name;if(key=="on_interact")spec.actionAnimation=name;
        }
        else if(key=="flip_h"||key=="flip_v"||key=="mirror_facing"){if(value!="yes"&&value!="no")return fail("use yes or no.");if(value=="yes")spec.code.flags|=key=="flip_h"?1:key=="flip_v"?2:4;}
        else if(key=="on_touch"){if(value!="none")spec.prompt=value;}
        else{bool ok;int n=value.toInt(&ok);int max=key=="speed"?2:key=="every"?8:255;
            if(!QStringList{"speed","every","range","sight"}.contains(key))return fail("unknown command: "+key);
            if(!ok||n<1||n>max)return fail(QString("%1 must be 1-%2.").arg(key).arg(max));
            if(key=="speed")spec.code.speed=n;if(key=="every")spec.code.interval=n;if(key=="range")spec.code.range=n;if(key=="sight")spec.code.sight=n;
        }
    }
    if(!kind){error="Add a behavior command.";return false;}out=spec;error.clear();return true;
}
CFAnimationBindings animationBindings(const BehaviorSpec &b,const RasterAsset &a){
    auto index=[&](const QString &name,int fallback){int i=a.animationIndex(name);return uint8_t(name.isEmpty()?fallback:i<0?255:i);};
    return {index(b.animation,0),index(b.moveAnimation,255),index(b.idleAnimation,255),index(b.actionAnimation,255)};
}
QString behaviorTemplate(const QString &kind){
    if(kind=="top_down")return "# Player movement\nbehavior top_down\nspeed 1\nevery 1\n";
    if(kind=="patrol")return "# Walk back and forth; reverse at solid tiles\nbehavior patrol\naxis x\nrange 32\nspeed 1\nevery 2\non_touch none\n";
    if(kind=="chase")return "# Follow the player within sight distance\nbehavior chase\nsight 80\nspeed 1\nevery 2\non_touch none\n";
    return "# Stationary character\nbehavior idle\non_touch none\n";
}
}
