import os
import sys, json
import joblib, numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
from dataset import load
X0, y, src = load(os.environ.get("CAR_HACKING_DIR", "data"), with_source=True)
test = np.zeros(len(y), bool)
for s in np.unique(src):
    idx = np.flatnonzero(src == s); a = np.flatnonzero(y[idx] != 0)
    cut = a[int(len(a)*0.8)] if len(a) else int(len(idx)*0.8); test[idx[cut:]] = True
P=5
X = X0.copy(); X[:, 2+P:] = 0
b = joblib.load("rf_p5.joblib"); m=b["model"]; known=np.array(b["known_ids"])
tr = ~test
# per ID byte ranges from TRAINING normal frames only
rng={}
for i in known:
    mm = tr & (y==0) & (X[:,0]==i)
    d = X[mm][:, 2:2+P]
    rng[int(i)] = (d.min(0), d.max(0), int(mm.sum()))
Xt=X[test]; yt=y[test]; Xf=Xt.astype(np.float32)
v=np.zeros((len(Xt),5),np.int8); r=np.arange(len(Xt))
for t in m.estimators_: v[r,np.argmax(t.predict_proba(Xf),1)]+=1
cls=v.argmax(1); top=v.max(1); unknown=~np.isin(Xt[:,0],known)
oor=np.zeros(len(Xt),bool)
for i,(lo,hi,n) in rng.items():
    mm = Xt[:,0]==i
    d = Xt[mm][:,2:2+P]
    oor[mm] = np.any((d<lo)|(d>hi),1)
N=["Normal","DoS","Fuzzy","Gear","RPM"]
def ev(d,name):
    e={"normal_destroyed":int(np.sum(d&(yt==0)))}
    for c in range(1,5):
        mm=yt==c; e[N[c]+"_pct"]=round(100*float(np.mean(d[mm])),3); e[N[c]+"_passed"]=int(np.sum(mm&~d))
    print(name,e); return e
out={}
out["majority"]=ev(unknown|((cls!=0)&(top>=3)),"majority")
out["strict"]=ev(unknown|((cls!=0)&(top==5)),"strict")
out["range_only"]=ev(unknown|oor,"range_only")
out["strict_or_majority_and_range"]=ev(unknown|((cls!=0)&(top==5))|((cls!=0)&(top>=3)&oor),"strict|maj&range")
out["strict_or_range"]=ev(unknown|((cls!=0)&(top==5))|oor,"strict|range")
# which normals violate range
fp = oor&(yt==0)
from collections import Counter
print("range fp by id",Counter(hex(int(i)) for i in Xt[fp,0]).most_common(10))
print("range fp by source",Counter(src[test][fp]).most_common())
json.dump(out,open("fix_eval.json","w"),indent=1)
json.dump({hex(k):[list(map(int,v[0])),list(map(int,v[1])),v[2]] for k,v in rng.items()},open("ranges_p5.json","w"))
d=unknown|((cls!=0)&(top==5))|((cls!=0)&(top>=3)&oor)
w=np.flatnonzero(d&(yt==0))
for k in w: print("FP",hex(int(Xt[k,0])),Xt[k,1:7],v[k],oor[k],src[test][k])
