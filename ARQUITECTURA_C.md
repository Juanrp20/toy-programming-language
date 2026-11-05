# Arquitectura de Migración a C99 (`toy-programming-language`)

Documento técnico que detalla la migración de los componentes de Python a **C puro estándar C99**, manteniendo paridad funcional, modularidad, cero fugas de memoria y reporte exacto de errores.

---

## 1. Estructura del Proyecto

```
toy-programming-language/
├── Makefile                 # Compilación con gcc C99 estricto y sanitizers
├── ARQUITECTURA_C.md        # Especificación arquitectónica y de diseño
├── include/
│   ├── arena.h              # Gestor de memoria (Arena allocator) para el AST
│   ├── ast.h                # Tagged unions para expresiones, sentencias y bloques
│   ├── lexer.h              # Analizador léxico y seguimiento de línea/columna
│   ├── ll1.h                # Parser EBNF, cálculo FIRST/FOLLOW y conflictos
│   ├── parser.h             # Analizador sintáctico descendente recursivo LL(1)
│   ├── set.h                # Conjuntos dinámicos ordenados de cadenas
│   └── token.h              # Definición de tokens y palabras reservadas
├── src/
│   ├── arena.c              # Asignador por bloques para evitar memory leaks
│   ├── ast.c                # Constructores y serializador idéntico al AST de Python
│   ├── lexer.c              # Implementación de autómatas finitos y lectura de literales
│   ├── ll1.c                # Algoritmo de cálculo LL(1) y resolución de EBNF
│   ├── main.c               # CLI toy_c: normal, --tokens y --ast
│   ├── main_ll1.c           # CLI toy_ll1: normal y --sets
│   ├── parser.c             # Análisis sintáctico con lookahead y saltos por error
│   ├── set.c                # Operaciones de unión, intersección y subconjuntos
│   └── token.c              # Mapeo de identificadores a tokens y cadenas descriptivas
└── tests/
    └── test_c.c             # Suite de pruebas unitarias y de regresión en C
```

---

## 2. Estructuras de Datos

### 2.1 Tokens ([`include/token.h`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/include/token.h))
```c
typedef enum {
    TOK_EOF = 0,
    TOK_ERROR,
    TOK_IF, TOK_THEN, TOK_ELSE, TOK_TRUE, TOK_FALSE,
    TOK_AND, TOK_OR, TOK_WHILE, TOK_DO, TOK_FOR,
    TOK_FUNC, TOK_END, TOK_MAIN, TOK_LOCAL,
    TOK_PRINT, TOK_PRINTLN, TOK_RET,
    TOK_KW_NUMBER, TOK_KW_STRING, TOK_KW_BOOL,
    TOK_MINUS, TOK_PLUS, TOK_MUL, TOK_DIV, TOK_MOD, TOK_EXP,
    TOK_LPAR, TOK_RPAR, TOK_COMMA,
    TOK_GTE, TOK_GT, TOK_LTE, TOK_LT, TOK_EQ, TOK_NEQ, TOK_NOT, TOK_ASSIGN,
    TOK_ID, TOK_NUMBER, TOK_STRING
} TokenKind;

typedef struct {
    TokenKind kind;
    char *lexeme;
    int line;
    int col;
} Token;
```

### 2.2 AST y Tagged Unions ([`include/ast.h`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/include/ast.h))
El Árbol de Sintaxis Abstracta modela cada nodo mediante uniones discriminadas:
- **`ASTExpr`**: Literales (`number`, `string`, `bool`), identificadores (`id`), paréntesis (`paren`), operaciones binarias (`binary`), unarios (`unary_minus`, `unary_not`) y llamadas a función (`call`).
- **`ASTStmt`**: Estructuras de control (`if`, `while`, `for`), salidas (`print`, `println`), declaraciones (`local`), asignaciones (`assign`), invocaciones como sentencia (`call_stmt`) y retornos (`ret`).
- **`ASTBlock`**: Lista dinámica de sentencias con un nodo `ret` opcional terminal.
- **`ASTProgram`**: Secuencia de funciones que culmina en `main`.

---

## 3. Módulo Lexer ([`src/lexer.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/src/lexer.c))

- Procesa el código fuente caracter a caracter preservando el conteo de `line` y `col`.
- Implementa comentarios con doble guion (`--`).
- Emite errores léxicos exactos:
  - `carácter '.' no válido`
  - `'=' solo no es válido, use '==' o ':='`
  - `':' solo no es válido, use ':='`
  - `cadena sin cerrar`
  - `número mal formado`
  - `carácter '<c>' no válido`

