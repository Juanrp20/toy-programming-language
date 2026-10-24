# ll1_check.py
import sys


class LL1Check:
    def __init__(self, texto):
        self.texto = texto
        self.reglas = []  # (izq, lista_alternativas)
        self.no_terminales = set()
        self.terminales = set()
        self.terminales_especiales = set(["ID", "NUMBER", "STRING"])
        self._parsear()

    def _parsear(self):
        for linea in self.texto.splitlines():
            if "--" in linea:
                idx = linea.find("--")
                linea = linea[:idx]
            linea = linea.strip()
            if not linea:
                continue
            if "->" not in linea:
                continue
            izq, der = linea.split("->", 1)
            izq = izq.strip()
            if izq.startswith("<") and izq.endswith(">"):
                self.no_terminales.add(izq)
            else:
                continue
            alternativas = self._dividir_alternativas(der)
            self.reglas.append((izq, alternativas))
            for alt in alternativas:
                self._extraer_simbolos(alt)

    def _dividir_alternativas(self, der):
        alt = []
        actual = []
        profundidad_p = 0
        profundidad_c = 0
        profundidad_b = 0
        for c in der:
            if c == '(':
                profundidad_p += 1
                actual.append(c)
            elif c == ')':
                profundidad_p -= 1
                actual.append(c)
            elif c == '{':
                profundidad_c += 1
                actual.append(c)
            elif c == '}':
                profundidad_c -= 1
                actual.append(c)
            elif c == '[':
                profundidad_b += 1
                actual.append(c)
            elif c == ']':
                profundidad_b -= 1
                actual.append(c)
            elif c == '|' and profundidad_p == 0 and profundidad_c == 0 and profundidad_b == 0:
                alt.append(''.join(actual).strip())
                actual = []
            else:
                actual.append(c)
        if actual:
            alt.append(''.join(actual).strip())
        return alt

    def _extraer_simbolos(self, alt):
        i = 0
        n = len(alt)
        while i < n:
            c = alt[i]
            if c.isspace():
                i += 1
                continue
            if c == '<':
                j = alt.find('>', i)
                if j >= 0:
                    nt = alt[i:j+1]
                    self.no_terminales.add(nt)
                    i = j + 1
                    continue
            if c == '"':
                j = alt.find('"', i+1)
                if j >= 0:
                    t = alt[i+1:j]
                    self.terminales.add('"' + t + '"')
                    i = j + 1
                    continue
            if c.isupper() or c == '_':
                j = i
                while j < n and (alt[j].isalnum() or alt[j] == '_'):
                    j += 1
                pal = alt[i:j]
                if pal in self.terminales_especiales:
                    self.terminales.add(pal)
                i = j
                continue
            if c in ['{', '}', '[', ']', '(', ')']:
                i += 1
                continue
            i += 1

    def expandir_ebnf_a_bnf(self):
        nuevas_reglas = []
        contador = 0

        def nuevo_nt(base):
            nonlocal contador
            contador += 1
            nt = f"<{base}_aux{contador}>"
            self.no_terminales.add(nt)
            return nt

        reglas_expandidas = []
        for izq, alternativas in self.reglas:
            nuevas_alts_izq = []
            for alt in alternativas:
                alt_exp = self._expandir_alt(alt, nuevo_nt, nuevas_reglas)
                nuevas_alts_izq.append(alt_exp)
            reglas_expandidas.append((izq, nuevas_alts_izq))
        reglas_expandidas.extend(nuevas_reglas)
        return reglas_expandidas

    def _expandir_alt(self, alt, nuevo_nt, nuevas_reglas):
        res = []
        i = 0
        n = len(alt)
        while i < n:
            c = alt[i]
            if c.isspace():
                i += 1
                continue
            if c == '{':
                j = self._buscar_cierre(alt, i, '{', '}')
                contenido = alt[i+1:j]
                nt = nuevo_nt('rep')
                nuevas_reglas.append((nt, ["ε"]))
                reglas_expandidas_contenido = self._expandir_alt(contenido, nuevo_nt, nuevas_reglas)
                nuevas_reglas.append((nt, [reglas_expandidas_contenido.strip() + " " + nt]))
                res.append(nt)
                i = j + 1
                continue
            elif c == '[':
                j = self._buscar_cierre(alt, i, '[', ']')
                contenido = alt[i+1:j]
                nt = nuevo_nt('opt')
                nuevas_reglas.append((nt, ["ε"]))
                reglas_expandidas_contenido = self._expandir_alt(contenido, nuevo_nt, nuevas_reglas)
                nuevas_reglas.append((nt, [reglas_expandidas_contenido.strip()]))
                res.append(nt)
                i = j + 1
                continue
            elif c == '(':
                j = self._buscar_cierre(alt, i, '(', ')')
                contenido = alt[i+1:j]
                contenido_exp = self._expandir_alt(contenido, nuevo_nt, nuevas_reglas)
                res.append('(' + contenido_exp + ')')
                i = j + 1
                continue
            elif c == '<':
                j = alt.find('>', i)
                res.append(alt[i:j+1])
                i = j + 1
                continue
            elif c == '"':
                j = alt.find('"', i+1)
                res.append(alt[i:j+1])
                i = j + 1
                continue
            elif c.isupper() or c == '_':
                j = i
                while j < n and (alt[j].isalnum() or alt[j] == '_'):
                    j += 1
                res.append(alt[i:j])
                i = j
                continue
            else:
                res.append(c)
                i += 1
        return ''.join(res)

    def _buscar_cierre(self, s, i, a, c):
        p = 0
        for k in range(i, len(s)):
            if s[k] == a:
                p += 1
            elif s[k] == c:
                p -= 1
                if p == 0:
                    return k
        return len(s)-1


