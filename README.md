# Toy

Toy es un lenguaje didáctico creado para el curso de Compiladores. El objetivo del proyecto es especificar formalmente un lenguaje (léxico con un AFD, sintaxis con una gramática libre de contexto en EBNF), demostrar que su gramática es LL(1) y construir las primeras fases de un compilador: analizador léxico y analizador sintáctico descendente recursivo.

## Ejemplo de programa Toy

```toy
-- programa completo de prueba
func suma(a,b)
  ret a+b
end

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
```

Explicación breve: el programa define funciones en ámbito global, termina con `func main() ... end main()` como punto de entrada, usa `local` para variables, estructuras de control `if/then/else/end`, `while/do/end`, `for` con paso opcional, `print`/`println`, `ret` y llamadas a funciones.

## Resumen del lenguaje

| Aspecto | Descripción |
|---|---|
| Estructura del programa | Secuencia de funciones `func nombre(params) ... end` terminada por `func main() ... end main()`. `main` es palabra reservada. |
| Variables | `local x := e` y asignación `x := e` mediante `:=`. |
| Control de flujo | `if`/`then`/`else`/`end`, `while`/`do`/`end`, `for` ID `:=` expr `,` expr [`,` expr] `do` ... `end` (paso opcional). |
| Salida | `print` / `println`. |
| Retorno | `ret` (solo al final de un bloque). |
| Llamadas | Llamadas a funciones como sentencia y en expresión. |
| Literales | Números enteros y decimales, cadenas con comillas simples o dobles, `true`/`false`. |
| Comentarios | `--` hasta fin de línea. |
| Palabras reservadas tipo | `number`, `string`, `bool` son palabras reservadas aún sin uso en la gramática. |

**Precedencia de operadores (de menor a mayor):** `or`; `and`; `==`, `~=`; `<`, `<=`, `>`, `>=`; `+`, `-`; `*`, `/`; `%`; unarios `-` y `~`; `^` (asociativa por la derecha).

## Fases del compilador

**Implementadas:**
- Analizador léxico escrito a mano, equivalente al AFD documentado en docs/.
- Analizador sintáctico descendente recursivo LL(1) que construye un AST.

**No implementadas:**
- Análisis semántico
- Verificación de tipos
- Generación de código / intérprete

## Verificación LL(1)

La gramática original presentaba dos conflictos:
- Conflicto en `<Program>`: `{ <FuncDecl> } <MainCall>` con ambos empezando por `"func"`, imposible de decidir con un token de lookahead.
- Conflicto en `<RetStmt>`: `<RetStmt> -> "ret" [ <Expr> ]` con `ID` en `PRIMERO(Expr)` y en `SIGUIENTE(RetStmt)`.

Se resolvieron:
- Factorizando el programa como `<Program> -> "func" <FuncRest>` distinguiendo `main` de otras funciones.
- Eliminando `<RetStmt>` de `<Statement>` y colocándolo opcional al final de `<Block>` (`<Block> -> { <Statement> } [ <RetStmt> ]`), de modo que `ret` solo aparece como última instrucción del bloque.

## Estructura del repositorio

```
toy/
├── README.md
├── .gitignore
├── docs/
│   ├── especificacion_y_diseno_formal_del_lenguaje_toy.md
│   ├── gramatica_formal_del_lenguaje_toy.md
│   └── gramatica_toy.md
├── gramatica/
│   ├── toy_gramatica.txt              (corregida, LL(1))
│   └── toy_gramatica_original.txt     (original, NO LL(1))
├── toy/
│   ├── __init__.py
│   ├── lexer.py
│   ├── parser.py
│   └── ll1_check.py
├── ejemplos/
│   ├── ok_completo.toy
│   └── ok_minimo.toy
└── tests/
    └── run_tests.py
```

## Cómo ejecutar

### Verificador LL(1)
```bash
python -m toy.ll1_check gramatica/toy_gramatica.txt [--sets]
python -m toy.ll1_check gramatica/toy_gramatica_original.txt  # debe reportar 2 conflictos
```

### Parser
```bash
python -m toy.parser archivo.toy
python -m toy.parser archivo.toy --tokens
python -m toy.parser archivo.toy --ast
```

### Pruebas
```bash
python tests/run_tests.py
```

## Cambios respecto a la gramática/especificación original

| Punto | Cambio |
|---|---|
| Conflicto en <Program> | Se reemplaza `<Program> -> { <FuncDecl> } <MainCall>` por `<Program> -> "func" <FuncRest>` con `<FuncRest>` factorizado entre `main` y función con ID. |
| Conflicto en <RetStmt> | Se elimina `<RetStmt>` de `<Statement>` y se pasa a `<Block> -> { <Statement> } [ <RetStmt> ]`. `ret` solo puede aparecer como última instrucción del bloque. |
| Documentación | Se actualizan secciones en docs/gramatica_formal_del_lenguaje_toy.md, se agrega "Verificación LL(1)" y apéndice con gramática original (no LL(1)). |
| Tokens | Se agrega `TOK-MAIN`. Se renombran `number/string/bool` a `TOK-KW-NUMBER/TOK-KW-STRING/TOK-KW-BOOL` (los literales siguen siendo `TOK-NUMBER`/`TOK-STRING`). |
| Léxico | Letra = [a-zA-Z_] completo. Se elimina Exponente. Se agrega Punto `[.]`. Blanco = espacio,\t,\r,\v,\f (sin `\n`). Se separan comillas en DQ/SQ. Se agregan `:` y `.` al alfabeto. |
| Números y cadenas | Números: dígitos+ opcionalmente seguidos de "." y dígitos+ (3.14 válido; "5." es error). Cadenas con misma comilla, sin salto de línea. |
| AFD | Se completa tabla en docs/especificacion_y_diseno_formal_del_lenguaje_toy.md. |
| Notas de diseño | Se agregan notas de diseño (precedencia de `%`, distinción palabras reservadas/identificadores, maximal munch). |

## Decisiones de diseño y pendientes

- `%` tiene mayor precedencia que `*` y `/` (intencional).
- Cadenas no multilínea.
- `number`/`string`/`bool` son palabras reservadas aún sin uso en la gramática.
- Léxico usa máxima coincidencia (maximal munch).
