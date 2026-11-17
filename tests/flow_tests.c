#include "cf_flow.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    CFPrompt p[] = {
        {"start", "", 2, {"a", "b", ""}, {1, 2, 255}},
        {"middle", "", 1, {"loop", "", ""}, {0, 255, 255}},
        {"end", "", 0, {"", "", ""}, {255, 255, 255}}
    };
    CFState s = {0, 0};
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_UP)); CHECK(s.choice == 1);
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_ACCEPT)); CHECK(s.prompt == 2 && s.choice == 0);
    CHECK(!cf_step(&s, p, 3, 0, CF_KEY_ACCEPT));
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_RESET)); CHECK(s.prompt == 0);
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_ACCEPT)); CHECK(s.prompt == 1);
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_ACCEPT)); CHECK(s.prompt == 0);
    p[0].targets[0] = 2; /* Editor rewiring changes the exact same runtime transition. */
    CHECK(cf_step(&s, p, 3, 0, CF_KEY_ACCEPT)); CHECK(s.prompt == 2);
    s.prompt = 0; p[0].targets[0] = 255; CHECK(!cf_step(&s, p, 3, 0, CF_KEY_ACCEPT)); CHECK(s.prompt == 0);
    CHECK(!cf_step(&s, p, 0, 0, CF_KEY_RESET));
    puts("Runtime choices, loops, endings, reset, rewiring and invalid-target checks passed.");
    return 0;
}