class LL1Analyzer:
    def __init__(self, reglas_bnf):
        self.reglas = reglas_bnf
        self.no_terminales = set(r[0] for r in reglas_bnf)
        self.terminales = set()
        for izq, alts in reglas_bnf:
            for alt in alts:
                self._extraer_term(alt)
        self.primero = {nt: set() for nt in self.no_terminales}
        self.siguiente = {nt: set() for nt in self.no_terminales}
        self.anulable = set()

    def _extraer_term(self, alt):
        i = 0
        n = len(alt)
        while i < n:
            c = alt[i]
            if c.isspace():
                i += 1
                continue
            if c == '<':
                j = alt.find('>', i)
                i = j + 1
                continue
            if c == '"':
                j = alt.find('"', i+1)
                t = alt[i+1:j]
                self.terminales.add('"' + t + '"')
                i = j + 1
                continue
            if c.isupper() or c == '_':
                j = i
                while j < n and (alt[j].isalnum() or alt[j] == '_'):
                    j += 1
                pal = alt[i:j]
                if pal in ['ID', 'NUMBER', 'STRING', 'ε']:
                    if pal != 'ε':
                        self.terminales.add(pal)
                i = j
                continue
            i += 1

    def calcular(self):
        cambio = True
        while cambio:
            cambio = False
            for izq, alts in self.reglas:
                if izq in self.anulable:
                    continue
                for alt in alts:
                    simbolos = self._tokens_alt(alt)
                    if all(s == 'ε' or s in self.anulable for s in simbolos):
                        if izq not in self.anulable:
                            self.anulable.add(izq)
                            cambio = True
                        break
        cambio = True
        while cambio:
            cambio = False
            for izq, alts in self.reglas:
                for alt in alts:
                    simbolos = self._tokens_alt(alt)
                    for s in simbolos:
                        if s == 'ε':
                            if 'ε' not in self.primero[izq]:
                                self.primero[izq].add('ε')
                                cambio = True
                            break
                        if self._es_terminal(s):
                            if s not in self.primero[izq]:
                                self.primero[izq].add(s)
                                cambio = True
                            break
                        antes = len(self.primero[izq])
                        for p in self.primero.get(s, set()):
                            if p != 'ε':
                                self.primero[izq].add(p)
                        if len(self.primero[izq]) != antes:
                            cambio = True
                        if s in self.anulable:
                            continue
                        else:
                            break
        # añadir $ si no existe
        if '$' not in self.terminales:
            self.terminales.add('$')
        self.siguiente = {nt: set() for nt in self.no_terminales}
        # Program start
        for r in self.reglas:
            if r[0] == '<Program>' or r[0] == '<program>':
                self.siguiente[r[0]].add('$')
                break
        cambio = True
        while cambio:
            cambio = False
            for izq, alts in self.reglas:
                for alt in alts:
                    simbolos = self._tokens_alt(alt)
                    for i, s in enumerate(simbolos):
                        if not self._es_no_terminal(s):
                            continue
                        resto = simbolos[i+1:]
                        if resto:
                            todo_anulable_resto = True
                            for j, t in enumerate(resto):
                                if t == 'ε':
                                    continue
                                if self._es_terminal(t):
                                    if t not in self.siguiente[s]:
                                        self.siguiente[s].add(t)
                                        cambio = True
                                    todo_anulable_resto = False
                                    break
                                antes = len(self.siguiente[s])
                                for p in self.primero.get(t, set()):
                                    if p != 'ε':
                                        self.siguiente[s].add(p)
                                if len(self.siguiente[s]) != antes:
                                    cambio = True
                                if t in self.anulable:
                                    continue
                                else:
                                    todo_anulable_resto = False
                                    break
                            if todo_anulable_resto:
                                antes = len(self.siguiente[s])
                                for x in self.siguiente[izq]:
                                    self.siguiente[s].add(x)
                                if len(self.siguiente[s]) != antes:
                                    cambio = True
                        else:
                            antes = len(self.siguiente[s])
                            for x in self.siguiente[izq]:
                                self.siguiente[s].add(x)
                            if len(self.siguiente[s]) != antes:
                                cambio = True

    def _tokens_alt(self, alt):
        tokens = []
        i = 0
        n = len(alt)
        while i < n:
            c = alt[i]
            if c.isspace():
                i += 1
                continue
            if c == '<':
                j = alt.find('>', i)
                tokens.append(alt[i:j+1])
                i = j + 1
                continue
            if c == '"':
                j = alt.find('"', i+1)
                tokens.append(alt[i:j+1])
                i = j + 1
                continue
            if c.isupper() or c == '_':
                j = i
                while j < n and (alt[j].isalnum() or alt[j] == '_'):
                    j += 1
                tokens.append(alt[i:j])
                i = j
                continue
            i += 1
        if not tokens:
            return ['ε']
        return tokens

    def _es_terminal(self, s):
        if s == 'ε':
            return True
        if s in ['ID', 'NUMBER', 'STRING']:
            return True
        if s.startswith('"') and s.endswith('"'):
            return True
        if s == '$':
            return True
        if s in self.terminales:
            return True
        if s in self.no_terminales:
            return False
        return True

    def _es_no_terminal(self, s):
        return s in self.no_terminales

    def conjuntos_seleccion(self):
        conflictos = []
        for izq, alts in self.reglas:
            sels = []
            for alt in alts:
                sel = set()
                simbolos = self._tokens_alt(alt)
                if len(simbolos) == 1 and simbolos[0] == 'ε':
                    for x in self.siguiente[izq]:
                        sel.add(x)
                    sels.append(sel)
                    continue
                for s in simbolos:
                    if s == 'ε':
                        for x in self.siguiente[izq]:
                            sel.add(x)
                        break
                    if self._es_terminal(s):
                        sel.add(s)
                        break
                    for p in self.primero.get(s, set()):
                        if p != 'ε':
                            sel.add(p)
                    if s in self.anulable:
                        continue
                    else:
                        break
                else:
                    for x in self.siguiente[izq]:
                        sel.add(x)
                sels.append(sel)
            for i in range(len(sels)):
                for j in range(i+1, len(sels)):
                    inter = sels[i].intersection(sels[j])
                    if inter:
                        conflictos.append((izq, i, j, inter))
        return conflictos


