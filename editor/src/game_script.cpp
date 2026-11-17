#include "game_script.h"
#include "project.h"
#include <QRegularExpression>
namespace cf {
GameCompiler::GameCompiler(const Project &p):project(p){}
int GameCompiler::variable(const QString &name)const{QString n=name;if(n.startsWith('$'))n.remove(0,1);return n=="result"?31:project.variableIndex(n);}
bool GameCompiler::value(const QString &token,int16_t &out,uint8_t &isVariable,QString &error)const{bool ok;int n=token.toInt(&ok);if(ok&&n>=-32768&&n<=32767){out=n;isVariable=0;return true;}int index=variable(token);if(index>=0){out=index;isVariable=1;return true;}error="Unknown variable or out-of-range integer: "+token;return false;}
bool GameCompiler::condition(const QString &source,CFCondition &out,QString &error)const{
    out={};if(source.trimmed().isEmpty())return true;auto tokens=source.trimmed().split(QRegularExpression("\\s+"));QString lhs,op,rhs;
    if(tokens.size()==5&&tokens[0]=="has"){out.type=2;int s=project.storageIndex(tokens[1]),i=project.itemIndex(tokens[2]);if(s<0||i<0){error="Unknown storage or item in condition.";return false;}out.a=s;out.b=i;op=tokens[3];rhs=tokens[4];}
    else if(tokens.size()==3){out.type=1;int v=variable(tokens[0]);if(v<0){error="Unknown variable in condition: "+tokens[0];return false;}out.a=v;op=tokens[1];rhs=tokens[2];}
    else{error="Use 'variable >= value' or 'has storage item >= value'.";return false;}
    QStringList operators{"==","!=","<","<=",">",">="};int cmp=operators.indexOf(op);if(cmp<0){error="Unknown comparison: "+op;return false;}out.compare=cmp;return value(rhs,out.value,out.rhs_variable,error);
}
bool GameCompiler::program(const QString &source,QVector<CFInstruction> &out,QString &error)const{out.clear();return expand(source,out,error,{});}
bool GameCompiler::expand(const QString &source,QVector<CFInstruction> &out,QString &error,QStringList chain)const{
    struct Branch{int test,jump;};QVector<Branch> stack;int lineNumber=0;
    for(auto line:source.split('\n')){++lineNumber;line=line.section('#',0,0).trimmed();if(line.isEmpty())continue;auto t=line.split(QRegularExpression("\\s+"));auto fail=[&](QString reason){error=QString("Line %1: %2").arg(lineNumber).arg(reason);return false;};CFInstruction code{};
        if(t[0]=="call"&&t.size()==2){if(chain.size()>=4||chain.contains(t[1]))return fail("Recursive/deep script call.");bool found=false;for(auto &s:project.scripts)if(s.id==t[1]){found=true;auto next=chain;next<<s.id;if(!expand(s.source,out,error,next))return false;break;}if(!found)return fail("Unknown script: "+t[1]);continue;}
        if(t[0]=="if"){if(stack.size()>=8)return fail("At most 8 nested conditions.");CFCondition c;if(!condition(line.mid(3),c,error)||!c.type)return fail(error);code.op=c.type==1?CF_OP_IF_VAR:CF_OP_IF_ITEM;code.a=c.a;code.b=c.type==1?c.compare:c.b;code.c=c.type==1?c.rhs_variable:c.compare|(c.rhs_variable?128:0);code.value=c.value;stack<<Branch{int(out.size()),-1};}
        else if(t[0]=="else"&&t.size()==1){if(stack.isEmpty()||stack.last().jump>=0)return fail("Unmatched else.");stack.last().jump=out.size();out[stack.last().test].d=out.size()+1;code.op=CF_OP_JUMP;}
        else if(t[0]=="end"&&t.size()==1){if(stack.isEmpty())return fail("Unmatched end.");auto b=stack.takeLast();if(b.jump>=0)out[b.jump].a=out.size();else out[b.test].d=out.size();continue;}
        else if(t[0]=="seed"&&t.size()==2){bool ok;int n=t[1].toInt(&ok);code.op=CF_OP_SEED;if(ok){if(n<0||n>65535)return fail("Seed literals must be 0-65535.");code.value=int16_t(n);}else if(!value(t[1],code.value,code.c,error))return fail(error);}
        else if(t[0]=="timer"&&t.size()==3){int index=-1;for(int i=0;i<project.timers.size();++i)if(project.timers[i].id==t[2])index=i;int action=QStringList{"start","stop","reset"}.indexOf(t[1]);if(index<0||action<0)return fail("Use timer start/stop/reset followed by a declared timer ID.");code.op=CF_OP_TIMER;code.a=index;code.b=action;}
        else if(t[0]=="random"&&t.size()==4){int v=variable(t[1]);bool a,b;int low=t[2].toInt(&a),high=t[3].toInt(&b);if(v<0||v==31||!a||!b||low<0||high<low||high>32767)return fail("Use random variable minimum maximum (0-32767).");code.op=CF_OP_RANDOM;code.a=v;code.value=low;code.b=high&255;code.c=high>>8;}
        else if(t[0]=="resume"&&t.size()==1){code.op=CF_OP_RESUME;}
        else if((t[0]=="set"||t[0]=="add"||t[0]=="sub")&&t.size()==3){int v=variable(t[1]);if(v<0||v==31)return fail("Choose a declared variable; result is read-only.");code.op=t[0]=="set"?CF_OP_SET:t[0]=="sub"?CF_OP_SUB:CF_OP_ADD;code.a=v;if(!value(t[2],code.value,code.c,error))return fail(error);}
        else if((t[0]=="give"||t[0]=="take")&&t.size()==4){int s=project.storageIndex(t[1]),i=project.itemIndex(t[2]);if(s<0||i<0)return fail("Unknown storage or item.");code.op=t[0]=="give"?CF_OP_GIVE:CF_OP_TAKE;code.a=s;code.b=i;if(!value(t[3],code.value,code.c,error))return fail(error);}
        else if(t[0]=="move"&&t.size()==5){int a=project.storageIndex(t[1]),b=project.storageIndex(t[2]),i=project.itemIndex(t[3]);if(a<0||b<0||i<0)return fail("Unknown storage or item.");code.op=CF_OP_MOVE;code.a=a;code.b=b;code.c=i;if(!value(t[4],code.value,code.d,error))return fail(error);}
        else if((t[0]=="open"||t[0]=="view")&&t.size()==2){int s=project.storageIndex(t[1]);if(s<0)return fail("Unknown storage.");code.op=t[0]=="view"?CF_OP_VIEW:CF_OP_OPEN;code.a=s;}
        else if(t[0]=="dialogue"&&t.size()==2){int p=project.indexOf(t[1]);if(p<0)return fail("Unknown prompt.");code.op=CF_OP_DIALOGUE;code.a=p;}
        else return fail("Unknown command or argument count: "+line);
        out<<code;if(out.size()>96)return fail("A compiled event may contain at most 96 instructions.");
    }
    if(!stack.isEmpty()){error="Missing end for if.";return false;}return true;
}
QString encodeGameText(const Project &p,const QString &text,QString *error){QString out;QRegularExpression fields("\\{([A-Za-z_][A-Za-z0-9_]*)\\}");int offset=0;auto matches=fields.globalMatch(text);while(matches.hasNext()){auto m=matches.next();out+=text.mid(offset,m.capturedStart()-offset);int index=m.captured(1)=="result"?31:p.variableIndex(m.captured(1));if(index<0){if(error)*error="Unknown text variable: "+m.captured(1);out+="?";}else{out+=QChar(31);out+=QChar(index+1);}offset=m.capturedEnd();}return out+text.mid(offset);}
QString renderGameText(const Project &p,const CFGameState &state,const QString &text,int columns){auto encoded=encodeGameText(p,text).toLatin1();char out[1024];cf_game_text(&state,encoded.constData(),out,sizeof out,columns);return QString::fromLatin1(out);}

}
