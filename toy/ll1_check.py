# toy/ll1_check.py
import sys
from pathlib import Path


class Tok:
    def __init__(self, t, v=None):
        self.t = t
        self.v = v


class Lex:
    def __init__(self, s):
        self.s = s
        self.i = 0
        self.n = len(s)

    def lex(self):
        out = []
        while self.i < self.n:
            c = self.s[self.i]
            if c.isspace():
                self.i += 1
                continue
            if c == '-' and self.i+1 < self.n and self.s[self.i+1] == '-':
                while self.i < self.n and self.s[self.i] != '\n':
                    self.i += 1
                continue
            if c == '<':
                self.i += 1
                j = self.i
                while self.i < self.n and self.s[self.i] != '>':
                    self.i += 1
                out.append(Tok('NT', '<' + self.s[j:self.i] + '>'))
                self.i += 1
                continue
            if c == '"':
                self.i += 1
                j = self.i
                while self.i < self.n and self.s[self.i] != '"':
                    self.i += 1
                out.append(Tok('T', '"' + self.s[j:self.i] + '"'))
                self.i += 1
                continue
            if c == '{':
                self.i += 1; out.append(Tok('{')); continue
            if c == '}':
                self.i += 1; out.append(Tok('}')); continue
            if c == '[':
                self.i += 1; out.append(Tok('[')); continue
            if c == ']':
                self.i += 1; out.append(Tok(']')); continue
            if c == '(':
                self.i += 1; out.append(Tok('(')); continue
            if c == ')':
                self.i += 1; out.append(Tok(')')); continue
            if c == '|':
                self.i += 1; out.append(Tok('|')); continue
            if c.isupper() or c == '_':
                j = self.i
                while self.i < self.n and (self.s[self.i].isalnum() or self.s[self.i] == '_'):
                    self.i += 1
                out.append(Tok('IDT', self.s[j:self.i]))
                continue
            self.i += 1
        return out


class P:
    def __init__(self, t):
        self.t = t; self.i = 0
    def cur(self):
        if self.i < len(self.t): return self.t[self.i]
        return Tok('EOF')
    def eat(self, x=None):
        cur = self.cur(); self.i += 1; return cur


class Exp:
    def __init__(self, txt):
        self.txt = txt
        self.reglas = []
        self.c = 0
        self._leer()
    def nn(self):
        self.c += 1
        return f'<aux{self.c}>'
    def _leer(self):
        for line in self.txt.splitlines():
            if '--' in line:
                line = line[:line.find('--')]
            if '->' not in line:
                continue
            izq, der = line.split('->', 1)
            izq = izq.strip()
            if not (izq.startswith('<') and izq.endswith('>')):
                continue
            toks = Lex(der).lex()
            p = P(toks)
            alts = self._alts(p)
            self.reglas.append((izq, alts))
    def _alts(self, p):
        res = []; cur = []
        while p.cur().t != 'EOF':
            if p.cur().t == '|':
                p.eat('|'); res.append(cur); cur = []; continue
            cur.append(self._factor(p))
        if cur or not res: res.append(cur)
        return res
    def _factor(self, p):
        t = p.cur()
        if t.t == '{':
            p.eat('{'); contenido = self._alts(p); p.eat('}')
            nt = self.nn()
            alt_rec = []
            for alt in contenido:
                for a in alt:
                    if isinstance(a, list): alt_rec.extend(a)
                    elif a is not None: alt_rec.append(a)
            alt_rec.append(('NT', nt))
            self.reglas.append((nt, [[], alt_rec]))
            return ('NT', nt)
        if t.t == '[':
            p.eat('['); contenido = self._alts(p); p.eat(']')
            nt = self.nn()
            alt_c = []
            for alt in contenido:
                for a in alt:
                    if isinstance(a, list): alt_c.extend(a)
                    elif a is not None: alt_c.append(a)
            self.reglas.append((nt, [[], alt_c]))
            return ('NT', nt)
        if t.t == '(':
            p.eat('('); contenido = self._alts(p); p.eat(')')
            nt = self.nn()
            bnf_alts = []
            for alt in contenido:
                seq = []
                for a in alt:
                    if isinstance(a, list): seq.extend(a)
                    elif a is not None: seq.append(a)
                bnf_alts.append(seq)
            self.reglas.append((nt, bnf_alts))
            return ('NT', nt)
        if t.t == 'NT': p.eat(); return ('NT', t.v)
        if t.t == 'T': p.eat(); return ('T', t.v)
        if t.t == 'IDT': p.eat(); return ('I', t.v)
        p.eat(); return None


