import sys,bisect,pickle,os
L=open('game.asm').read().splitlines()
addrs=[];idx=[]
for i,l in enumerate(L):
    if l.startswith('   14') and ':' in l[:14]:
        addrs.append(int(l[3:12],16));idx.append(i)
pickle.dump((addrs,idx),open('asm_idx.pkl','wb'))
