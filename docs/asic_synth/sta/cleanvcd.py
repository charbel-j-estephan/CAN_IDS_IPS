import sys
src,dst=sys.argv[1],sys.argv[2]
out=open(dst,'w'); indef=True; realids=set()
for line in open(src):
    if indef:
        if line.startswith('$var real'):
            realids.add(line.split()[3]); continue
        out.write(line)
        if line.startswith('$enddefinitions'): indef=False
        continue
    t=line.strip()
    if t in ('$dumpvars','$dumpon','$dumpoff','$dumpall','$end'): continue
    if t.startswith('r') and t.split()[-1] in realids: continue
    out.write(line)
