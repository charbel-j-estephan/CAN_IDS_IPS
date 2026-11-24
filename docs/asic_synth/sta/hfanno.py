import re,sys,collections
net_v,out_tcl,th=sys.argv[1],sys.argv[2],int(sys.argv[3])
OUTP={'X','Y','Q','Q_N','HI','LO','COUT','SUM','rdata','q'}
src=open(net_v).read()
body=src[src.index(');')+2:]
inst_re=re.compile(r'^\s+(\S+)\s+(\S+(?: )?)\s*\(\s*$((?:\s*\.[^\n]*\n)*?)\s*\);',re.M)
drv={}; loads=collections.defaultdict(list)
pinre=re.compile(r'\.(\w+)\((.*)\)\s*,?\s*$')
def bits(expr):
    expr=expr.strip()
    if expr.startswith('{'):
        return None
    return expr
for m in inst_re.finditer(body):
    cell,inst,pins=m.group(1),m.group(2).strip(),m.group(3)
    if not cell.startswith('sky130'): continue
    for pl in pins.strip().split('\n'):
        pm=pinre.search(pl.strip())
        if not pm: continue
        p,e=pm.group(1),bits(pm.group(2))
        if e is None: continue
        if p in OUTP: drv[e]=(inst,p)
        else: loads[e].append((inst,p))
def pn(inst,p):
    i=inst
    if i.startswith('\\'): i=i[1:]
    i=i.replace('[','\\[').replace(']','\\]')
    return '{'+i+'/'+p+'}'
n=0;tot=0
with open(out_tcl,'w') as f:
    for e,ls in loads.items():
        if len(ls)>th and e in drv:
            n+=1; tot+=len(ls)
            di,dp=drv[e]
            f.write(f'catch {{set_annotated_delay -cell -from [get_pins -of_objects [get_cells {{{di.lstrip(chr(92))}}}] -filter direction==input] -to [get_pins {pn(di,dp)}] 0.35}}\n')
            for li,lp in ls:
                f.write(f'set_annotated_delay -net -from [get_pins {pn(di,dp)}] -to [get_pins {pn(li,lp)}] 1.0\n')
print('nets',n,'loads',tot)
