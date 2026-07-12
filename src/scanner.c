// External scanner for Hone's block comments.
//
// Hone block comments nest: `/* outer /* inner */ still outer */`. Nesting is not a
// regular language, so a tree-sitter regex token cannot express it — it would stop at
// the first `*/` and leave the tail of the outer comment to be parsed as code. This
// scanner keeps a depth counter, which is what honec's lexer does.
//
// Stateless between tokens: a comment is consumed whole in one scan, so serialize /
// deserialize have nothing to carry.

#include "tree_sitter/parser.h"

enum TokenType {
  BLOCK_COMMENT,
};

void *tree_sitter_hone_external_scanner_create(void) { return NULL; }

void tree_sitter_hone_external_scanner_destroy(void *payload) { (void)payload; }

unsigned tree_sitter_hone_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_hone_external_scanner_deserialize(void *payload, const char *buffer,
                                                   unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

bool tree_sitter_hone_external_scanner_scan(void *payload, TSLexer *lexer,
                                            const bool *valid_symbols) {
  (void)payload;

  if (!valid_symbols[BLOCK_COMMENT]) {
    return false;
  }

  // `true` marks the character as whitespace, keeping it out of the token's extent.
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\n' ||
         lexer->lookahead == '\r') {
    lexer->advance(lexer, true);
  }

  if (lexer->lookahead != '/') {
    return false;
  }
  lexer->advance(lexer, false);
  if (lexer->lookahead != '*') {
    return false;
  }
  lexer->advance(lexer, false);

  unsigned depth = 1;
  while (depth > 0) {
    if (lexer->eof(lexer)) {
      // Unterminated comment. Claim what we have: the alternative is to reject, which
      // would re-lex the body as code and produce a cascade of errors from one typo.
      break;
    }
    if (lexer->lookahead == '/') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '*') {
        lexer->advance(lexer, false);
        depth++;
      }
    } else if (lexer->lookahead == '*') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '/') {
        lexer->advance(lexer, false);
        depth--;
      }
    } else {
      lexer->advance(lexer, false);
    }
  }

  lexer->result_symbol = BLOCK_COMMENT;
  return true;
}
