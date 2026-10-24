# Toy

Toy es un lenguaje didáctico para un curso de Compiladores. Este repositorio contiene la gramática formal, la especificación léxica y la implementación (analizador léxico + analizador sintáctico descendente recursivo LL(1) + verificador LL(1) + pruebas).

## Estructura del repo

- `Documentation/gramatica_formal_del_lenguaje_toy.md` - Gramática formal con correcciones LL(1)
- `Documentation/especificacion_y_diseño_formal_del_lenguaje_toy.md` - Especificación léxica y AFD
- `Documentation/gramatica_toy.md` - Gramática en formato EBNF
- `toy_gramatica.txt` - Gramática corregida LL(1)
- `toy_lexer.py` - Analizador léxico
- `toy_parser.py` - Analizador sintáctico descendente recursivo LL(1)
- `ll1_check.py` - Verificador LL(1)
- `run_tests.py` - Pruebas
- `ejemplos/` - Ejemplos de programas

## Cómo ejecutar

### Verificador LL(1)
```bash
python ll1_check.py toy_gramatica.txt [--sets]
python ll1_check.py <gramatica_original>  # debe reportar 2 conflictos
```

### Parser
```bash
python toy_parser.py archivo.toy
python toy_parser.py archivo.toy --tokens
python toy_parser.py archivo.toy --ast
```

### Pruebas
```bash
python run_tests.py
```

## Cambios respecto a la gramática/especificación original

| Punto | Cambio |
|---|---|
| A1 - Conflicto 1 | Se reemplaza `<Program> -> { <FuncDecl> } <MainCall>` por `<Program> -> "func" <FuncRest>` con `<FuncRest>` factorizado entre `main` y función con ID. |
| A1 - Conflicto 2 | Se elimina `<RetStmt>` de `<Statement>` y se pasa a `<Block> -> { <Statement> } [ <RetStmt> ]`. `ret` solo puede aparecer como última instrucción del bloque. |
| A1 - Documentación | Se actualizan secciones 0.2, 0.3, 0.4, se agrega "Verificación LL(1)" y apéndice con gramática original (no LL(1)). |
| A2 - Tokens | Se agrega `TOK-MAIN`. Se renombran `number/string/bool` a `TOK-KW-NUMBER/TOK-KW-STRING/TOK-KW-BOOL` (los literales siguen siendo `TOK-NUMBER`/`TOK-STRING`). |
| A2 - Léxico | Letra = [a-zA-Z_] completo. Se elimina Exponente. Se agrega Punto `[.]`. Blanco = espacio,\t,\r,\v,\f (sin `\n`). Se separan comillas en DQ/SQ. Se agregan `:` y `.` al alfabeto. |
| A2 - Números/Cadenas | Números: dígitos+ opcionalmente seguidos de "." y dígitos+ (3.14 válido; "5." es error). Cadenas con misma comilla, sin salto de línea (error si `\n`/EOF antes de cerrar). |
| A2 - AFD | Se completa tabla con S0-S12 + S3b,S13,S14,S15,S16,S17,S18,S19,S20. Estados finales actualizados. Transiciones clave según especificación. |
| A2 - Notas | Se agregan notas de diseño (precedencia de `%`, distinción palabras reservadas/identificadores, maximal munch). |