---

## 4. Módulo Parser ([`src/parser.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/src/parser.c))

- Analizador sintáctico descendente recursivo con un token de preanálisis (`la`).
- Usa `setjmp`/`longjmp` para interrumpir el análisis ante errores y limpiar memoria sin fallos de segmentación.
- Respeta la jerarquía formal de precedencia de operadores y la asociatividad por derecha de `^`.
- Formato uniforme de error:
  `Línea <line>, columna <col>: se esperaba <esperado>, se encontró <encontrado>`

---

## 5. Gestión de Memoria ([`src/arena.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/src/arena.c))

- Implementa un asignador de memoria por arena (`Arena`).
- Toda la memoria asignada para el AST y cadenas dinámicas se registra de forma centralizada.
- `arena_free()` libera el 100% de los nodos en una sola operación al terminar o al recuperarse de un error.
- Validado con `gcc -fsanitize=address,undefined` (cero leaks, cero memory corruption).

---

## 6. Módulo Verificador LL(1) ([`src/ll1.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/src/ll1.c) / [`src/set.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/src/set.c))

- Escanea y analiza gramáticas en notación EBNF.
- Expande constructos `{}` (repetición) y `[]` (opcionalidad) a reglas auxiliares `<NT_auxN>` en BNF puro.
- Implementa conjuntos dinámicos de cadenas ordenadas ([`StringSet`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/include/set.h#L6-L10)).
- Calcula conjuntos de punto fijo:
  - `NULLABLE`: no terminales que pueden derivar la cadena vacía $\varepsilon$.
  - `PRIMERO`: símbolos terminales iniciales derivados de cada símbolo o secuencia.
  - `SIGUIENTE`: símbolos terminales que pueden aparecer inmediatamente a la derecha de cada no terminal.
- Detecta conflictos LL(1) cuando dos alternativas de un mismo no terminal comparten símbolos en sus conjuntos de selección.

---

## 7. Suite de Pruebas ([`tests/test_c.c`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/tests/test_c.c))

La suite unitaria e integradora cubre 9 suites de pruebas críticas con 100% de éxito:
1. `test_lexer_basicos`: palabras reservadas, operadores, números, cadenas y comentarios.
2. `test_lexer_errores`: validación de los 7 mensajes de error léxico exactos.
3. `test_parser_ok`: análisis sintáctico de [`ejemplos/ok_minimo.toy`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/ejemplos/ok_minimo.toy) y [`ejemplos/ok_completo.toy`](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/ejemplos/ok_completo.toy).
4. `test_parser_invalidos`: rechazo de 15 programas sintácticamente incorrectos.
5. `test_precedencia`: verificación de jerarquía y asociatividad de operadores.
6. `test_ll1`: cálculo de conjuntos y verificación de 0 conflictos en gramática corregida y 2 conflictos en gramática original.
7. `test_readme_ejemplo`: extracción y análisis sintáctico del código de ejemplo del [README.md](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/README.md).
8. `test_apendice_gramatica_original`: verificación de la gramática original documentada en el apéndice de [docs/gramatica_formal_del_lenguaje_toy.md](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/docs/gramatica_formal_del_lenguaje_toy.md).
9. `test_afd_especificacion`: simulación de transiciones del AFD formal documentado en [docs/especificacion_y_diseno_formal_del_lenguaje_toy.md](file:///home/juanma/Documentos/compiladores/Tarea2/toy-programming-language/docs/especificacion_y_diseno_formal_del_lenguaje_toy.md).

---

## 8. Compilación y Ejecución

```bash
# Compilar binarios de producción (toy_c y toy_ll1)
make all

# Compilar y ejecutar todas las pruebas con AddressSanitizer y UndefinedBehaviorSanitizer
make test

# Ejecución del analizador sintáctico y léxico
./toy_c ejemplos/ok_minimo.toy
./toy_c ejemplos/ok_completo.toy
./toy_c ejemplos/ok_completo.toy --tokens
./toy_c ejemplos/ok_completo.toy --ast

# Ejecución del verificador de gramática LL(1)
./toy_ll1 gramatica/toy_gramatica.txt
./toy_ll1 gramatica/toy_gramatica.txt --sets
./toy_ll1 gramatica/toy_gramatica_original.txt
```
