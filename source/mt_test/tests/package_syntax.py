"""Lightweight delimiter check only; not a Windows PowerShell execution test."""
def balanced(s):
 stack=[];pairs={')':'(',']':'[','}':'{'};i=0
 while i<len(s):
  if s.startswith('<#',i):
   z=s.find('#>',i+2)
   if z<0:return False
   i=z+2;continue
  if s[i]=='#':
   z=s.find('\n',i);i=len(s) if z<0 else z;continue
  if s[i] in ('"',"'"):
   q=s[i];i+=1
   while i<len(s):
    if q=='"' and s[i]=='`':i+=2;continue
    if s[i]==q:
     if q=="'" and i+1<len(s) and s[i+1]==q:i+=2;continue
     i+=1;break
    i+=1
   continue
  if s[i] in '([{':stack.append(s[i])
  elif s[i] in ')]}':
   if not stack or stack.pop()!=pairs[s[i]]:return False
  i+=1
 return not stack
