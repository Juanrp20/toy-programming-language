import sys
from pathlib import Path


class EBNFParser:
    def __init__(self, text):
        self.text = text
        self.aux_counters = {}
        self.rules = []
        self.aux_rules = []

    def make_aux(self, orig_lhs):
        base = orig_lhs.strip("<>").split("_aux")[0]
        self.aux_counters[base] = self.aux_counters.get(base, 0) + 1
        return f"<{base}_aux{self.aux_counters[base]}>"

    def tokenize(self):
        tokens = []
        for line in self.text.splitlines():
            if "--" in line:
                line = line[:line.find("--")]
            i = 0
            n = len(line)
            while i < n:
                c = line[i]
                if c.isspace():
                    i += 1
                    continue
                if line[i:i + 2] == "->":
                    tokens.append(("ARROW", "->"))
                    i += 2
                    continue
                if c in "{}[]()|":
                    tokens.append((c, c))
                    i += 1
                    continue
                if c == "<":
                    j = line.find(">", i)
                    if j == -1:
                        j = n
                    tokens.append(("NT", line[i:j + 1]))
                    i = j + 1
                    continue
                if c == '"':
                    j = line.find('"', i + 1)
                    if j == -1:
                        j = n - 1
                    tokens.append(("T", line[i:j + 1]))
                    i = j + 1
                    continue
                if c == "'":
                    j = line.find("'", i + 1)
                    if j == -1:
                        j = n - 1
                    tokens.append(("T", '"' + line[i + 1:j] + '"'))
                    i = j + 1
                    continue
                if c.isalnum() or c == "_":
                    j = i
                    while j < n and (line[j].isalnum() or line[j] == "_"):
                        j += 1
                    tokens.append(("ID", line[i:j]))
                    i = j
                    continue
                tokens.append(("T", c))
                i += 1
        return tokens

    def parse(self):
        tokens = self.tokenize()
        rule_starts = []
        for i in range(len(tokens) - 1):
            if tokens[i][0] == "NT" and tokens[i + 1][0] == "ARROW":
                rule_starts.append(i)

        parsed_rules = []
        for idx, start_i in enumerate(rule_starts):
            end_i = rule_starts[idx + 1] if idx + 1 < len(rule_starts) else len(tokens)
            lhs = tokens[start_i][1]
            rhs_tokens = tokens[start_i + 2: end_i]
            parsed_rules.append((lhs, rhs_tokens))

        rules_dict = {}
        rule_order = []
        self.aux_rules = []

        for lhs, rhs_tokens in parsed_rules:
            if lhs not in rules_dict:
                rules_dict[lhs] = []
                rule_order.append(lhs)
            alts = self._parse_rhs(lhs, rhs_tokens)
            rules_dict[lhs].extend(alts)

        final_rules = [(nt, rules_dict[nt]) for nt in rule_order]
        final_rules.extend(self.aux_rules)
        return final_rules

    def _parse_rhs(self, orig_lhs, tokens):
        pos = [0]

        def cur():
            return tokens[pos[0]] if pos[0] < len(tokens) else ("EOF", "")

        def eat(expected=None):
            t = cur()
            pos[0] += 1
            return t

        def parse_expr():
            alts = [parse_seq()]
            while cur()[0] == "|":
                eat("|")
                alts.append(parse_seq())
            return alts

        def parse_seq():
            seq = []
            while cur()[0] not in ("EOF", "|", "}", "]", ")"):
                f = parse_factor()
                if f is not None:
                    seq.append(f)
            return seq

        def parse_factor():
            t = cur()
            if t[0] == "{":
                eat("{")
                inner = parse_expr()
                eat("}")
                aux_nt = self.make_aux(orig_lhs)
                aux_alts = []
                for alt in inner:
                    aux_alts.append(alt + [aux_nt])
                aux_alts.append([])
                self.aux_rules.append((aux_nt, aux_alts))
                return aux_nt
            elif t[0] == "[":
                eat("[")
                inner = parse_expr()
                eat("]")
                aux_nt = self.make_aux(orig_lhs)
                aux_alts = []
                for alt in inner:
                    aux_alts.append(list(alt))
                aux_alts.append([])
                self.aux_rules.append((aux_nt, aux_alts))
                return aux_nt
            elif t[0] == "(":
                eat("(")
                inner = parse_expr()
                eat(")")
                aux_nt = self.make_aux(orig_lhs)
                aux_alts = []
                for alt in inner:
                    aux_alts.append(list(alt))
                self.aux_rules.append((aux_nt, aux_alts))
                return aux_nt
            elif t[0] == "NT":
                eat("NT")
                return t[1]
            elif t[0] in ("T", "ID"):
                eat()
                return t[1]
            else:
                eat()
                return None

        return parse_expr()


