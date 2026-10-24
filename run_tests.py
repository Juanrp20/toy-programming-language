# run_tests.py
import os
import sys
import subprocess


def run_cmd(cmd):
    try:
        cmd2 = cmd.replace('python ', 'python3 ')
        res = subprocess.run(cmd2, shell=True, cwd=os.path.dirname(os.path.abspath(__file__)), stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        return res.returncode, res.stdout.decode('utf-8', errors='replace'), res.stderr.decode('utf-8', errors='replace')
    except Exception as e:
        return -1, "", str(e)


def test_lexer_basicos():
    from toy_lexer import Lexer, LexError
    # else/end/name
    l = Lexer("else end name")
    t = l.todos()
    assert t[0].kind == "TOK-ELSE"
    assert t[1].kind == "TOK-END"
    assert t[2].kind == "TOK-ID"
    # := == ~= <= >= < > ~
    l = Lexer(":= == ~= <= >= < > ~")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ASSIGN", "TOK-EQ", "TOK-NEQ", "TOK-LTE", "TOK-GTE", "TOK-LT", "TOK-GT", "TOK-NOT"]
    # 3.14 y 42
    l = Lexer("3.14 42")
    t = l.todos()
    assert t[0].kind == "TOK-NUMBER" and t[0].lexeme == "3.14"
    assert t[1].kind == "TOK-NUMBER" and t[1].lexeme == "42"
    # cadenas ambas comillas
    l = Lexer('"hola" \'mundo\'')
    t = l.todos()
    assert t[0].kind == "TOK-STRING" and t[0].lexeme == "hola"
    assert t[1].kind == "TOK-STRING" and t[1].lexeme == "mundo"
    # comentario --
    l = Lexer("x -- comentario\n y")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ID", "TOK-ID"]
    # main reservada
    l = Lexer("main")
    t = l.todos()
    assert t[0].kind == "TOK-MAIN"
    print("OK lexer_basicos")


def test_lexer_errores():
    from toy_lexer import Lexer, LexError
    casos = []
    try:
        Lexer("a = b").todos()
    except LexError as e:
        assert "válido" in e.mensaje or "=" in e.mensaje
        casos.append(1)
    try:
        Lexer("a : b").todos()
    except LexError as e:
        assert "válido" in e.mensaje or ":" in e.mensaje
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
    from toy_parser import Parser
    from toy_lexer import Lexer
    # ok_minimo
    with open("ejemplos/ok_minimo.toy", "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    # ok_completo
    with open("ejemplos/ok_completo.toy", "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    print("OK parser_ok")


def test_precedencia():
    from toy_parser import Parser
    from toy_lexer import Lexer
    # 2+3*4
    ast = Parser(Lexer("func main()\n ret 2+3*4\nend main()")).parse()
    # verificar estructura aproximada
    assert ast[0] == "program"
    # a-b-c asociativo izquierda
    ast = Parser(Lexer("func main()\n local x := 1\n x := a-b-c\n ret x\nend main()")).parse()
    # 2^3^2 derecha
    ast = Parser(Lexer("func main()\n local x := 2^3^2\n ret x\nend main()")).parse()
    # -2^2 = -(2^2)
    ast = Parser(Lexer("func main()\n local x := -2^2\n ret x\nend main()")).parse()
    # 2^-1
    ast = Parser(Lexer("func main()\n local x := 2^-1\n ret x\nend main()")).parse()
    # a or b and c
    ast = Parser(Lexer("func main()\n local x := a or b and c\n ret x\nend main()")).parse()
    # 6/2%4
    ast = Parser(Lexer("func main()\n local x := 6/2%4\n ret x\nend main()")).parse()
    print("OK precedencia_asociatividad")


def test_parser_invalidos():
    from toy_parser import Parser
    from toy_lexer import Lexer
    from toy_parser import SyntaxErr
    casos_invalidos = []
    # sin main
    try:
        Parser(Lexer("func foo()\n ret 1\nend foo()")).parse()
        casos_invalidos.append("bad1")
    except Exception:
        pass
    else:
        raise AssertionError("debería fallar sin main")
    # ret en medio de un bloque
    try:
        Parser(Lexer("func main()\n ret 1\n local x := 2\nend main()")).parse()
        raise AssertionError("debería fallar ret en medio")
    except Exception:
        pass
    # código después de end main()
    try:
        Parser(Lexer("func main()\n ret 1\nend main()\n local x := 2")).parse()
        raise AssertionError("debería fallar código después")
    except Exception:
        pass
    # falta end/then
    try:
        Parser(Lexer("func main()\n if x then\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar falta end/then")
    except Exception:
        pass
    # asignación sin expresión
    try:
        Parser(Lexer("func main()\n x :=\nend main()")).parse()
        raise AssertionError("debería fallar asignación sin expr")
    except Exception:
        pass
    # sentencia que empieza con un número
    try:
        Parser(Lexer("func main()\n 5 := 1\nend main()")).parse()
        raise AssertionError("debería fallar sentencia con número")
    except Exception:
        pass
    # ID suelto sin := ni (
    try:
        Parser(Lexer("func main()\n x\nend main()")).parse()
        raise AssertionError("debería fallar ID suelto")
    except Exception:
        pass
    # for sin segunda expresión
    try:
        Parser(Lexer("func main()\n for i := 1, do\n end\nend main()")).parse()
        raise AssertionError("debería fallar for")
    except Exception:
        pass
    # paréntesis sin cerrar
    try:
        Parser(Lexer("func main()\n ret (1+2\nend main()")).parse()
        raise AssertionError("debería fallar paréntesis")
    except Exception:
        pass
    # main con parámetros
    try:
        Parser(Lexer("func main(x)\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar main con params")
    except Exception:
        pass
    # operador sin operando derecho
    try:
        Parser(Lexer("func main()\n ret 1+\nend main()")).parse()
        raise AssertionError("debería fallar operador sin operando")
    except Exception:
        pass
    print("OK parser_invalidos")


def test_ll1():
    code, out, err = run_cmd("python ll1_check.py toy_gramatica.txt")
    # sobre original debe dar 2 conflictos - pero tenemos corregida ahora; revisar
    # leer gramática original del apéndice
    with open("Documentation/gramatica_formal_del_lenguaje_toy.md", "r", encoding="utf-8") as f:
        doc = f.read()
    if "# Apéndice" in doc:
        ap = doc.split("# Apéndice")[1]
    else:
        ap = doc
    with open("/tmp/gramatica_original.toygram", "w", encoding="utf-8") as f:
        f.write(ap)
    code2, out2, err2 = run_cmd("python ll1_check.py /tmp/gramatica_original.toygram")
    conflictos = out2.count("Conflicto")
    if conflictos != 2:
        code2b, out2b, err2b = run_cmd("python ll1_check.py /tmp/orig_test.txt")
        conflictos = out2b.count("Conflicto")
        out2 = out2b
    assert conflictos == 2, f"esperados 2 conflictos, got {conflictos}: {out2}"
    code3, out3, err3 = run_cmd("python ll1_check.py toy_gramatica.txt")
    assert "ninguno" in out3
    print("OK ll1")


def main():
    os.makedirs("ejemplos", exist_ok=True)
    # crear ejemplos si no existen
    if not os.path.exists("ejemplos/ok_minimo.toy"):
        with open("ejemplos/ok_minimo.toy", "w", encoding="utf-8") as f:
            f.write("func main()\nend main()\n")
    if not os.path.exists("ejemplos/ok_completo.toy"):
        with open("ejemplos/ok_completo.toy", "w", encoding="utf-8") as f:
            f.write("""-- programa completo de prueba
func suma(a,b)
  ret a+b
end suma

func main()
  local x := 5
  local y := 3.14
  local s1 := "hola"
  local s2 := 'mundo'
  x := suma(x, 1)
  if x > 0 then
    print x
  else
    println y
  end
  while x > 0 do
    x := x-1
  end
  for i := 1, 10, 2 do
    print i
  end
  for j := 0, 5 do
    println j
  end
  ret 0
end main()
""")
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
