using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace San14Patch101 {
public static class Files {
 public static string Hash(byte[] b) { using(var s=SHA256.Create())return BitConverter.ToString(s.ComputeHash(b)).Replace("-", "").ToLowerInvariant(); }
 public static string HashFile(string p) { if(!File.Exists(p)) { if(Directory.Exists(p))throw new IOException("파일 위치가 폴더입니다: "+p); return ""; } using(var f=File.OpenRead(p))using(var s=SHA256.Create())return BitConverter.ToString(s.ComputeHash(f)).Replace("-", "").ToLowerInvariant(); }
 public static string Relative(string r) {
  r=r.Replace('\\','/');
  if(String.IsNullOrEmpty(r)||r.StartsWith("/")||r.IndexOfAny(new[]{':','\0','\r','\n'})>=0||r.Split(new char[]{'/'},StringSplitOptions.None).Any(x=>x==""||x=="."||x==".."||x.EndsWith(".")||x.EndsWith(" ")))throw new IOException("잘못된 파일 경로: "+r);
  return r;
 }
 public static string Child(string root,string r) {
  r=Relative(r); string p=Path.Combine(root,r.Replace('/',Path.DirectorySeparatorChar));
  for(string q=p; !String.IsNullOrEmpty(q); q=Path.GetDirectoryName(q)) {
   // R43 11-08 7차: 연결 폴더(스팀 라이브러리 옮김·다른 모드가 이어 둔 폴더)는 막지 않고 따라감 — 게임도 그 연결된 실제 폴더에서 파일을 읽음
   if(String.Equals(q,Path.GetFullPath(root),StringComparison.OrdinalIgnoreCase))break;
  }
  return p;
 }
 public static XDocument Xml(string p) { using(var r=XmlReader.Create(p,new XmlReaderSettings{DtdProcessing=DtdProcessing.Prohibit,XmlResolver=null,MaxCharactersInDocument=4000000}))return XDocument.Load(r); }
 public static byte[] XmlBytes(XDocument d) { using(var m=new MemoryStream()){d.Save(m);return m.ToArray();} }
 public static void NewFile(string p,byte[] b) { Directory.CreateDirectory(Path.GetDirectoryName(p));using(var f=new FileStream(p,FileMode.CreateNew,FileAccess.Write,FileShare.None)){f.Write(b,0,b.Length);f.Flush(true);} }
 public static void Atomic(string p,byte[] b) {
  Directory.CreateDirectory(Path.GetDirectoryName(p)); string temp=Path.Combine(Path.GetDirectoryName(p),".s14_"+Guid.NewGuid().ToString("N").Substring(0,12)+".tmp");
  try { NewFile(temp,b); if(File.Exists(p))File.Replace(temp,p,null);else File.Move(temp,p); }
  finally {if(File.Exists(temp))File.Delete(temp);}
 }
 public static string Attr(XElement e,string n) {return (string)e.Attribute(n)??"";}
}

public sealed class Record {
 public string Path,Before="",After="",Kind="owned";
 public XElement Xml() {return new XElement("File",new XAttribute("path",Path),new XAttribute("before",Before),new XAttribute("after",After),new XAttribute("kind",Kind));}
}
public sealed class Transaction {
 public readonly string Root,Store,Journal; public Action<int> AfterWrite; public Action BeforeWrite;
 public Transaction(string root) {Root=Path.GetFullPath(root);Store=Files.Child(Root,".san14ce_compat_r3/blobs");Journal=Files.Child(Root,".san14ce_compat_r3/pending.xml");}
 public string Put(byte[] b) {string h=Files.Hash(b),p=Files.Child(Store,h);if(File.Exists(p)){if(Files.HashFile(p)!=h)throw new IOException("백업 검증 실패: "+p);}else Files.NewFile(p,b);return h;}
 public byte[] Get(string h){if(h.Length!=64||h.Any(c=>!Uri.IsHexDigit(c)))throw new IOException("백업 이름 오류");string p=Files.Child(Store,h);if(Files.HashFile(p)!=h)throw new IOException("백업 검증 실패: "+p);return File.ReadAllBytes(p);}
 public string Snapshot(string rel) {string p=Files.Child(Root,rel);return File.Exists(p)?Put(File.ReadAllBytes(p)):Files.HashFile(p);}
 public string RecoverNote=""; // R43 11-08 7차: 복구를 못 했을 때 남기는 안내(설치기 기록칸)
 public void Recover() {
  if(!File.Exists(Journal))return;
  try{RecoverCore();}
  catch(Exception e){
   // 끊긴 설치의 복구 기록을 되돌릴 수 없으면(백신이 백업을 지움·다른 프로그램이 파일을 바꿈 등) 멈추지 않음:
   //  복구 기록은 따로 보관하고, 지금 디스크의 파일 상태를 기준으로 이어 감(우리 파일은 해시로 알아봄 → 다시 설치하면 정리됨)
   string keep=Journal+".broken_"+DateTime.Now.ToString("yyyyMMdd_HHmmss");
   try{File.Move(Journal,keep);}catch(Exception){try{File.Delete(Journal);}catch(Exception){}}
   RecoverNote="이전 설치가 중간에 끊긴 흔적을 되돌리지 못해 따로 보관했습니다("+Path.GetFileName(keep)+"). 지금 파일 상태 기준으로 이어서 설치합니다. 이유: "+e.Message;
  }
 }
 void RecoverCore() {  var d=Files.Xml(Journal);if(d.Root.Name!="Transaction")throw new IOException("설치 복구 기록이 올바르지 않습니다.");
  var ops=d.Root.Elements("File").Select(e=>new Record{Path=Files.Relative(Files.Attr(e,"path")),Before=Files.Attr(e,"before"),After=Files.Attr(e,"after")}).ToList();
  if(ops.Count>4096||ops.Select(x=>x.Path.ToLowerInvariant()).Distinct().Count()!=ops.Count)throw new IOException("설치 복구 기록이 중복됩니다.");
  bool complete=true;
  foreach(var op in ops){string h=Files.HashFile(Files.Child(Root,op.Path));if(h!=op.Before&&h!=op.After)throw new IOException("설치 도중 다른 프로그램이 파일을 바꿨습니다. 보존한 상태로 중단합니다: "+op.Path);if(h!=op.After)complete=false;if(op.Before!="")Get(op.Before);if(op.After!="")Get(op.After);}
  if(!complete)foreach(var op in ops.AsEnumerable().Reverse()){if(BeforeWrite!=null)BeforeWrite();string p=Files.Child(Root,op.Path);string current=Files.HashFile(p);if(current==op.Before)continue;if(current!=op.After)throw new IOException("복원 중 다른 프로그램이 바꾼 파일을 보존합니다: "+op.Path);if(op.Before=="")File.Delete(p);else Files.Atomic(p,Get(op.Before));if(Files.HashFile(p)!=op.Before)throw new IOException("복원 파일 검증 실패: "+op.Path);}
  File.Delete(Journal);
 }
 public void Commit(List<Record> ops) {
  if(ops==null||ops.Count>4096||ops.Any(x=>x==null)||ops.Select(x=>Files.Relative(x.Path).ToLowerInvariant()).Distinct().Count()!=ops.Count)throw new IOException("설치 작업 목록이 중복되거나 올바르지 않습니다.");
  if(File.Exists(Journal))throw new IOException("이전 설치 복구가 필요합니다.");
  foreach(var op in ops){if(Files.HashFile(Files.Child(Root,op.Path))!=op.Before)throw new IOException("설치 준비 중 파일이 바뀌었습니다: "+op.Path);if(op.After!="")Get(op.After);if(op.Before!="")Get(op.Before);}
  Files.Atomic(Journal,Files.XmlBytes(new XDocument(new XElement("Transaction",ops.Select(x=>x.Xml())))));
  try {int i=0;foreach(var op in ops){if(BeforeWrite!=null)BeforeWrite();string p=Files.Child(Root,op.Path);if(Files.HashFile(p)!=op.Before)throw new IOException("다른 프로그램이 변경한 파일은 보존합니다: "+op.Path);if(op.After=="")File.Delete(p);else Files.Atomic(p,Get(op.After));if(Files.HashFile(p)!=op.After)throw new IOException("기록한 파일 검증 실패: "+op.Path);if(AfterWrite!=null)AfterWrite(++i);}File.Delete(Journal);}
  catch {
   bool finished=ops.All(x=>Files.HashFile(Files.Child(Root,x.Path))==x.After);
   Recover();if(!finished)throw;
  }
 }
}

}