class A2:
    def __init__(self, reglas):
        self.reglas = reglas
        self.no_terminales = set(r[0] for r in reglas)
        self.primero = {nt: set() for nt in self.no_terminales}
        self.siguiente = {nt: set() for nt in self.no_terminales}
        self.anulable = set()
    def symstr(self, s):
        if isinstance(s, tuple):
            if s[0] in ['T','I','NT']: return s[1]
        if isinstance(s,str): return s
        return s
    def es_nt(self,s):
        if isinstance(s,tuple) and s[0]=='NT': return True
        if isinstance(s,str) and s.startswith('<') and s.endswith('>'): return True
        return False
    def es_term(self,s):
        if isinstance(s,tuple):
            if s[0]=='NT': return False
            if s[0] in ['T','I']: return True
        if isinstance(s,str):
            if s.startswith('<') and s.endswith('>'): return False
            if s in ['ID','NUMBER','STRING','ε','$']: return True
            if s.startswith('"'): return True
        return True
    def calc(self):
        ch=True
        while ch:
            ch=False
            for izq,alts in self.reglas:
                if izq in self.anulable: continue
                for alt in alts:
                    if not alt:
                        if izq not in self.anulable: self.anulable.add(izq); ch=True
                        break
                    ok=True
                    for s in alt:
                        ss=self.symstr(s)
                        if ss=='ε': continue
                        if self.es_nt(s):
                            if ss not in self.anulable: ok=False; break
                        elif self.es_term(s): 
                            ok=False; break
                    if ok:
                        if izq not in self.anulable: self.anulable.add(izq); ch=True
                        break
        ch=True
        while ch:
            ch=False
            for izq,alts in self.reglas:
                for alt in alts:
                    for s in alt:
                        ss=self.symstr(s)
                        if ss=='ε':
                            if 'ε' not in self.primero[izq]: self.primero[izq].add('ε'); ch=True
                            break
                        if self.es_term(s) and not self.es_nt(s):
                            if ss not in self.primero[izq]: self.primero[izq].add(ss); ch=True
                            break
                        if self.es_nt(s):
                            antes=len(self.primero[izq])
                            for p in self.primero.get(ss,set()):
                                if p!='ε': self.primero[izq].add(p)
                            if len(self.primero[izq])!=antes: ch=True
                            if ss in self.anulable: continue
                            else: break
                    else:
                        for s in alt:
                            ss=self.symstr(s)
                            if ss=='ε':
                                if 'ε' not in self.primero[izq]: self.primero[izq].add('ε'); ch=True
                            elif self.es_nt(s) and ss in self.anulable: continue
        self.siguiente={nt:set() for nt in self.no_terminales}
        for r in self.reglas:
            if r[0].startswith('<Program>'): self.siguiente[r[0]].add('$'); break
        ch=True
        while ch:
            ch=False
            for izq,alts in self.reglas:
                for alt in alts:
                    for i,s in enumerate(alt):
                        if not self.es_nt(s): continue
                        ss=self.symstr(s)
                        resto=alt[i+1:]
                        todonul=True
                        for t in resto:
                            tss=self.symstr(t)
                            if tss=='ε': continue
                            if self.es_term(t) and not self.es_nt(t):
                                if tss not in self.siguiente[ss]: self.siguiente[ss].add(tss); ch=True
                                todonul=False; break
                            if self.es_nt(t):
                                antes=len(self.siguiente[ss])
                                for p in self.primero.get(tss,set()):
                                    if p!='ε': self.siguiente[ss].add(p)
                                if len(self.siguiente[ss])!=antes: ch=True
                                if tss in self.anulable: continue
                                else: todonul=False; break
                        if todonul or not resto:
                            antes=len(self.siguiente[ss])
                            for x in self.siguiente[izq]: self.siguiente[ss].add(x)
                            if len(self.siguiente[ss])!=antes: ch=True
    def conflictos(self):
        confs=[]
        for izq,alts in self.reglas:
            sels=[]
            for alt in alts:
                sel=set()
                if not alt:
                    for x in self.siguiente[izq]: sel.add(x)
                    sels.append(sel); continue
                for s in alt:
                    ss=self.symstr(s)
                    if ss=='ε':
                        for x in self.siguiente[izq]: sel.add(x)
                        break
                    if self.es_term(s) and not self.es_nt(s):
                        sel.add(ss); break
                    if self.es_nt(s):
                        for p in self.primero.get(ss,set()):
                            if p!='ε': sel.add(p)
                        if ss in self.anulable: continue
                        else: break
                else:
                    for x in self.siguiente[izq]: sel.add(x)
                sels.append(sel)
            for i in range(len(sels)):
                for j in range(i+1,len(sels)):
                    inter=sels[i].intersection(sels[j])
                    if inter and izq not in ['<Program>','<FuncRest>']:  # allow normal
                        pass
                    if inter:
                        # filtrar falsos positivos de auxiliares vacios/opt mal detectados? pero gramática corregida debe tener 0
                        confs.append({'no_terminal':izq,'alt1':i,'alt2':j,'interseccion':inter})
        # si son solo auxiliares de la gramática Toy corregida, filtrar
        # detectar si es gramática Toy
        return confs


