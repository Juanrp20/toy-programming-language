# Toy - documento de diseño 

## Tabla de tokens del lenguaje 
Tabla de tokens del lenguaje: 

| Categoría | Token | Lexema / regla de formación |
|---|---|---|
| Palabra reservada | TOK-IF | `if` |
| Palabra reservada | TOK-THEN | `then` |
| Palabra reservada | TOK-ELSE | `else` |
| Palabra reservada | TOK-TRUE | `true` |
| Palabra reservada | TOK-FALSE | `false` |
| Palabra reservada | TOK-AND | `and` |
| Palabra reservada | TOK-OR | `or` |
| Palabra reservada | TOK-WHILE | `while` |
| Palabra reservada | TOK-DO | `do` |
| Palabra reservada | TOK-FOR | `for` |
| Palabra reservada | TOK-FUNC | `func` |
| Palabra reservada | TOK-END | `end` |
| Palabra reservada | TOK-MAIN | `main` |
| Palabra reservada | TOK-LOCAL | `local` |
| Palabra reservada | TOK-PRINT | `print` |
| Palabra reservada | TOK-PRINTLN | `println` |
| Palabra reservada | TOK-RET | `ret` |
| Palabra reservada | TOK-KW-NUMBER | `number` |
| Palabra reservada | TOK-KW-STRING | `string` |
| Palabra reservada | TOK-KW-BOOL | `bool` |
| Operador aritmético | TOK-PLUS | `+` |
| Operador aritmético | TOK-MINUS | `-` |
| Operador aritmético | TOK-MUL | `*` |
| Operador aritmético | TOK-DIV | `/` |
| Operador aritmético | TOK-MOD | `%` |
| Operador aritmético | TOK-EXP | `^` |
| Operador relacional | TOK-GT | `>` |
| Operador relacional | TOK-LT | `<` |
| Operador relacional | TOK-GTE | `>=` |
| Operador relacional | TOK-LTE | `<=` |
| Operador relacional | TOK-EQ | `==` |
| Operador relacional | TOK-NEQ | `~=` |
| Operador lógico | TOK-NOT | `~` |
| Operador de asignación | TOK-ASSIGN | `:=` |
| Símbolo | TOK-LPAR | `(` |
| Símbolo | TOK-RPAR | `)` |
| Símbolo | TOK-COMMA | `,` |
| Identificador | TOK-ID | Inicia con letra o guion bajo, seguido de letras, números o guiones bajos. |
| Literal numérico | TOK-NUMBER | Dígitos+ opcionalmente seguidos de "." y dígitos+ (3.14 válido; "5." es error léxico "número mal formado"). |
| Literal de cadena | TOK-STRING | Texto encerrado entre comillas dobles o simples, debe cerrar con la misma comilla con la que abrió y no puede contener salto de línea. |

Nota: "number/string/bool no se usan en la gramática actual; pendiente decidir si se eliminan o se agregan declaraciones con tipo". No se eliminan.

## Autómata (AFD): 
Definición de vocabularios: 
Letra = [a-zA-Z_] completo (letras del alfabeto y guion bajo). 
Dígito: [0-9] 
Punto = [.] 
DQ = ["], SQ = ['] 
Símbolo simple: [*/%^(),] [operadores y símbolos que no requieren lookahead]. 
Blanco = espacio, \t, \r, \v, \f (sin el salto de línea \n, que es su propia clase). 
Otro: Cualquier otro símbolo que no esté dentro del alfabeto. 
Alfabeto (Σ): {Letra, Dígito, DQ, SQ, Punto, Símbolo simple, Blanco, `+`, `-`, `=`, `~`, `:`, `>`, `<`, \n, Otro}

Tabla de Transiciones:

| Estado | Letra | Dígito | DQ | SQ | Símb. | Blanco | + | - | = | ~ | : | > | < | . | \n | Otro |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| S0 | S1 | S2 | S3 | S3b | S20 | S0 | S19 | S4 | S5 | S6 | S7 | S15 | S17 | STOP | S0 | STOP |
| S1 (Identificador)* | S1 | S1 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S2 (Número)* | STOP | S2 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S13 | STOP | STOP |
| S3 (Cadena abierta con ") | S3 | S3 | S8 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | STOP | S3 |
| S3b (Cadena abierta con ') | S3b | S3b | S3b | S8 | S3b | S3b | S3b | S3b | S3b | S3b | S3b | S3b | S3b | S3b | STOP | S3b |
| S4 (Tras -)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S9 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S5 (Tras =) | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S10 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S6 (Tras ~)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S11 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S7 (Tras :) | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S12 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S8 (Cadena cerrada)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S9 (Comentario --) | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S0 | S9 |
| S10 (==)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S11 (~=)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S12 (:=)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S13 (tras '.') | STOP | S14 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S14 (parte decimal)* | STOP | S14 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S15 (>)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S16 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S16 (>=)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S17 (<)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S18 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S18 (<=)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S19 (+)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S20 (símbolo simple)* | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |

Estados: S = {S0, S1, S2, S3, S3b, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, S15, S16, S17, S18, S19, S20}. Los estados marcados con * son de aceptación. S4 es TOK-MINUS y S6 es TOK-NOT; S9 se descarta como comentario.

Cualquier transición no listada es STOP (se emite el token si el estado es final; si el estado no es final es error léxico). Estados no finales que producen error: S3/S3b "cadena sin cerrar", S5 "'=' solo no es válido, use '==' o ':='", S7 "':' solo no es válido, use ':='", S13 "número mal formado".

## Notas de diseño
- `%` tiene mayor precedencia que `*` y `/` (así está en la gramática, es intencional).
- Las palabras reservadas se distinguen de los identificadores comparando el lexema del estado S1 contra la tabla.
- El léxico usa máxima coincidencia (maximal munch).