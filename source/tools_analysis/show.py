import sys,bisect,pickle
L=open('game.asm').read().splitlines()
addrs,idx=pickle.load(open('asm_idx.pkl','rb'))
def find(a): 
    k=bisect.bisect_left(addrs,a); return idx[k]
for arg in sys.argv[1:]:
    a,before,after=arg.split(',') if arg.count(',')==2 else (arg,'-1','40')
    a=int(a,16);i=find(a)
    if before=='-1':
        s=i
        while 'int3' not in L[s] and s>0: s-=1
        s+=1
    else: s=i-int(before)
    print('=====',hex(a))
    for l in L[s:i+int(after)]:
        print(l[3:12],l[32:])
