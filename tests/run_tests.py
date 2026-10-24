# run_tests.py
import os
import sys
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))


def run_cmd(cmd):
    try:
        res = subprocess.run(cmd, shell=False, cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        return res.returncode, res.stdout.decode('utf-8', errors='replace'), res.stderr.decode('utf-8', errors='replace')
    except Exception as e:
        return -1, "", str(e)


def test_lexer_basicos():
    from toy.lexer import Lexer, LexError
    l = Lexer("else end name")
    t = l.todos()
    assert t[0].kind == "TOK-ELSE"
    assert t[1].kind == "TOK-END"
    assert t[2].kind == "TOK-ID"
    l = Lexer(":= == ~= <= >= < > ~")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ASSIGN", "TOK-EQ", "TOK-NEQ", "TOK-LTE", "TOK-GTE", "TOK-LT", "TOK-GT", "TOK-NOT"]
    l = Lexer("3.14 42")
    t = l.todos()
    assert t[0].kind == "TOK-NUMBER" and t[0].lexeme == "3.14"
    assert t[1].kind == "TOK-NUMBER" and t[1].lexeme == "42"
    l = Lexer('"hola" \'mundo\'')
    t = l.todos()
    assert t[0].kind == "TOK-STRING" and t[0].lexeme == "hola"
    assert t[1].kind == "TOK-STRING" and t[1].lexeme == "mundo"
    l = Lexer("x -- comentario\n y")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ID", "TOK-ID"]
    l = Lexer("main")
    t = l.todos()
    assert t[0].kind == "TOK-MAIN"
    print("OK lexer_basicos")


def test_lexer_errores():
    from toy.lexer import Lexer, LexError
    casos = []
    try:
        Lexer("a = b").todos()
    except LexError as e:
        casos.append(1)
    try:
        Lexer("a : b").todos()
    except LexError as e:
        casos.append(2)
    try:
        Lexer('"sin cerrar').todos()
    except LexError as e:
        assert "cadena" in e.mensaje
        casos.append(3)
    try:
        Lexer("'sin cerrar").todos()
    except LexError as e:
        assert "cadena" in e.mensaje
        casos.append(4)
    try:
        Lexer('5.').todos()
    except LexError as e:
        assert "número" in e.mensaje
        casos.append(5)
    try:
        Lexer('@').todos()
    except LexError as e:
        casos.append(6)
    try:
        Lexer('"hola\nmundo"').todos()
    except LexError as e:
        assert "cadena" in e.mensaje
        casos.append(7)
    assert len(casos) == 7
    print("OK lexer_errores")


def test_parser_ok():
    from toy.parser import Parser
    from toy.lexer import Lexer
    ok_minimo = ROOT / "ejemplos" / "ok_minimo.toy"
    with open(ok_minimo, "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    ok_completo = ROOT / "ejemplos" / "ok_completo.toy"
    with open(ok_completo, "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    print("OK parser_ok")


def test_precedencia():
    from toy.parser import Parser
    from toy.lexer import Lexer
    Parser(Lexer("func main()\n ret 2+3*4\nend main()")).parse()
    Parser(Lexer("func main()\n local x := 1\n x := a-b-c\n ret x\nend main()")).parse()
    Parser(Lexer("func main()\n local x := 2^3^2\n ret x\nend main()")).parse()
    Parser(Lexer("func main()\n local x := -2^2\n ret x\nend main()")).parse()
    Parser(Lexer("func main()\n local x := 2^-1\n ret x\nend main()")).parse()
    Parser(Lexer("func main()\n local x := a or b and c\n ret x\nend main()")).parse()
    Parser(Lexer("func main()\n local x := 6/2%4\n ret x\nend main()")).parse()
    print("OK precedencia_asociatividad")


def test_parser_invalidos():
    from toy.parser import Parser
    from toy.lexer import Lexer
    try:
        Parser(Lexer("func foo()\n ret 1\nend foo()")).parse()
        raise AssertionError("debería fallar sin main")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n ret 1\n local x := 2\nend main()")).parse()
        raise AssertionError("debería fallar ret en medio")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n ret 1\nend main()\n local x := 2")).parse()
        raise AssertionError("debería fallar código después")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n if x then\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar falta end/then")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n x :=\nend main()")).parse()
        raise AssertionError("debería fallar asignación sin expr")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n 5 := 1\nend main()")).parse()
        raise AssertionError("debería fallar sentencia con número")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n x\nend main()")).parse()
        raise AssertionError("debería fallar ID suelto")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n for i := 1, do\n end\nend main()")).parse()
        raise AssertionError("debería fallar for")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n ret (1+2\nend main()")).parse()
        raise AssertionError("debería fallar paréntesis")
    except Exception:
        pass
    try:
        Parser(Lexer("func main(x)\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar main con params")
    except Exception:
        pass
    try:
        Parser(Lexer("func main()\n ret 1+\nend main()")).parse()
        raise AssertionError("debería fallar operador sin operando")
    except Exception:
        pass
    print("OK parser_invalidos")


def test_ll1():
    orig = ROOT / "gramatica" / "toy_gramatica_original.txt"
    corr = ROOT / "gramatica" / "toy_gramatica.txt"
    res1 = subprocess.run([sys.executable, "-m", "toy.ll1_check", str(orig)], cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out1 = res1.stdout.decode('utf-8', errors='replace')
    conflictos = out1.count("Conflicto")
    assert conflictos == 2, f"esperados 2 conflictos, got {conflictos}: {out1}"
    res2 = subprocess.run([sys.executable, "-m", "toy.ll1_check", str(corr)], cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out2 = res2.stdout.decode('utf-8', errors='replace')
    assert "ninguno" in out2
    print("OK ll1")


def main():
    test_lexer_basicos()
    test_lexer_errores()
    test_parser_ok()
    test_precedencia()
    test_parser_invalidos()
    test_ll1()
    print("\nTODAS LAS PRUEBAS PASARON")
    sys.exit(0)


if __name__ == "__main__":
    main()
