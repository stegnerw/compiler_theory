#ifndef CHAR_TABLE_H
#define CHAR_TABLE_H

enum CharType {
  C_INVALID,  // Anything not listed below - the default
  C_UPPER,  // [A-Z]
  C_LOWER,  // [a-z]
  C_DIGIT,  // [0-9]
  C_PERIOD,  // .
  C_UNDER,  // _
  C_SEMICOL,  // ;
  C_COLON,  // :
  C_COMMA,  // ,
  C_LPAREN,  // (
  C_RPAREN,  // )
  C_LBRACK,  // [
  C_RBRACK,  // ]
  C_EXPR,    // & |
  C_RELAT,  // < > = !
  C_ARITH,  // + -
  C_TERM,    // / *
  C_QUOTE,  // "
  C_WHITE,  // space, tab, newline, carriage return
  C_EOF    // EOF char
};

class CharTable {
  public:
    CharTable();
    CharType getCharType(int);
  private:
    CharType char_type_table[128];
};

#endif // CHAR_TABLE_H
