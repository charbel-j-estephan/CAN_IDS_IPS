import sys
src,dst,top=sys.argv[1],sys.argv[2],sys.argv[3].split('/')
out=open(dst,'w'); stack=[]; indef=True; flat=[]; seen=set()
for line in open(src):
    if indef:
        t=line.split()
        if t and t[0]=='$scope':
            stack.append(t[2]); continue
        if t and t[0]=='$upscope':
            stack.pop(); continue
        if t and t[0]=='$var':
            if stack[:len(top)]==top:
                rel=stack[len(top):]
                name='.'.join(rel+[t[4]]) 
                rest=' '.join(t[5:-1])
                if t[3] in seen: continue
                seen.add(t[3])
                flat.append(f"$var {t[1]} {t[2]} {t[3]} {name} {rest} $end\n".replace('  ',' '))
            continue
        if t and t[0]=='$enddefinitions':
            for s in top: out.write(f"$scope module {s} $end\n")
            out.writelines(flat)
            for s in top: out.write("$upscope $end\n")
            out.write(line); indef=False; continue
        if line.startswith('$timescale') or line.startswith('$date') or line.startswith('$version'):
            out.write(line)
        else:
            out.write(line)
        continue
    out.write(line)
