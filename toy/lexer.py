# toy_lexer.py
from dataclasses import dataclass


class LexError(Exception):
    def __init__(self, line, col, mensaje):
        self.line = line
        self.col = col
        self.mensaje = mensaje
        super().__init__(f"Línea {line}, columna {col}: {mensaje}")


@dataclass
class Token:
    kind: str
    lexeme: str
    line: int
    col: int


TOK_EOF = "TOK-EOF"

PALABRAS_RESERVADAS = {
    "if": "TOK-IF",
    "then": "TOK-THEN",
    "else": "TOK-ELSE",
    "true": "TOK-TRUE",
    "false": "TOK-FALSE",
    "and": "TOK-AND",
    "or": "TOK-OR",
    "while": "TOK-WHILE",
    "do": "TOK-DO",
    "for": "TOK-FOR",
    "func": "TOK-FUNC",
    "end": "TOK-END",
    "main": "TOK-MAIN",
    "local": "TOK-LOCAL",
    "print": "TOK-PRINT",
    "println": "TOK-PRINTLN",
    "ret": "TOK-RET",
    "number": "TOK-KW-NUMBER",
    "string": "TOK-KW-STRING",
    "bool": "TOK-KW-BOOL",
}


class Lexer:
    def __init__(self, texto):
        self.texto = texto
        self.i = 0
        self.line = 1
        self.col = 1
        self.n = len(texto)

    def _char(self):
        if self.i >= self.n:
            return None
        return self.texto[self.i]

    def _avanzar(self):
        c = self._char()
        if c is None:
            self.i += 1
            return None
        self.i += 1
        if c == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return c

    def siguiente(self):
        while True:
            c = self._char()
            if c is None:
                return Token(TOK_EOF, "", self.line, self.col)
            if c in [" ", "\t", "\r", "\v", "\f"]:
                self._avanzar()
                continue
            if c == "\n":
                self._avanzar()
                continue
            if c == "-":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "-":
                    self._avanzar()
                    while True:
                        cc = self._char()
                        if cc is None or cc == "\n":
                            break
                        self._avanzar()
                    continue
                else:
                    return Token("TOK-MINUS", "-", start_line, start_col)
            if c == "+":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-PLUS", "+", start_line, start_col)
            if c == "*":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-MUL", "*", start_line, start_col)
            if c == "/":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-DIV", "/", start_line, start_col)
            if c == "%":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-MOD", "%", start_line, start_col)
            if c == "^":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-EXP", "^", start_line, start_col)
            if c == "(":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-LPAR", "(", start_line, start_col)
            if c == ")":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-RPAR", ")", start_line, start_col)
            if c == ",":
                start_line, start_col = self.line, self.col
                self._avanzar()
                return Token("TOK-COMMA", ",", start_line, start_col)
            if c == ".":
                start_line, start_col = self.line, self.col
                self._avanzar()
                raise LexError(start_line, start_col, "carácter '.' no válido")
            if c == ">":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "=":
                    self._avanzar()
                    return Token("TOK-GTE", ">=", start_line, start_col)
                return Token("TOK-GT", ">", start_line, start_col)
            if c == "<":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "=":
                    self._avanzar()
                    return Token("TOK-LTE", "<=", start_line, start_col)
                return Token("TOK-LT", "<", start_line, start_col)
            if c == "=":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "=":
                    self._avanzar()
                    return Token("TOK-EQ", "==", start_line, start_col)
                raise LexError(start_line, start_col, "'=' solo no es válido, use '==' o ':='")
            if c == "~":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "=":
                    self._avanzar()
                    return Token("TOK-NEQ", "~=", start_line, start_col)
                return Token("TOK-NOT", "~", start_line, start_col)
            if c == ":":
                start_line, start_col = self.line, self.col
                self._avanzar()
                c2 = self._char()
                if c2 == "=":
                    self._avanzar()
                    return Token("TOK-ASSIGN", ":=", start_line, start_col)
                raise LexError(start_line, start_col, "':' solo no es válido, use ':='")
            if c == '"':
                return self._leer_cadena('"')
            if c == "'":
                return self._leer_cadena("'")
            if self._es_letra(c):
                return self._leer_identificador()
            if self._es_digito(c):
                return self._leer_numero()
            start_line, start_col = self.line, self.col
            self._avanzar()
            raise LexError(start_line, start_col, f"carácter '{c}' no válido")

    def _es_letra(self, c):
        if c is None:
            return False
        return ('a' <= c <= 'z') or ('A' <= c <= 'Z') or c == '_'

    def _es_digito(self, c):
        if c is None:
            return False
        return '0' <= c <= '9'

    def _leer_identificador(self):
        start_line, start_col = self.line, self.col
        lex = []
        while True:
            c = self._char()
            if c is not None and (self._es_letra(c) or self._es_digito(c)):
                lex.append(c)
                self._avanzar()
            else:
                break
        lexeme = ''.join(lex)
        if lexeme in PALABRAS_RESERVADAS:
            kind = PALABRAS_RESERVADAS[lexeme]
        else:
            kind = "TOK-ID"
        return Token(kind, lexeme, start_line, start_col)

    def _leer_numero(self):
        start_line, start_col = self.line, self.col
        lex = []
        while True:
            c = self._char()
            if c is not None and self._es_digito(c):
                lex.append(c)
                self._avanzar()
            else:
                break
        c = self._char()
        if c == '.':
            self._avanzar()
            c2 = self._char()
            if c2 is not None and self._es_digito(c2):
                lex.append('.')
                while True:
                    c3 = self._char()
                    if c3 is not None and self._es_digito(c3):
                        lex.append(c3)
                        self._avanzar()
                    else:
                        break
                return Token("TOK-NUMBER", ''.join(lex), start_line, start_col)
            else:
                raise LexError(start_line, start_col, "número mal formado")
        return Token("TOK-NUMBER", ''.join(lex), start_line, start_col)

    def _leer_cadena(self, comilla):
        start_line, start_col = self.line, self.col
        self._avanzar()  # consumir comilla inicial
        lex = []
        while True:
            c = self._char()
            if c is None:
                raise LexError(start_line, start_col, "cadena sin cerrar")
            if c == '\n':
                raise LexError(start_line, start_col, "cadena sin cerrar")
            if c == comilla:
                self._avanzar()  # consumir comilla final
                break
            lex.append(c)
            self._avanzar()
        return Token("TOK-STRING", ''.join(lex), start_line, start_col)

    def todos(self):
        tokens = []
        while True:
            t = self.siguiente()
            tokens.append(t)
            if t.kind == TOK_EOF:
                break
        return tokens
