# toy_parser.py
import sys
from toy.lexer import Lexer, Token, LexError, TOK_EOF


class SyntaxErr(Exception):
    def __init__(self, line, col, esperado, encontrado):
        self.line = line
        self.col = col
        self.esperado = esperado
        self.encontrado = encontrado
        super().__init__(f"Línea {line}, columna {col}: se esperaba {esperado}, se encontró {encontrado}")


class Parser:
    def __init__(self, lexer):
        self.lexer = lexer
        self.la = self.lexer.siguiente()

    def error(self, esperado):
        raise SyntaxErr(self.la.line, self.la.col, esperado, self.la.lexeme if self.la.lexeme != "" else self.la.kind)

    def comer(self, kind=None):
        if kind is not None and self.la.kind != kind:
            self.error(kind)
        tok = self.la
        self.la = self.lexer.siguiente()
        return tok

    def parse(self):
        ast = self.program()
        if self.la.kind != TOK_EOF:
            self.error("TOK-EOF")
        return ast

    def program(self):
        # <Program> -> "func" <FuncRest>
        self.comer("TOK-FUNC")
        return self.func_rest()

    def func_rest(self):
        if self.la.kind == "TOK-MAIN":
            # <FuncRest> -> "main" "(" ")" <Block> "end" "main" "(" ")"
            self.comer("TOK-MAIN")
            self.comer("TOK-LPAR")
            self.comer("TOK-RPAR")
            bloque = self.block()
            self.comer("TOK-END")
            self.comer("TOK-MAIN")
            self.comer("TOK-LPAR")
            self.comer("TOK-RPAR")
            return ("program", [("main", bloque)])
        elif self.la.kind == "TOK-ID":
            # | ID "(" [ <ParamList> ] ")" <Block> "end" <Program>
            nombre = self.comer("TOK-ID").lexeme
            self.comer("TOK-LPAR")
            params = []
            if self.la.kind == "TOK-ID":
                params = self.param_list()
            self.comer("TOK-RPAR")
            bloque = self.block()
            self.comer("TOK-END")
            resto = self.program()
            if isinstance(resto, tuple) and resto[0] == "program":
                return ("program", [("func", nombre, params, bloque)] + resto[1])
            return ("program", [("func", nombre, params, bloque), resto])
        else:
            self.error("'main' o ID")

    def param_list(self):
        params = []
        params.append(self.comer("TOK-ID").lexeme)
        while self.la.kind == "TOK-COMMA":
            self.comer("TOK-COMMA")
            params.append(self.comer("TOK-ID").lexeme)
        return params

    def block(self):
        stmts = []
        # { <Statement> }
        while True:
            if self.la.kind == "TOK-RET":
                break
            if self._es_statement_start(self.la):
                stmts.append(self.statement())
            else:
                break
        ret = None
        if self.la.kind == "TOK-RET":
            ret = self.ret_stmt()
        return ("block", stmts, ret)

    def _es_statement_start(self, la):
        k = la.kind
        if k in ["TOK-IF", "TOK-WHILE", "TOK-FOR", "TOK-PRINT", "TOK-PRINTLN", "TOK-LOCAL"]:
            return True
        if k == "TOK-ID":
            return True
        return False

    def statement(self):
        k = self.la.kind
        if k == "TOK-IF":
            return self.if_stmt()
        if k == "TOK-WHILE":
            return self.while_stmt()
        if k == "TOK-FOR":
            return self.for_stmt()
        if k == "TOK-PRINT" or k == "TOK-PRINTLN":
            return self.print_stmt()
        if k == "TOK-LOCAL":
            return self.local_stmt()
        if k == "TOK-ID":
            return self.id_stmt()
        self.error("sentencia")

    def if_stmt(self):
        self.comer("TOK-IF")
        cond = self.expr()
        self.comer("TOK-THEN")
        bloque1 = self.block()
        bloque2 = None
        if self.la.kind == "TOK-ELSE":
            self.comer("TOK-ELSE")
            bloque2 = self.block()
        self.comer("TOK-END")
        return ("if", cond, bloque1, bloque2)

    def while_stmt(self):
        self.comer("TOK-WHILE")
        cond = self.expr()
        self.comer("TOK-DO")
        bloque = self.block()
        self.comer("TOK-END")
        return ("while", cond, bloque)

    def for_stmt(self):
        self.comer("TOK-FOR")
        var = self.comer("TOK-ID").lexeme
        self.comer("TOK-ASSIGN")
        e1 = self.expr()
        self.comer("TOK-COMMA")
        if self.la.kind == "TOK-COMMA":
            # no hay segunda expresión en forma for sin paso? pero gramática dice [ "," <Expr> ]
            # pero test dice "for sin segunda expresión" debe rechazarse
            self.error("segunda expresión")
        e2 = self.expr()
        e3 = None
        if self.la.kind == "TOK-COMMA":
            self.comer("TOK-COMMA")
            if self.la.kind == "TOK-DO":
                self.error("segunda expresión")
            e3 = self.expr()
        self.comer("TOK-DO")
        bloque = self.block()
        self.comer("TOK-END")
        return ("for", var, e1, e2, e3, bloque)

    def print_stmt(self):
        if self.la.kind == "TOK-PRINT":
            self.comer("TOK-PRINT")
            kind = "print"
        else:
            self.comer("TOK-PRINTLN")
            kind = "println"
        e = self.expr()
        return (kind, e)

    def local_stmt(self):
        self.comer("TOK-LOCAL")
        var = self.comer("TOK-ID").lexeme
        self.comer("TOK-ASSIGN")
        e = self.expr()
        return ("local", var, e)

    def ret_stmt(self):
        self.comer("TOK-RET")
        if self._es_expr_start(self.la):
            e = self.expr()
            return ("ret", e)
        return ("ret", None)

    def id_stmt(self):
        nombre = self.comer("TOK-ID").lexeme
        if self.la.kind == "TOK-ASSIGN":
            self.comer("TOK-ASSIGN")
            e = self.expr()
            return ("assign", nombre, e)
        elif self.la.kind == "TOK-LPAR":
            self.comer("TOK-LPAR")
            args = []
            if self._es_expr_start(self.la):
                args = self.arg_list()
            self.comer("TOK-RPAR")
            return ("call_stmt", nombre, args)
        else:
            self.error("':=' o '('")

    def arg_list(self):
        args = [self.expr()]
        while self.la.kind == "TOK-COMMA":
            self.comer("TOK-COMMA")
            args.append(self.expr())
        return args

    def expr(self):
        return self.or_expr()

    def or_expr(self):
        node = self.and_expr()
        while self.la.kind == "TOK-OR":
            self.comer("TOK-OR")
            right = self.and_expr()
            node = ("or", node, right)
        return node

    def and_expr(self):
        node = self.eq_expr()
        while self.la.kind == "TOK-AND":
            self.comer("TOK-AND")
            right = self.eq_expr()
            node = ("and", node, right)
        return node

    def eq_expr(self):
        node = self.rel_expr()
        while self.la.kind == "TOK-EQ" or self.la.kind == "TOK-NEQ":
            op = self.la.kind
            self.comer(op)
            right = self.rel_expr()
            node = (op, node, right)
        return node

    def rel_expr(self):
        node = self.add_expr()
        while self.la.kind in ["TOK-LT", "TOK-LTE", "TOK-GT", "TOK-GTE"]:
            op = self.la.kind
            self.comer(op)
            right = self.add_expr()
            node = (op, node, right)
        return node

    def add_expr(self):
        node = self.mult_expr()
        while self.la.kind == "TOK-PLUS" or self.la.kind == "TOK-MINUS":
            op = self.la.kind
            self.comer(op)
            right = self.mult_expr()
            node = (op, node, right)
        return node

    def mult_expr(self):
        node = self.mod_expr()
        while self.la.kind == "TOK-MUL" or self.la.kind == "TOK-DIV":
            op = self.la.kind
            self.comer(op)
            right = self.mod_expr()
            node = (op, node, right)
        return node

    def mod_expr(self):
        node = self.unary_expr()
        while self.la.kind == "TOK-MOD":
            op = self.la.kind
            self.comer(op)
            right = self.unary_expr()
            node = (op, node, right)
        return node

    def unary_expr(self):
        if self.la.kind == "TOK-MINUS":
            self.comer("TOK-MINUS")
            node = self.unary_expr()
            return ("unary_minus", node)
        if self.la.kind == "TOK-NOT":
            self.comer("TOK-NOT")
            node = self.unary_expr()
            return ("unary_not", node)
        return self.exp_expr()

    def exp_expr(self):
        node = self.primary()
        if self.la.kind == "TOK-EXP":
            self.comer("TOK-EXP")
            right = self.unary_expr()  # right associative
            return ("TOK-EXP", node, right)
        return node

    def _es_expr_start(self, la):
        k = la.kind
        if k in ["TOK-NUMBER", "TOK-STRING", "TOK-TRUE", "TOK-FALSE", "TOK-LPAR", "TOK-ID", "TOK-MINUS", "TOK-NOT"]:
            return True
        return False

    def primary(self):
        k = self.la.kind
        if k == "TOK-NUMBER":
            t = self.comer("TOK-NUMBER")
            return ("number", t.lexeme)
        if k == "TOK-STRING":
            t = self.comer("TOK-STRING")
            return ("string", t.lexeme)
        if k == "TOK-TRUE":
            self.comer("TOK-TRUE")
            return ("bool", "true")
        if k == "TOK-FALSE":
            self.comer("TOK-FALSE")
            return ("bool", "false")
        if k == "TOK-LPAR":
            self.comer("TOK-LPAR")
            e = self.expr()
            self.comer("TOK-RPAR")
            return ("paren", e)
        if k == "TOK-ID":
            nombre = self.comer("TOK-ID").lexeme
            if self.la.kind == "TOK-LPAR":
                self.comer("TOK-LPAR")
                args = []
                if self._es_expr_start(self.la):
                    args = self.arg_list()
                self.comer("TOK-RPAR")
                return ("call", nombre, args)
            return ("id", nombre)
        self.error("expresión")


def main():
    if len(sys.argv) < 2:
        print("Uso: python -m toy.parser archivo.toy [--tokens] [--ast]")
        return
    filename = sys.argv[1]
    with open(filename, "r", encoding="utf-8") as f:
        texto = f.read()
    try:
        lexer = Lexer(texto)
        if "--tokens" in sys.argv:
            for t in lexer.todos():
                print(f"{t.kind} {t.lexeme} {t.line}:{t.col}")
            return
        parser = Parser(lexer)
        ast = parser.parse()
        if "--ast" in sys.argv:
            print(ast)
        else:
            print("OK")
    except Exception as e:
        print(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
