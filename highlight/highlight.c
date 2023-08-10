#include <stdio.h>

#define HL_NUMBERS (1 << 0)
#define HL_STRINGS (1 << 1)

typedef struct syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singlelineCommentStart;
    char *multilineCommentStart;
    char *multilineCommentEnd;
    int flags;
} syntax;

char *ARSON_extensions[] = {".ars", NULL};
char *ARSON_keywords[] = {"burn", "for",  "through", "while", "prepmatch",
                          "if",   "else", "return",  "True|", "False|",
                          "int|", "str|", "float|",  "bool|", NULL};

char *C_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_keywords[] = {
    "auto",      "break",  "case",   "const",   "continue", "default",
    "do",        "else",   "extern", "for",     "goto",     "if",
    "register",  "return", "sizeof", "static",  "switch",   "typedef",
    "volatile",  "while",  "char|",  "double|", "enum|",    "float|",
    "int|",      "long|",  "short|", "signed|", "struct|",  "union|",
    "unsigned|", "void|",  NULL};

char *CSS_extensions[] = {".css", ".scss", NULL};
char *CSS_keywords = {"accent-color", "acos", "abs|"};

char *HTML_extensions[] = {};
char *HTML_keywords = {};

char *JS_extensions[] = {};
char *JS_keywords = {};

char *MD_extensions[] = {};
char *MD_keywords = {};

char *PY_extensions[] = {".py", NULL};
char *PY_keywords = {"await",
                     "else",
                     "import",
                     "pass",
                     "break",
                     "except",
                     "in",
                     "raise",
                     "True",
                     "class",
                     "finally",
                     "is",
                     "return",
                     "and",
                     "continue",
                     "for",
                     "lambda",
                     "try",
                     "as",
                     "def",
                     "from",
                     "nonlocal",
                     "while",
                     "assert",
                     "del",
                     "global",
                     "not",
                     "with",
                     "async",
                     "elif",
                     "if",
                     "or",
                     "yield"
                     "False|",
                     "None|",
                     "str|",
                     "int|",
                     "float|",
                     "complex|",
                     "list|",
                     "tuple|",
                     "range|",
                     "dict|",
                     "set|",
                     "frozenset|",
                     "bool|",
                     "bytes|",
                     "bytearray|",
                     "memoryview|",
                     NULL};

char *RUST_extensions[] = {".rs", NULL};
char *RUST_keywords = {
    "as",    "use",  "extern_crate", "break",  "const", "continue",
    "crate", "else", "if",           "if let", "enum",  "extern",
    "fn",    "for",  "impl",         "in",     "let",   "loop",
    "match", "mod",  "move",         "false|", NULL};

syntax HLDB[] = {
    {"arson", ARSON_extensions, ARSON_keywords, "#", NULL, NULL,
     HL_NUMBERS | HL_STRINGS},
    {"c", C_extensions, C_keywords, "//", "/*", "*/", HL_NUMBERS | HL_STRINGS}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))
