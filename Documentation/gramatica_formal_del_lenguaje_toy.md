# 0.1 Introducción
Toy es un lenguaje de programación de tipado estático diseñado con un enfoque didáctico para el aprendizaje sobre compiladores. Su sintaxis está inspirada en lenguajes como Lua y Ruby, lo que lo hace simple y evita la complejidad, prescindiendo del uso de llaves para bloques y de puntos y comas para terminar instrucciones. Este documento describe su sintaxis formal utilizando notación EBNF (Extended Backus-Naur Form), garantizando las condiciones necesarias para la implementación de un analizador sintáctico predictivo descendente (LL1).

# 0.2 Estructura Básica de un Programa en Toy
Todo programa en Toy consiste en una secuencia de funciones terminada por `main`. `main` es una palabra reservada y el programa es una secuencia de funciones terminada por `main`; actúa como punto de entrada del programa. Toy no permite funciones anidadas.
EBNF para el programa principal:
```ebnf
<Program>  -> "func" <FuncRest>
<FuncRest> -> "main" "(" ")" <Block> "end" "main" "(" ")"
             | ID "(" [ <ParamList> ] ")" <Block> "end" <Program>
```

# 0.3 Declaración de Funciones
Las funciones en Toy se declaran utilizando la palabra clave `func` seguida de un identificador y una lista opcional de parámetros. La función termina con la palabra reservada `end`. Opcionalmente, las funciones pueden devolver un valor utilizando la instrucción `ret`.
EBNF para funciones y bloques:
```ebnf
<ParamList> -> ID { "," ID }
<Block>    -> { <Statement> } [ <RetStmt> ]
```

# 0.4 Variables, Asignaciones e Instrucciones (Statements)
Toy es un lenguaje de tipado estático, pero la especificación documentada utiliza la asignación mediante el operador `:=` (ej. score `:=` 5). Además, una variable puede ser forzada a ser local dentro de un bloque usando la palabra reservada `local`.
Para mantener la propiedad LL(1) de la gramática y evitar conflictos (FIRST) entre variables y llamadas a funciones (ya que ambas inician con un ID), se ha factorizado la regla <Statement>.
"ret" solo puede aparecer como última instrucción de un bloque.
EBNF para Instrucciones:
```ebnf
<Statement> -> <IfStmt> 
              | <WhileStmt> 
              | <ForStmt> 
              | <PrintStmt> 
              | <LocalStmt>
              | ID <IdTail>
<IdTail> -> ":=" <Expr>            -- Asignación
           | "(" [ <ArgList> ] ")"  -- Llamada a función
<ArgList> -> <Expr> { "," <Expr> }
<LocalStmt> -> "local" ID ":=" <Expr>
<RetStmt> -> "ret" [ <Expr> ]
```

# 0.5 Estructuras de Control e Impresión
Toy soporta estructuras condicionales (`if`...`then`...`else`...`end`) y dos tipos de ciclos (`while` y `for`). Para la salida estándar, utiliza `print` y `println`.
EBNF para Control de Flujo:
```ebnf
<IfStmt> -> "if" <Expr> "then" <Block> [ "else" <Block> ] "end"
<WhileStmt> -> "while" <Expr> "do" <Block> "end"
<ForStmt> -> "for" ID ":=" <Expr> "," <Expr> [ "," <Expr> ] "do" <Block> "end"
<PrintStmt> -> ( "print" | "println" ) <Expr>
```

# 0.6 Expresiones y Operadores
Las expresiones en Toy producen un valor y respetan una estricta precedencia y asociatividad. La gramática resuelve la precedencia dividiendo las expresiones en niveles EBNF de menor a mayor prioridad. Para garantizar un analizador LL(1) sin recursión por la izquierda, se utilizan iteraciones { }.
Precedencia (de menor a mayor): `or`, `and`, igualdades (`==`, `~=`), relacionales (`<`, `>`, `<=`, `>=`), suma/resta (`+`, `-`), multiplicación/división (`*`, `/`), módulo (`%`), unarios (`-`, `~`), y exponenciación (`^`).
La exponenciación y los operadores unarios son asociativos por la derecha.
EBNF para Expresiones:
```ebnf
<Expr> -> <OrExpr> 
<OrExpr> -> <AndExpr> { "or" <AndExpr> }
<AndExpr> -> <EqExpr> { "and" <EqExpr> }
<EqExpr> -> <RelExpr> { ( "==" | "~=" ) <RelExpr> }
<RelExpr> -> <AddExpr> { ( "<" | "<=" | ">" | ">=" ) <AddExpr> }
<AddExpr> -> <MultExpr> { ( "+" | "-" ) <MultExpr> }
<MultExpr> -> <ModExpr> { ( "*" | "/" ) <ModExpr> }
<ModExpr> -> <UnaryExpr> { "%" <UnaryExpr> }
<UnaryExpr> -> "-" <UnaryExpr> 
              | "~" <UnaryExpr> 
              | <ExpExpr>
-- Asociatividad por la derecha para la exponenciación
<ExpExpr> -> <Primary> [ "^" <UnaryExpr> ] 
<Primary> -> NUMBER 
            | STRING 
            | "true" 
            | "false" 
            | "(" <Expr> ")"
            | ID [ "(" [ <ArgList> ] ")" ]
```

# Verificación LL(1)

Conflicto 1: `<Program> -> { <FuncDecl> } <MainCall>`. Tanto `<FuncDecl>` como `<MainCall>` empiezan ambos con `"func"`, y no se puede decidir con un token de lookahead.
Conflicto 2: `<RetStmt> -> "ret" [ <Expr> ]`. `ID` está en `PRIMERO(Expr)` y en `SIGUIENTE(RetStmt)` (después de un `ret` puede venir una sentencia que empieza con `ID`).

Solución:
- Se reemplaza la estructura del programa por `<Program> -> "func" <FuncRest>` con factorización entre función distinta de `main` y `main` mismo (`"main" (...) ...` vs `ID (...) ...`), eliminando la ambigüedad entre `<FuncDecl>` y `<MainCall>`.
- Se elimina `<RetStmt>` de `<Statement>` y se coloca `<RetStmt>` opcional al final de `<Block>` (`<Block> -> { <Statement> } [ <RetStmt> ]`), de modo que `ret` solo puede aparecer como última instrucción del bloque y se resuelve el conflicto con sentencias que empiezan con `ID`.

# Apéndice: versión original (no LL(1))

```ebnf
<Program> -> { <FuncDecl> } <MainCall>
<MainCall> -> "func" "main" "(" ")" <Block> "end" "main" "(" ")"

<FuncDecl> -> "func" ID "(" [ <ParamList> ] ")" <Block> "end"
<ParamList> -> ID { "," ID }
<Block> -> { <Statement> }

<Statement> -> <IfStmt> 
              | <WhileStmt> 
              | <ForStmt> 
              | <PrintStmt> 
              | <RetStmt> 
              | <LocalStmt>
              | ID <IdTail>
<IdTail> -> ":=" <Expr>            -- Asignación
           | "(" [ <ArgList> ] ")"  -- Llamada a función
<ArgList> -> <Expr> { "," <Expr> }
<LocalStmt> -> "local" ID ":=" <Expr>
<RetStmt> -> "ret" [ <Expr> ]
```
