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

**Implementadas en C (estándar C99):**
- Analizador léxico escrito a mano en C, equivalente al AFD documentado en `docs/`.
- Analizador sintáctico descendente recursivo LL(1) en C que construye un AST.
- Herramienta de verificación de gramáticas LL(1) y cálculo de conjuntos (`PRIMERO`/`SIGUIENTE`) en C.

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
toy-programming-language/
├── Makefile                           # Compilación con gcc C99
├── ARQUITECTURA_C.md                  # Especificación de migración a C99
├── include/
│   ├── arena.h                        # Asignador por arena para AST
│   ├── ast.h                          # Estructuras AST y constructores
│   ├── lexer.h                        # Escáner léxico C99
│   ├── ll1.h                          # Verificador EBNF y cálculo LL(1)
│   ├── parser.h                       # Parser descendente recursivo
│   ├── set.h                          # Estructura de conjuntos dinámicos de cadenas
│   └── token.h                        # Definición de tokens y palabras reservadas
├── src/
│   ├── arena.c
│   ├── ast.c
│   ├── lexer.c
│   ├── ll1.c
│   ├── main.c                         # Punto de entrada de toy_c
│   ├── main_ll1.c                     # Punto de entrada de toy_ll1
│   ├── parser.c
│   ├── set.c
│   └── token.c
├── docs/
│   ├── especificacion_y_diseno_formal_del_lenguaje_toy.md
│   ├── gramatica_formal_del_lenguaje_toy.md
│   └── gramatica_toy.md
├── gramatica/
│   ├── toy_gramatica.txt              (corregida, LL(1))
│   └── toy_gramatica_original.txt     (original, NO LL(1))
├── ejemplos/
│   ├── ok_completo.toy
│   └── ok_minimo.toy
└── tests/
    └── test_c.c                       # Suite de pruebas automatizadas en C
```

## Guía paso a paso: Compilación y Ejecución

### 1. Requisitos previos
- Compilador de C compatible con **C99** (recomendado: `gcc` versión 9+ o `clang`).
- Utilidad `make`.

---

### 2. Paso a paso de compilación

#### Opción A: Usando `make` (Recomendado)

> **Nota para Windows (MinGW/MSYS2):** En lugar del comando `make`, utiliza `mingw32-make` (o agrega un alias). El Makefile detecta automáticamente Windows para adaptar las extensiones `.exe` y comandos de limpieza.

1. **Compilar los binarios principales (`toy_c` y `toy_ll1`):**
   ```bash
   make all
   # O en Windows:
   mingw32-make all
   ```
   Esto compila con banderas de optimización `-O2`, `-std=c99`, `-Wall`, `-Wextra` y `-pedantic`.

2. **Compilar y ejecutar la suite de pruebas automatizadas:**
   ```bash
   make test
   # O en Windows:
   mingw32-make test
   ```
   En Linux/macOS compila con `AddressSanitizer` (`-fsanitize=address,undefined`); en Windows compila en modo debug `-g` y ejecuta las 9 suites de pruebas.

3. **Compilar en modo depuración:**
   ```bash
   make debug
   # O en Windows:
   mingw32-make debug
   ```

4. **Limpiar binarios generados:**
   ```bash
   make clean
   # O en Windows:
   mingw32-make clean
   ```

---

#### Opción B: Compilación manual directa con `gcc` (sin `make`)

Si no tienes `make` instalado, compila directamente con los siguientes comandos:

1. **Compilar el compilador (`toy_c`):**
   ```bash
   gcc -Wall -Wextra -std=c99 -pedantic -Iinclude -O2 -o toy_c \
       src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c src/main.c
   ```

2. **Compilar el verificador LL(1) (`toy_ll1`):**
   ```bash
   gcc -Wall -Wextra -std=c99 -pedantic -Iinclude -O2 -o toy_ll1 \
       src/set.c src/ll1.c src/main_ll1.c
   ```

3. **Compilar y ejecutar las pruebas (`test_c`):**
   - **En Linux / macOS:**
     ```bash
     gcc -Wall -Wextra -std=c99 -pedantic -Iinclude -g -fsanitize=address,undefined -o test_c \
         src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c src/set.c src/ll1.c tests/test_c.c
     ./test_c
     ```
   - **En Windows (PowerShell / CMD):**
     ```powershell
     gcc -Wall -Wextra -std=c99 -pedantic -Iinclude -g -o test_c.exe src/token.c src/lexer.c src/arena.c src/ast.c src/parser.c src/set.c src/ll1.c tests/test_c.c
     .\test_c.exe
     ```

---

### 3. Guía de uso de los ejecutables

#### A. Analizador Léxico y Sintáctico (`./toy_c`)

- **Validar sintaxis (salida `OK` si es válido, reporte de error si no):**
  ```bash
  ./toy_c ejemplos/ok_completo.toy
  ```

- **Inspeccionar tokens reconocidos por el lexer:**
  ```bash
  ./toy_c ejemplos/ok_completo.toy --tokens
  ```

- **Imprimir el Árbol de Sintaxis Abstracta (AST):**
  ```bash
  ./toy_c ejemplos/ok_completo.toy --ast
  ```

#### B. Verificador de Gramáticas LL(1) (`./toy_ll1`)

- **Verificar conflictos en la gramática LL(1) corregida (debe retornar `ninguno`):**
  ```bash
  ./toy_ll1 gramatica/toy_gramatica.txt
  ```

- **Verificar conflictos en la gramática original (debe reportar 2 conflictos):**
  ```bash
  ./toy_ll1 gramatica/toy_gramatica_original.txt
  ```

- **Calcular e imprimir los conjuntos PRIMERO y SIGUIENTE:**
  ```bash
  ./toy_ll1 gramatica/toy_gramatica.txt --sets
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
