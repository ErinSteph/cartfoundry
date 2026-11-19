#include "builder.h"
#include <QCoreApplication>
#include <QTextStream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv); QTextStream out(stdout), err(stderr);
    auto args = app.arguments();
    if (args.size() == 3 && (args[1] == "--example-overworld" || args[1]=="--example-pokemon" || args[1]=="--example-scenes" || args[1]=="--example-sound" || args[1]=="--example-menus")) {
        QString error; if(args[1]=="--example-menus"){if(!cf::Project::menusExample().save(args[2],error)){err<<error<<'\n';return 1;}return 0;} if(args[1]=="--example-sound"){auto p=cf::Project::example();p.title="Foundry Nights";p.sounds<<cf::foundryDemoSong();p.scenes[0].onSceneEnter="sound loop foundry_four";p.scenes[0].elements.clear();cf::ScreenElement title;title.id="song_title";title.name="Song title";title.type="text";title.text="FOUNDRY AFTER HOURS";title.rect={0,3,20,1};p.scenes[0].elements<<title;title.id="song_info";title.name="Channels";title.text="4 CHANNEL CHIPTUNE\nPULSE 1: LEAD\nPULSE 2: ARPEGGIO\nWAVE: BASS\nNOISE: DRUMS";title.rect={1,6,18,5};p.scenes[0].elements<<title;if(!p.save(args[2],error)){err<<error<<'\n';return 1;}return 0;} if (!(args[1]=="--example-scenes"?cf::Project::scenesExample():args[1]=="--example-pokemon"?cf::Project::pokemonExample():cf::Project::overworldExample()).save(args[2],error)) { err << error << '\n'; return 1; } return 0;
    }
    if (args.size() != 3) { err << "Usage: cf-build project.cartfoundry output.gb[c]\n       cf-build --example-overworld game.cartfoundry\n"; return 2; }
    cf::Project p; QString error;
    if (!cf::Project::load(args[1], p, error)) { err << error << '\n'; return 1; }
    auto r = cf::buildRom(p, args[2]); out << r.log << '\n'; return r.success ? 0 : 1;
}
