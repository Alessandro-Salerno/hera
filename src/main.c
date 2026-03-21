#include <ergen/lexer.h>

int main(void) {
    ER_String      s = (ER_String){0};
    ER_LexerResult r = er_lexer_run(s);
    ER_RESULT_UNWRAP(r, "generation stopped");
    return 0;
}
