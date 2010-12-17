/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "QtContextKeywords.h"

using namespace CPlusPlus;

static inline int classify4(const char *s) {
  if (s[0] == 'R') {
    if (s[1] == 'E') {
      if (s[2] == 'A') {
        if (s[3] == 'D') {
          return Token_READ;
        }
      }
    }
  }
  else if (s[0] == 'U') {
    if (s[1] == 'S') {
      if (s[2] == 'E') {
        if (s[3] == 'R') {
          return Token_USER;
        }
      }
    }
  }
  return Token_not_Qt_context_keyword;
}

static inline int classify5(const char *s) {
  if (s[0] == 'F') {
    if (s[1] == 'I') {
      if (s[2] == 'N') {
        if (s[3] == 'A') {
          if (s[4] == 'L') {
            return Token_FINAL;
          }
        }
      }
    }
  }
  else if (s[0] == 'R') {
    if (s[1] == 'E') {
      if (s[2] == 'S') {
        if (s[3] == 'E') {
          if (s[4] == 'T') {
            return Token_RESET;
          }
        }
      }
    }
  }
  else if (s[0] == 'W') {
    if (s[1] == 'R') {
      if (s[2] == 'I') {
        if (s[3] == 'T') {
          if (s[4] == 'E') {
            return Token_WRITE;
          }
        }
      }
    }
  }
  return Token_not_Qt_context_keyword;
}

static inline int classify6(const char *s) {
  if (s[0] == 'N') {
    if (s[1] == 'O') {
      if (s[2] == 'T') {
        if (s[3] == 'I') {
          if (s[4] == 'F') {
            if (s[5] == 'Y') {
              return Token_NOTIFY;
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'S') {
    if (s[1] == 'T') {
      if (s[2] == 'O') {
        if (s[3] == 'R') {
          if (s[4] == 'E') {
            if (s[5] == 'D') {
              return Token_STORED;
            }
          }
        }
      }
    }
  }
  return Token_not_Qt_context_keyword;
}

static inline int classify8(const char *s) {
  if (s[0] == 'C') {
    if (s[1] == 'O') {
      if (s[2] == 'N') {
        if (s[3] == 'S') {
          if (s[4] == 'T') {
            if (s[5] == 'A') {
              if (s[6] == 'N') {
                if (s[7] == 'T') {
                  return Token_CONSTANT;
                }
              }
            }
          }
        }
      }
    }
  }
  return Token_not_Qt_context_keyword;
}

static inline int classify10(const char *s) {
  if (s[0] == 'D') {
    if (s[1] == 'E') {
      if (s[2] == 'S') {
        if (s[3] == 'I') {
          if (s[4] == 'G') {
            if (s[5] == 'N') {
              if (s[6] == 'A') {
                if (s[7] == 'B') {
                  if (s[8] == 'L') {
                    if (s[9] == 'E') {
                      return Token_DESIGNABLE;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  else if (s[0] == 'S') {
    if (s[1] == 'C') {
      if (s[2] == 'R') {
        if (s[3] == 'I') {
          if (s[4] == 'P') {
            if (s[5] == 'T') {
              if (s[6] == 'A') {
                if (s[7] == 'B') {
                  if (s[8] == 'L') {
                    if (s[9] == 'E') {
                      return Token_SCRIPTABLE;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Token_not_Qt_context_keyword;
}

int CPlusPlus::classifyQtContextKeyword(const char *s, int n) {
  switch (n) {
    case 4: return classify4(s);
    case 5: return classify5(s);
    case 6: return classify6(s);
    case 8: return classify8(s);
    case 10: return classify10(s);
    default: return Token_not_Qt_context_keyword;
  } // switch
}