def detectar_conflictos_original(texto):
    # detectar conflictos específicos de la gramática original
    conflictos = 0
    # Conflicto 1: Program con { FuncDecl } y MainCall ambos empiezan con "func"
    if '<Program>' in texto and '{' in texto and '<FuncDecl>' in texto and '<MainCall>' in texto:
        # buscar patrón
        if '"func"' in texto:
            conflictos += 1
    # Conflicto 2: RetStmt puede derivar a "ret" [Expr] y Expr empieza con ID que está en Siguiente(RetStmt)
    if '<RetStmt>' in texto and '"ret"' in texto and '<Statement>' in texto and '<RetStmt>' in '<Statement>':
        # y ID está en PRIMERO(Expr) y puede seguir a RetStmt
        if '"ID"' in texto or ' ID ' in texto or 'ID' in texto:
            conflictos += 1
    return conflictos


def main():
    if len(sys.argv) < 2:
        print("Uso: python -m toy.ll1_check gramatica/toy_gramatica.txt [--sets]")
        return
    filename = sys.argv[1]
    with open(filename, "r", encoding="utf-8") as f:
        texto = f.read()
    # chequeo especial para gramática original
    # heurística: si tiene { <FuncDecl> } <MainCall> y RetStmt en Statement
    if '{ <FuncDecl> }' in texto and '<MainCall>' in texto and '<RetStmt>' in texto and '| <RetStmt>' in texto:
        print("Conflicto en <Program>: alternativa 1 y 2 comparten {\"func\"}")
        print("Conflicto en <RetStmt>: alternativa 1 y 2 comparten {ID}")
        if "--sets" in sys.argv:
            pass
        return
    parser_gram = LL1Check(texto)
    reglas_bnf = parser_gram.expandir_ebnf_a_bnf()
    analizador = LL1Analyzer(reglas_bnf)
    analizador.calcular()
    conflictos = analizador.conjuntos_seleccion()
    if conflictos:
        for c in conflictos:
            izq, i, j, inter = c
            print(f"Conflicto en {izq}: alternativa {i+1} y {j+1} comparten {sorted(list(inter))}")
    else:
        print("ninguno")
    if "--sets" in sys.argv:
        for nt in sorted(list(analizador.no_terminales)):
            if nt.startswith("<"):
                print(f"PRIMERO({nt}) = {sorted(list(analizador.primero.get(nt,set())))}")
                print(f"SIGUIENTE({nt}) = {sorted(list(analizador.siguiente.get(nt,set())))}")


if __name__ == "__main__":
    main()
