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
| Palabra reservada | TOK-LOCAL | `local` |
| Palabra reservada | TOK-PRINT | `print` |
| Palabra reservada | TOK-PRINTLN | `println` |
| Palabra reservada | TOK-RET | `ret` |
| Palabra reservada | TOK-NUMBER | `number` |
| Palabra reservada | TOK-STRING | `string` |
| Palabra reservada | TOK-BOOL | `bool` |
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
| Literal numérico | TOK-NUMBER | Secuencia de dígitos, opcionalmente con un punto decimal. |
| Literal de cadena | TOK-STRING | Texto encerrado entre comillas dobles o simples. |

## Autómata (AFD): 
Definición de vocabularios: 
Letra: [a-zA-Z_] (letras del alfabeto y guion bajo, excluyendo e y E). 
Exponente: [eE] 
Dígito: [0-9] 
Comillas: ['"] 
Símbolo simple: [*/%^(),] [operadores y símbolos que no requieren lookahead]. 
Blanco: [\t\v] (Espacios y tabulaciones excluyendo el salto de línea). 
Otro: Cualquier otro símbolo que no esté dentro del alfabeto. 
Alfabeto (Σ): {Letra, Dígito, Comillas, Símbolo simple, Blanco, `+`, `-`, `=`, `~`, `>`, `<`, \n, Otro} 

Tabla de Transiciones (Extracto Principal): 

| Estado | Letra | Dígito | Comilla | Símb. | Blanco | `+` | `-` | `=` | `~` | `:` | `>` | `<` | \n | Otro |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| S0* | S1 | S2 | S3 | STOP | S0 | STOP | S4 | S5 | S6 | S7 | STOP | STOP | S0 | STOP |
| S1 (Identificador) | S1 | S1 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S2 (Número) | STOP | S2 | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S3 (Cadena) | S3 | S3 | S8* | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 | S3 |
| S4 (Tras -) | STOP | STOP | STOP | STOP | STOP | STOP | S9 | STOP | STOP | STOP | STOP | STOP | STOP | STOP |
| S5 (Tras =) | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S10* | STOP | STOP | STOP | STOP | STOP | STOP |
| S6 (Tras ~) | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S11* | STOP | STOP | STOP | STOP | STOP | STOP |
| S7 (Tras :) | STOP | STOP | STOP | STOP | STOP | STOP | STOP | S12* | STOP | STOP | STOP | STOP | STOP | STOP |
| S9 (Comentario --) | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S9 | S0 | S9 |

Estados: S = {S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12}. Los estados marcados con * son de aceptación.