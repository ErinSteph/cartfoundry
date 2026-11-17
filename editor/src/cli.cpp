#include "builder.h"
#include <QCoreApplication>
#include <QTextStream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv); QTextStream out(stdout), err(stderr);
    auto args = app.arguments();
    if (args.size() == 3 && (args[1] == "--example-overworld" || args[1]=="--example-pokemon" || args[1]=="--example-scenes")) {
        QString error; if (!(args[1]=="--example-scenes"?cf::Project::scenesExample():args[1]=="--example-pokemon"?cf::Project::pokemonExample():cf::Project::overworldExample()).save(args[2],error)) { err << error << '\n'; return 1; } return 0;
    }
    if (args.size() != 3) { err << "Usage: cf-build project.cartfoundry output.gb[c]\n       cf-build --example-overworld game.cartfoundry\n"; return 2; }
    cf::Project p; QString error;
    if (!cf::Project::load(args[1], p, error)) { err << error << '\n'; return 1; }
    auto r = cf::buildRom(p, args[2]); out << r.log << '\n'; return r.success ? 0 : 1;
}
