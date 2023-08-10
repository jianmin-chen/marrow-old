#include "../config.h"
#include "../libs/ini.h"
#include "../status/error.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct colors {
    int background;
    int normal;
    int keyword;
    int type;
    int string;
    int number;
    int match;
    int comment;
} colors;

typedef struct syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singlelineCommentStart;
    char *multilineCommentStart;
    char *multilineCommentEnd;
    int flags;
} syntax;

enum highlight {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD,
    HL_TYPE,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

#ifdef MARROW_THEME
#else
colors DEFAULT_THEME = {-1, 37, 33, 32, 35, 31, 34, 36};
colors *theme = &DEFAULT_THEME;
char *themeName = "default";
#endif

char *ARSON_extensions[] = {".ars", NULL};
char *ARSON_keywords[] = {"burn", "for",  "through", "while", "prepmatch",
                          "if",   "else", "return",  "True|", "False|",
                          "int|", "str|", "float|",  "bool|", NULL};

char *C_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_keywords[] = {
    "#define",  "#include", "auto",      "break",  "case",   "const",
    "continue", "default",  "do",        "else",   "extern", "for",
    "goto",     "if",       "register",  "return", "sizeof", "static",
    "switch",   "typedef",  "volatile",  "while",  "char|",  "double|",
    "enum|",    "float|",   "int|",      "long|",  "short|", "signed|",
    "struct|",  "union|",   "unsigned|", "void|",  NULL};

char *CSS_extensions[] = {".css", ".scss", NULL};
char *CSS_keywords[] = {"accent-color", "acos", "abs|"};

char *HTML_extensions[] = {".html", ".ejs", NULL};
char *HTML_keywords[] = {NULL};

char *JS_extensions[] = {".js", ".ts", ".cjs", ".mjs", NULL};
char *JS_keywords[] = {
    "await",    "break",      "case",    "catch",   "class",      "const",
    "continue", "debugger",   "default", "delete",  "do",         "else",
    "enum",     "export",     "extends", "finally", "for",        "function",
    "if",       "implements", "import",  "in",      "instanceof", "interface",
    "let",      "new",        "package", "private", "protected",  "public",
    "return",   "super",      "switch",  "static",  "this",       "throw",
    "try",      "true",       "typeof",  "var",     "void",       "while",
    "with",     "yield",      "false|",  "null|",   "true|",      NULL};

char *PY_extensions[] = {".py", NULL};
char *PY_keywords[] = {"await",
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
char *RUST_keywords[] = {
    "as",    "use",  "extern_crate", "break",  "const", "continue",
    "crate", "else", "if",           "if let", "enum",  "extern",
    "fn",    "for",  "impl",         "in",     "let",   "loop",
    "match", "mod",  "move",         "false|", NULL};

syntax HLDB[] = {{"arson", ARSON_extensions, ARSON_keywords, "#", NULL, NULL,
                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
                 {"c", C_extensions, C_keywords, "//", "/*", "*/",
                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
                 {"js", JS_extensions, JS_keywords, "//", "/*", "*/",
                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
                 {"py", PY_extensions, PY_keywords, "#", "\"\"\"", "\"\"\"",
                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
                 {"rust", RUST_extensions, RUST_keywords, "//", "/*", "*/",
                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** syntax highlighting ***/

int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int syntaxToColor(colors *theme, int hl) {
    switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT:
        return theme->comment;
    case HL_KEYWORD:
        return theme->keyword;
    case HL_TYPE:
        return theme->type;
    case HL_STRING:
        return theme->string;
    case HL_NUMBER:
        return theme->number;
    case HL_MATCH:
        return theme->match;
    default:
        return theme->normal;
    }
}

syntax *selectSyntaxHighlight(char *filename, char *filetype) {
    if (filename == NULL)
        return NULL;
    
    syntax *s = malloc(sizeof(syntax));
    s->filetype = NULL;
    s->filematch = NULL;
    s->keywords = NULL;
    s->singlelineCommentStart = NULL;
    s->multilineCommentStart = NULL;
    s->multilineCommentEnd = NULL;

    s->filetype = filetype;

    char *ext = strrchr(filename, '.');
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        // Go through each HLDB entry to check which one matches
        syntax *curr = &HLDB[j];
        unsigned int i = 0;
        while (curr->filematch[i]) {
            int is_ext = (curr->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, curr->filematch[i])) ||
                (!is_ext && strstr(filename, curr->filematch[i]))) {
                s = curr;
                return s;
            }
            i++;
        }
    }

    return NULL;
}

int handler(void *c, const char *section, const char *name, const char *value) {
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH(themeName, "normal")) {
		theme->normal = atoi(value);
	} else if (MATCH(themeName, "keyword")) {
		theme->keyword = atoi(value);
	} else if (MATCH(themeName, "type")) {
		theme->type = atoi(value);
	} else if (MATCH(themeName, "string")) {
		theme->string = atoi(value);
	} else if (MATCH(themeName, "number")) {
		theme->number = atoi(value);
	} else if (MATCH(themeName, "match")) {
		theme->match = atoi(value);
	} else if (MATCH(themeName, "comment")) {
		theme->comment = atoi(value);
	}
	return 0;
}

// Given a syntax struct and an INI file containing the themes load the themes
// into syntax->themes
void loadTheme(char *name) {
	themeName = name;
	
	if (ini_parse(HL_HIGHLIGHT_LOCATION, handler, &theme) < 0) {
		die("Unable to load theme");
	}
}

