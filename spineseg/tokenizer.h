/*

    SpineSeg
    http://www.lni.hc.unicamp.br/app/spineseg
    https://github.com/fbergo/braintools
    Copyright (C) 2009-2017 Felipe P.G. Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

// classes here: Tokenizer

#ifndef TOKENIZER_H
#define TOKENIZER_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

using namespace std;

//! String Tokenizer
class Tokenizer {
public:

  //! Constructor, creates a tokenizer with no string and blanks as separator.
  //! See \ref setSeparator(const char *s).
  Tokenizer() {
    src = NULL;
    ptr = NULL;
    sep = NULL;
    token = NULL;
    setSeparator();
  }
  
  //! Constructor, creates a tokenizer with string s and the given
  //! set of separators. If separator is NULL, blanks are the separators.
  //! See \ref setSeparator(const char *s).
  Tokenizer(const char *s, const char *separator=NULL) {
    src = NULL;
    ptr = NULL;
    sep = NULL;
    token = NULL;
    setString(s);
    setSeparator(separator);
  }


  //! Destructor
  ~Tokenizer() {
    if (src!=NULL) free(src); 
    if (sep!=NULL) free(sep);
    if (token!=NULL) free(token);
  }

  //! Returns the next token, or NULL if there are no more tokens.
  char * nextToken() {
    char *start;
    int i;

    while(*ptr != 0 && strchr(sep,*ptr)!=NULL)
      ++ptr;

    if (*ptr == 0) return NULL; // no more tokens
    start = ptr;

    while(*ptr != 0 && strchr(sep,*ptr)==NULL)
      ++ptr;

    if (token!=NULL) { free(token); token = NULL; }
    token = (char *) malloc(1+ptr-start);
    if (token==NULL) return NULL;
    
    for(i=0;i<ptr-start;i++)
      token[i] = start[i];
    token[ptr-start] = 0;
    return token;
  }

  //! Returns the next token interpreted as an integer, or 0 if there
  //! are no more tokens left.
  int nextInt() {
    char *x;
    x = nextToken();
    if (x==NULL) return 0; else return(atoi(x));
  }

  //! Returns the next token interpreted as a double, or 0 if there
  //! are no more tokens left.
  double nextDouble() {
    char *x;
    x = nextToken();
    if (x==NULL) return 0; else return(atof(x));
  }

  //! Returns the next token interpreted as a float, or 0 if there
  //! are no more tokens left.
  float  nextFloat() {
    return((float)nextDouble());
  }

  //! Sets the string to be tokenized to s, and places the tokenization
  //! cursor at the start of s.
  void setString(const char *s) {
    if (src!=NULL) { free(src); src=NULL; }
    src = strdup(s);
    ptr = src;
  }

  //! Sets the set of token separator characters. If set to NULL, the blanks
  //! string (" \t\n\r") is used.
  void setSeparator(const char *separator=NULL) {
    if (sep!=NULL) { free(sep); sep=NULL; }
    if (separator!=NULL) sep = strdup(separator); else sep = strdup(" \t\n\r");
  }

  //! Rewinds the tokenization cursor to the start of the current subject
  //! string.
  void rewind() {
    ptr = src;
  }

  //! Returns the number of tokens left
  int countTokens() {
    int n;
    char *ptrcopy;
    ptrcopy=ptr;
    n = 0;
    while(nextToken()!=NULL) ++n;
    ptr = ptrcopy;
    return n;
  }

private:
  char *src,*ptr,*sep,*token;
};

#endif