class LL1Calculator:
    def __init__(self, rules):
        self.rules = rules
        self.non_terminals = set(r[0] for r in rules)
        self.nullable = set()
        self.first = {nt: set() for nt in self.non_terminals}
        self.follow = {nt: set() for nt in self.non_terminals}

    def first_seq(self, seq):
        res = set()
        for sym in seq:
            if sym in self.non_terminals:
                res |= self.first[sym]
                if sym not in self.nullable:
                    break
            else:
                res.add(sym)
                break
        return res

    def is_seq_nullable(self, seq):
        return all(sym in self.nullable for sym in seq)

    def calculate(self):
        changed = True
        while changed:
            changed = False
            for lhs, alts in self.rules:
                if lhs in self.nullable:
                    continue
                for alt in alts:
                    if self.is_seq_nullable(alt):
                        self.nullable.add(lhs)
                        changed = True
                        break

        changed = True
        while changed:
            changed = False
            for lhs, alts in self.rules:
                for alt in alts:
                    f = self.first_seq(alt)
                    if not f.issubset(self.first[lhs]):
                        self.first[lhs] |= f
                        changed = True

        if self.rules:
            self.follow[self.rules[0][0]].add("$")
        changed = True
        while changed:
            changed = False
            for lhs, alts in self.rules:
                for alt in alts:
                    for i, sym in enumerate(alt):
                        if sym in self.non_terminals:
                            beta = alt[i + 1:]
                            f_beta = self.first_seq(beta)
                            if not f_beta.issubset(self.follow[sym]):
                                self.follow[sym] |= f_beta
                                changed = True
                            if self.is_seq_nullable(beta):
                                if not self.follow[lhs].issubset(self.follow[sym]):
                                    self.follow[sym] |= self.follow[lhs]
                                    changed = True

    def conflicts(self):
        confs = []
        for lhs, alts in self.rules:
            selection_sets = []
            for alt in alts:
                sel = set(self.first_seq(alt))
                if self.is_seq_nullable(alt):
                    sel |= self.follow[lhs]
                selection_sets.append(sel)
            for i in range(len(selection_sets)):
                for j in range(i + 1, len(selection_sets)):
                    inter = selection_sets[i] & selection_sets[j]
                    if inter:
                        confs.append({
                            "no_terminal": lhs,
                            "alt1": i,
                            "alt2": j,
                            "interseccion": set(inter)
                        })
        return confs

    # Compatibilidad con nombres antiguos
    @property
    def no_terminales(self):
        return self.non_terminals

    @property
    def anulable(self):
        return self.nullable

    @property
    def primero(self):
        return self.first

    @property
    def siguiente(self):
        return self.follow

    def calc(self):
        self.calculate()


Exp = EBNFParser
A2 = LL1Calculator


def _leer_texto(texto_o_ruta):
    if isinstance(texto_o_ruta, Path):
        with open(texto_o_ruta, "r", encoding="utf-8") as f:
            return f.read()
    if isinstance(texto_o_ruta, str):
        if "\n" not in texto_o_ruta and len(texto_o_ruta) < 1000:
            try:
                p = Path(texto_o_ruta)
                if p.exists() and p.is_file():
                    with open(p, "r", encoding="utf-8") as f:
                        return f.read()
            except Exception:
                pass
        return texto_o_ruta
    return str(texto_o_ruta)


def calcular_conjuntos(texto_o_ruta):
    texto = _leer_texto(texto_o_ruta)
    rules = EBNFParser(texto).parse()
    calc = LL1Calculator(rules)
    calc.calculate()
    return calc.nullable, calc.first, calc.follow


def verificar(texto_o_ruta):
    texto = _leer_texto(texto_o_ruta)
    rules = EBNFParser(texto).parse()
    calc = LL1Calculator(rules)
    calc.calculate()
    return calc.conflicts()


def main():
    if len(sys.argv) < 2:
        print("Uso: python -m toy.ll1_check gramatica/toy_gramatica.txt [--sets]")
        return
    filename = sys.argv[1]
    with open(filename, "r", encoding="utf-8") as f:
        texto = f.read()
    confs = verificar(texto)
    if confs:
        for c in confs:
            inter = sorted(list(c["interseccion"]))
            print(f"Conflicto en {c['no_terminal']}: alternativa {c['alt1'] + 1} y {c['alt2'] + 1} comparten {inter}")
    else:
        print("ninguno")
    if "--sets" in sys.argv:
        anulables, primero, siguiente = calcular_conjuntos(texto)
        for nt in sorted(list(primero.keys())):
            if nt.startswith("<"):
                print(f"PRIMERO({nt}) = {sorted(list(primero.get(nt, set())))}")
                print(f"SIGUIENTE({nt}) = {sorted(list(siguiente.get(nt, set())))}")


if __name__ == "__main__":
    main()
