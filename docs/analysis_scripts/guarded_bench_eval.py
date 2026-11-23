import os
import csv,json,joblib,numpy as np,glob
b=joblib.load("rf_p5.joblib"); m=b["model"]; known=np.array(sorted(int(i) for i in b["known_ids"]))
rng={int(k,16):(np.array(v[0]),np.array(v[1])) for k,v in json.load(open("ranges_p5.json")).items()}
def rules(ids,dl,pay):
    X=np.zeros((len(ids),10),np.float32); X[:,0]=ids; X[:,1]=dl; X[:,2:7]=pay
    v=np.zeros((len(X),5),np.int8); r=np.arange(len(X))
    for t in m.estimators_: v[r,np.argmax(t.predict_proba(X),1)]+=1
    cls=v.argmax(1); top=v.max(1); unk=~np.isin(ids,known)
    oor=np.zeros(len(ids),bool)
    for i,(lo,hi) in rng.items():
        mm=ids==i
        if mm.any(): oor[mm]=np.any((pay[mm]<lo)|(pay[mm]>hi),1)
    return {"majority":unk|((cls!=0)&(top>=3)),"strict":unk|((cls!=0)&(top==5)),"new":unk|((cls!=0)&(top==5))|((cls!=0)&(top>=3)&oor)}
def load(f):
    R=list(csv.DictReader(open(f))); ids=[];dl=[];pay=[];cl=[]
    for x in R:
        d=[int(h,16) for h in x['data'].split()]; ids.append(int(x['can_id'],16)); dl.append(int(x.get('dlc') or len(d)))
        p=(d+[0]*8)[:5]
        # zero bytes beyond dlc as in training
        pay.append(p); cl.append(x.get('class','Normal'))
    return np.array(ids),np.array(dl),np.array(pay),np.array(cl)
out={}
for f in sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'runs', '*', 'frames.csv'))):
    ids,dl,pay,cl=load(f); R=rules(ids,dl,pay); name=f.split('/')[-2]
    res={}
    for k,d in R.items():
        res[k]={"genuine_flagged":int(np.sum(d&(cl=='Normal'))),"attack_flagged":f"{int(np.sum(d&(cl!='Normal')))}/{int(np.sum(cl!='Normal'))}"}
    out[name]=res; print(name,res)
json.dump(out,open("bench_eval.json","w"),indent=1)