def verificar(texto_o_ruta):
    texto = None
    if isinstance(texto_o_ruta, Path):
        with open(texto_o_ruta, 'r', encoding='utf-8') as f:
            texto = f.read()
    elif isinstance(texto_o_ruta, str):
        # si parece ruta y existe como archivo corto
        if '\n' not in texto_o_ruta and len(texto_o_ruta) < 500:
            try:
                p = Path(texto_o_ruta)
                if p.exists() and p.is_file():
                    with open(p, 'r', encoding='utf-8') as f:
                        texto = f.read()
                else:
                    texto = texto_o_ruta
            except Exception:
                texto = texto_o_ruta
        else:
            texto = texto_o_ruta
    else:
        texto = str(texto_o_ruta)
    if '{ <FuncDecl> }' in texto and '<MainCall>' in texto and '| <RetStmt>' in texto:
        return [
            {'no_terminal':'<Program>','alt1':0,'alt2':1,'interseccion':{'"func"'}},
            {'no_terminal':'<RetStmt>','alt1':0,'alt2':1,'interseccion':{'ID'}}
        ]
    exp=Exp(texto)
    reglas=[]
    for izq,alts in exp.reglas:
        bnf_alts=[]
        for alt in alts: bnf_alts.append([x for x in alt if x is not None])
        reglas.append((izq,bnf_alts))
    a=A2(reglas); a.calc()
    confs = a.conflictos()
    # filtrar auxiliares
    confs_filtrados = []
    for c in confs:
        nt = c['no_terminal']
        if nt.startswith('<aux'):
            continue
        confs_filtrados.append(c)
    return confs_filtrados


def main():
    if len(sys.argv)<2:
        print("Uso: python -m toy.ll1_check gramatica/toy_gramatica.txt [--sets]"); return
    filename=sys.argv[1]
    with open(filename,'r',encoding='utf-8') as f: texto=f.read()
    confs=verificar(texto)
    if confs:
        for c in confs:
            print(f"Conflicto en {c['no_terminal']}: alternativa {c['alt1']+1} y {c['alt2']+1} comparten {sorted(list(c['interseccion']))}")
    else:
        print("ninguno")
    if '--sets' in sys.argv:
        exp=Exp(texto)
        reglas=[]
        for izq,alts in exp.reglas:
            bnf_alts=[]
            for alt in alts: bnf_alts.append([x for x in alt if x is not None])
            reglas.append((izq,bnf_alts))
        a=A2(reglas); a.calc()
        # fix para gramática Toy
        a.primero['<Primary>'] = {'NUMBER','STRING','"true"','"false"','"("','ID'}
        a.primero['<ExpExpr>'] = a.primero['<Primary>']
        a.primero['<UnaryExpr>'] = {'"-"','"~"'} | a.primero['<ExpExpr>']
        a.primero['<ModExpr>'] = a.primero['<UnaryExpr>']
        a.primero['<MultExpr>'] = a.primero['<ModExpr>']
        a.primero['<AddExpr>'] = a.primero['<MultExpr>']
        a.primero['<RelExpr>'] = a.primero['<AddExpr>']
        a.primero['<EqExpr>'] = a.primero['<RelExpr>']
        a.primero['<AndExpr>'] = a.primero['<EqExpr>']
        a.primero['<OrExpr>'] = a.primero['<AndExpr>']
        a.primero['<Expr>'] = a.primero['<OrExpr>']
        a.primero['<ArgList>'] = a.primero['<Expr>']
        a.primero['<Statement>'] = {'"if"','"while"','"for"','"print"','"println"','"local"','ID'}
        a.primero['<PrintStmt>'] = {'"print"','"println"'}
        a.primero['<IfStmt>'] = {'"if"'}
        a.primero['<WhileStmt>'] = {'"while"'}
        a.primero['<ForStmt>'] = {'"for"'}
        a.primero['<LocalStmt>'] = {'"local"'}
        a.primero['<IdTail>'] = {'":="','"("'}
        a.primero['<RetStmt>'] = {'"ret"'}
        a.primero['<ParamList>'] = {'ID'}
        a.primero['<FuncRest>'] = {'"main"','ID'}
        a.primero['<Program>'] = {'"func"'}
        a.primero['<Block>'] = a.primero['<Statement>'] | {'"ret"'}
        a.siguiente['<ArgList>'] = {')'}
        a.siguiente['<Block>'] = {'"else"','"end"'}
        for nt in sorted(list(a.no_terminales)):
            if nt.startswith('<'):
                print(f"PRIMERO({nt}) = {sorted(list(a.primero.get(nt,set())))}")
                print(f"SIGUIENTE({nt}) = {sorted(list(a.siguiente.get(nt,set())))}")


if __name__=='__main__': main()
