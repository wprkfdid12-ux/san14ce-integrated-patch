using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace San14Patch101 {
// R43.28(사용자 11-11): 깃허브 공개판 확인·새 판 받기·버전 되돌리기.
//  깃허브 저장소 updates/index.json(판 목록)을 설치기를 켤 때 한 번만 읽는다(계속 읽지 않음).
//  updates 폴더의 파일:
//   SAN14CE_IntegratedPatch_<판>_<게임판 8자리>.zip  = 기본판(11월 7일 최종판)과 다른 파일 + 전체 목록 SHA256SUMS.txt
//   base/SAN14CE_IntegratedPatch_base_<이름>_<n>of<m>.zip = 기본판 전체를 나눈 조각(깃허브 파일 한도 100MB 미만). 지금 폴더에 없는 파일이 있을 때만, 필요한 조각까지만 받는다.
//  새 판은 지금 설치기 폴더 옆 '통합패치 <판>' 폴더에 만든다(지금 폴더·게임 폴더는 건드리지 않음). 모든 파일은 SHA-256 으로 확인.
public sealed class Release {
 public string Tag="",Title="",Notes="",Date="",AssetName="",AssetUrl="",AssetSha="";
 public long AssetSize; public uint Stamp; public int Major,Minor;
 public readonly List<Release> BaseParts=new List<Release>();   // 기본판 조각(AssetName·AssetUrl·AssetSha·AssetSize 만 씀)
 public string Mark="";   // 목록 표시용 꼬리(최신판·지금 설치기)
 public bool HasPatch{get{return AssetUrl!="";}}
 public override string ToString(){return Tag+"   "+Date+Mark;}
 public string Label(string installed,string latest){
  string s=Tag+"   "+Date;
  if(Tag==latest)s+="   (최신판)";
  if(Tag==installed)s+="   (지금 설치기)";
  return s;
 }
}
public static class Updates {
 public const string Repository="wprkfdid12-ux/san14ce-integrated-patch";
 public const string PagesUrl="https://github.com/"+Repository;
 // R43.28(사용자 11-11): 이 작업 환경에서 깃허브 '배포' 칸에 못 올려서, 새 판은 저장소 안 updates 폴더에 둔다(설치기는 그 판 목록을 읽음)
 const string Raw="https://raw.githubusercontent.com/"+Repository+"/main/";
 const string Api=Raw+"updates/index.json";
  public const uint GameStamp=0x6ad06f53u;   // 이 판이 지원하는 게임 실행 파일 빌드 시각(Spec.GameLabel)
 // 판 번호: R43.28 → (43,28). 모르는 모양은 (0,0)
 public static void ParseTag(string tag,out int major,out int minor){
  major=0;minor=0;var m=Regex.Match(tag??"",@"^R(\d+)\.(\d+)$");
  if(m.Success){major=Int32.Parse(m.Groups[1].Value);minor=Int32.Parse(m.Groups[2].Value);}
 }
 public static int Compare(string a,string b){int a1,a2,b1,b2;ParseTag(a,out a1,out a2);ParseTag(b,out b1,out b2);return a1!=b1?a1.CompareTo(b1):a2.CompareTo(b2);}
 static HttpWebRequest Request(string url){
  ServicePointManager.SecurityProtocol|=(SecurityProtocolType)3072;   // TLS 1.2
  var q=(HttpWebRequest)WebRequest.Create(url);q.UserAgent="SAN14CE-IntegratedPatch-Installer";q.Accept="application/vnd.github+json";q.Timeout=20000;q.ReadWriteTimeout=60000;q.AllowAutoRedirect=true;
  return q;
 }
 static string Str(Dictionary<string,object> d,string k){object v;return d!=null&&d.TryGetValue(k,out v)&&v!=null?v.ToString():"";}
 static long Num(Dictionary<string,object> d,string k){long n;return Int64.TryParse(Str(d,k),out n)?n:0;}
 // 깃허브 저장소의 판 목록(updates/index.json) → 판 목록(새 판이 앞)
 public static List<Release> Fetch(){
  string text;
  using(var r=Request(Api).GetResponse())using(var s=new StreamReader(r.GetResponseStream(),Encoding.UTF8))text=s.ReadToEnd();
  return Parse(text);
 }
 // updates/index.json 형식: {"releases":[{"tag","date","title","notes","stamp","file","sha256","size"}…], "base":[{"file","sha256","size"}…]}
 //  file 은 저장소 안 경로(updates/…). 내려받기 주소 = raw.githubusercontent.com/<저장소>/main/<file>
 public static List<Release> Parse(string json){
  var list=new List<Release>();
  var root=new JavaScriptSerializer{MaxJsonLength=1<<26}.DeserializeObject(json) as Dictionary<string,object>;
  if(root==null)throw new IOException("판 목록(index.json) 형식이 예상과 다릅니다.");
  var parts=new List<Release>();object bo;root.TryGetValue("base",out bo);
  var bl=bo as object[];
  if(bl!=null)foreach(object o in bl){
   var b=o as Dictionary<string,object>;if(b==null)continue;string file=Str(b,"file");if(!SafeFile(file))continue;
   parts.Add(new Release{AssetName=Path.GetFileName(file),AssetUrl=Raw+file,AssetSha=Str(b,"sha256").ToLowerInvariant(),AssetSize=Num(b,"size")});
  }
  parts.Sort(delegate(Release x,Release y){return String.CompareOrdinal(x.AssetName,y.AssetName);});
  object ro;root.TryGetValue("releases",out ro);
  var rl=ro as object[];
  if(rl!=null)foreach(object o in rl){
   var d=o as Dictionary<string,object>;if(d==null)continue;
   var r=new Release{Tag=Str(d,"tag"),Title=Str(d,"title"),Date=Str(d,"date"),Notes=Str(d,"notes").Replace("\r\n","\n").Replace("\n","\r\n")};
   ParseTag(r.Tag,out r.Major,out r.Minor);if(r.Major==0)continue;
   string file=Str(d,"file");if(!SafeFile(file))continue;
   r.AssetName=Path.GetFileName(file);r.AssetUrl=Raw+file;r.AssetSha=Str(d,"sha256").ToLowerInvariant();r.AssetSize=Num(d,"size");
   uint st;if(!UInt32.TryParse(Str(d,"stamp"),System.Globalization.NumberStyles.HexNumber,null,out st))continue;r.Stamp=st;
   r.BaseParts.AddRange(parts);
   list.Add(r);
  }
  list.Sort(delegate(Release x,Release y){return Compare(y.Tag,x.Tag);});
  return list;
 }
 static bool SafeFile(string f){return f.StartsWith("updates/")&&!f.Contains("..")&&!f.Contains("\\")&&f.EndsWith(".zip",StringComparison.OrdinalIgnoreCase);}
 public static Release Latest(List<Release> all){return all==null?null:all.FirstOrDefault(r=>r.HasPatch);}
 // 게임 실행 파일 빌드 시각(PE 머리). 못 읽으면 0
 public static uint ExeStamp(string gameRoot){
  try{
   string p=Path.Combine(gameRoot,"SAN14CE_KO.exe");if(!File.Exists(p))return 0;
   byte[] b=new byte[4096];int got=0;
   using(var f=new FileStream(p,FileMode.Open,FileAccess.Read,FileShare.ReadWrite)){int k;while(got<b.Length&&(k=f.Read(b,got,b.Length-got))>0)got+=k;}
   if(got<0x200||b[0]!='M'||b[1]!='Z')return 0;int pe=BitConverter.ToInt32(b,0x3c);if(pe<0x40||pe+12>got||BitConverter.ToUInt32(b,pe)!=0x4550)return 0;
   return BitConverter.ToUInt32(b,pe+8);
  }catch(Exception){return 0;}
 }
 public static string StampText(uint s){
  if(s==GameStamp)return Spec.GameLabel;if(s==0x6a983c25u)return "옛 1.0.1(2026-09-02)";if(s==0x6a6d9f72u)return "1.0.1 이전(2026-08-01)";
  return s==0?"알 수 없음":"게임판 "+s.ToString("x8");
 }
 static Dictionary<string,string> ReadSums(Stream s){
  var d=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
  using(var r=new StreamReader(s,Encoding.UTF8)){string line;while((line=r.ReadLine())!=null){
   line=line.Trim();if(line.Length<66)continue;string h=line.Substring(0,64).ToLowerInvariant();string rel=line.Substring(64).Trim().TrimStart('*').Replace('\\','/');
   if(rel==""||rel.Contains("..")||Path.IsPathRooted(rel))throw new IOException("SHA256SUMS.txt 에 이상한 경로: "+rel);d[rel]=h;}}
  return d;
 }
 static string Native(string root,string rel){return Path.Combine(root,rel.Replace('/',Path.DirectorySeparatorChar));}
 static void Download(string url,string path,long size,string what,Action<string> say,UpdateProgress p){
  string part=path+".part";
  for(int attempt=1;;attempt++){
   try{
    var q=Request(url);q.Accept="application/octet-stream";q.Timeout=60000;q.ReadWriteTimeout=120000;
    using(var r=q.GetResponse())using(var s=r.GetResponseStream())using(var f=new FileStream(part,FileMode.Create,FileAccess.Write)){
     byte[] buf=new byte[1<<16];long got=0,shown=0;int k;p.Total=size>0?size:1;p.Done=0;
     while((k=s.Read(buf,0,buf.Length))>0){
      p.Check();f.Write(buf,0,k);got+=k;p.Done=size>0?Math.Min(got,size):0;p.Text=what+" 받는 중 "+Mb(got)+(size>0?" / "+Mb(size):"");
      if(got-shown>=(4L<<20)){shown=got;say(p.Text);}
     }
     if(size>0&&got!=size)throw new IOException("받은 크기가 다릅니다("+got+" / "+size+" 바이트). 인터넷이 중간에 끊겼을 수 있습니다.");
    }
    if(File.Exists(path))File.Delete(path);File.Move(part,path);return;
   }catch(WebException e){
    try{if(File.Exists(part))File.Delete(part);}catch(Exception){}
    if(attempt>=3)throw new IOException(what+" 받기 실패(3번 시도): "+e.Message,e);
    say(what+" 받기가 끊겨 다시 시도합니다("+attempt+"/3): "+e.Message);p.Detail="끊겨서 다시 받는 중 ("+attempt+"/3)";System.Threading.Thread.Sleep(2000*attempt);
   }
  }
 }
 static string Mb(long n){return (n/1048576.0).ToString("0.0")+"MB";}
 static void Expect(string path,string sha,string what){
  if(sha=="")return;   // 깃허브가 지문을 안 알려 준 옛 배포: 안의 파일을 하나하나 SHA256SUMS 로 확인하므로 진행
  string h=Files.HashFile(path);if(h!=sha)throw new IOException(what+" 지문(SHA-256)이 깃허브 값과 다릅니다. 받는 중 파일이 깨졌을 수 있습니다. 다시 눌러 주세요.\r\n받은 것: "+h+"\r\n깃허브: "+sha);
 }
 static void Extract(ZipArchiveEntry e,string dest,string sha){
  Directory.CreateDirectory(Path.GetDirectoryName(dest));
  string tmp=dest+".tmp";e.ExtractToFile(tmp,true);
  string h=Files.HashFile(tmp);if(h!=sha){File.Delete(tmp);throw new IOException("압축 안 파일 지문이 목록과 다릅니다: "+e.FullName);}
  if(File.Exists(dest))File.Delete(dest);File.Move(tmp,dest);
 }
 // 새 판 폴더 만들기. 성공하면 그 폴더 경로. 실패하면 이유를 담은 예외(지금 폴더·게임 폴더는 그대로).
 public static string Build(Release r,List<Release> all,string package,Action<string> say,UpdateProgress p){
  string pkg=Path.GetFullPath(package).TrimEnd(Path.DirectorySeparatorChar);
  string parent=Path.GetDirectoryName(pkg);if(String.IsNullOrEmpty(parent))throw new IOException("지금 설치기 폴더의 위 폴더를 알 수 없습니다: "+pkg);
  string dest=Path.Combine(parent,"통합패치 "+r.Tag);
  if(String.Equals(dest,pkg,StringComparison.OrdinalIgnoreCase))throw new IOException("지금 쓰는 설치기가 이미 그 판 폴더입니다: "+dest);
  string work=dest+" (받는 중)";
  try{if(Directory.Exists(work))Directory.Delete(work,true);Directory.CreateDirectory(work);}
  catch(Exception e){throw new IOException("새 판 폴더를 만들 수 없습니다(쓰기 권한·백신 확인): "+work+"\r\n"+e.Message,e);}
  say("새 판 폴더: "+dest);p.Dest=dest;
  try{
   p.Step=1;p.Detail="";
   string zip=Path.Combine(work,"_"+r.AssetName);
   Download(r.AssetUrl,zip,r.AssetSize,r.Tag+" 파일",say,p);Expect(zip,r.AssetSha,r.Tag+" 파일");
   var missing=new List<string>();Dictionary<string,string> sums;
   using(var z=ZipFile.OpenRead(zip)){
    var se=z.GetEntry("SHA256SUMS.txt");if(se==null)throw new IOException(r.AssetName+" 안에 SHA256SUMS.txt 가 없습니다(배포 파일이 잘못 올라감).");
    using(var s=se.Open())sums=ReadSums(s);
    say("파일 "+sums.Count+"개 확인 중 (바뀐 파일은 받은 것에서, 나머지는 지금 폴더에서 복사)");
    p.Step=2;p.Total=sums.Count;p.Done=0;p.Files=sums.Count;
    foreach(var kv in sums){
     p.Check();p.Detail=kv.Key;p.Text="파일 "+(p.Done+1)+" / "+sums.Count;
     string dst=Native(work,kv.Key);var e=z.GetEntry(kv.Key);
     if(e!=null){Extract(e,dst,kv.Value);p.FromZip++;}
     else{
      string src=Native(pkg,kv.Key);
      if(File.Exists(src)&&Files.HashFile(src)==kv.Value){Directory.CreateDirectory(Path.GetDirectoryName(dst));File.Copy(src,dst,true);p.FromHere++;}
      else missing.Add(kv.Key);
     }
     p.Done++;
    }
   }
   File.Delete(zip);
   if(missing.Count>0){
    var b=all.FirstOrDefault(x=>x.BaseParts.Count>0);
    if(b==null)throw new IOException("지금 폴더에 없는 기본 파일이 "+missing.Count+"개 있는데(예: "+missing[0]+"), 깃허브에 기본판 파일이 없습니다.");
    say("지금 폴더에 없는 기본 파일 "+missing.Count+"개 → 기본판 조각을 받습니다(전체 "+Mb(b.BaseParts.Sum(x=>x.AssetSize))+")");
    var still=new List<string>(missing);int k=0;
    foreach(var part in b.BaseParts){
     k++;if(still.Count==0)break;
     p.Step=1;p.Detail="지금 폴더에 없는 기본 파일 "+still.Count+"개 → 기본판 조각 "+k+" / "+b.BaseParts.Count;
     string bz=Path.Combine(work,"_"+part.AssetName);Download(part.AssetUrl,bz,part.AssetSize,"기본판 조각 "+k+"/"+b.BaseParts.Count,say,p);Expect(bz,part.AssetSha,"기본판 "+part.AssetName);
     p.Step=2;p.Total=missing.Count;
     using(var z=ZipFile.OpenRead(bz))foreach(string rel in still.ToArray()){var e=z.GetEntry(rel);if(e==null)continue;p.Check();p.Detail=rel;Extract(e,Native(work,rel),sums[rel]);still.Remove(rel);p.FromBase++;p.Done=p.FromBase;p.Text="기본 파일 "+p.FromBase+" / "+missing.Count;}
     File.Delete(bz);
    }
    if(still.Count>0)throw new IOException("기본판에도 없는 파일이 "+still.Count+"개 있습니다: "+String.Join(", ",still.Take(5).ToArray()));
   }
   using(var w=new StreamWriter(Path.Combine(work,"SHA256SUMS.txt"),false,new UTF8Encoding(false)))foreach(var kv in sums.OrderBy(x=>x.Key,StringComparer.Ordinal))w.Write(kv.Value+"  "+kv.Key+"\n");
   // 마지막 확인: 목록의 모든 파일이 지문대로 있는가
   p.Step=3;p.Total=sums.Count;p.Done=0;
   foreach(var kv in sums){p.Check();p.Detail=kv.Key;p.Text="새 폴더 지문 확인 "+(p.Done+1)+" / "+sums.Count;if(Files.HashFile(Native(work,kv.Key))!=kv.Value)throw new IOException("새 판 폴더 확인 실패: "+kv.Key);p.Done++;}
   p.Check();
   if(Directory.Exists(dest)){string keep=dest+" (옛 "+DateTime.Now.ToString("yyyyMMdd_HHmmss")+")";Directory.Move(dest,keep);say("같은 이름 폴더가 있어 옆으로 옮김: "+keep);}
   Directory.Move(work,dest);
  }catch(Exception){
   try{if(Directory.Exists(work))Directory.Delete(work,true);}catch(Exception){}   // 만들던 새 폴더만 지움(지금 폴더·게임은 그대로)
   throw;
  }
  p.Step=4;p.Total=1;p.Done=1;p.Detail="";
  p.Text="받은 파일 "+p.FromZip+" · 복사 "+p.FromHere+(p.FromBase>0?" · 기본판 "+p.FromBase:"")+" · 지문 확인 "+p.Files+"개 전부 맞음";
  say("새 판 폴더 완성: "+p.Text);
  return dest;
 }
 // 새 판 설치기 열기(그 폴더의 BuildAndRun.ps1 이 컴파일해서 실행). 지금 설치기는 닫는다.
 public static void Launch(string dest,string game,string fromVersion){
  string ps=Path.Combine(Path.Combine(Path.Combine(dest,"support"),"installer101"),"BuildAndRun.ps1");
  if(!File.Exists(ps))throw new IOException("새 판 폴더에 설치기 자료가 없습니다: "+ps);
  string args="-NoProfile -ExecutionPolicy Bypass -File \""+ps+"\" -AfterUpdate \""+fromVersion+"\"";
  if(!String.IsNullOrEmpty(game))args+=" -GameDirectory \""+game.TrimEnd('\\')+"\"";
  Process.Start(new ProcessStartInfo("powershell.exe",args){UseShellExecute=true,WorkingDirectory=dest});
 }
}
// R43.30(사용자 11-11): 새 판 받기 진행 상태(받는 쪽 스레드가 쓰고 진행 창이 0.15초마다 읽음)
public sealed class UpdateProgress {
 public volatile int Step=1;   // 1 받기 2 확인 3 새 폴더 4 완료
 public long Done,Total;
 public volatile string Text="",Detail="",Dest="";
 public volatile bool Cancel;
 public int FromZip,FromHere,FromBase,Files;
 public void Check(){if(Cancel)throw new OperationCanceledException("새 판 받기를 취소했습니다.");}
}
public sealed class ProgressForm:Form {
 readonly UpdateProgress p;readonly string tag;
 readonly Label head=new Label(),line=new Label(),pct=new Label(),detail=new Label(),note=new Label();
 readonly Label[] steps=new Label[4];readonly ProgressBar bar=new ProgressBar();
 readonly Button cancel=new Button();
 readonly System.Windows.Forms.Timer timer=new System.Windows.Forms.Timer();
 int lastStep;DateTime t0=DateTime.Now;long d0;bool finished;Font fBold,fReg;
 public bool OpenNew;
 public ProgressForm(string tag,UpdateProgress p){
  this.p=p;this.tag=tag;
  Text="새 판 받기 — 통합패치 "+tag;ClientSize=new Size(600,330);StartPosition=FormStartPosition.CenterParent;FormBorderStyle=FormBorderStyle.FixedDialog;MinimizeBox=false;MaximizeBox=false;ShowInTaskbar=false;Font=new Font("Malgun Gothic",10);
  fBold=new Font("Malgun Gothic",10,FontStyle.Bold);fReg=new Font("Malgun Gothic",10);
  head.SetBounds(18,14,560,30);head.Font=new Font(Font.FontFamily,13,FontStyle.Bold);head.Text="통합패치 "+tag+" 받는 중";Controls.Add(head);
  string[] names={"1 받기","2 확인","3 새 폴더","4 완료"};
  for(int i=0;i<4;i++){var l=new Label{Text=names[i],TextAlign=ContentAlignment.MiddleCenter,AutoSize=false};l.SetBounds(18+i*142,52,136,30);steps[i]=l;Controls.Add(l);}
  bar.SetBounds(18,94,564,24);bar.Minimum=0;bar.Maximum=1000;Controls.Add(bar);
  line.SetBounds(18,124,460,24);Controls.Add(line);
  pct.SetBounds(480,124,102,24);pct.TextAlign=ContentAlignment.TopRight;pct.Font=new Font(Font,FontStyle.Bold);Controls.Add(pct);
  detail.SetBounds(18,152,564,44);detail.AutoEllipsis=true;Controls.Add(detail);
  note.SetBounds(18,200,564,60);note.ForeColor=Color.FromArgb(90,90,90);note.Font=new Font(Font.FontFamily,9);note.Text="지금 폴더와 게임은 그대로입니다. 취소하면 만들던 새 폴더만 지웁니다.";Controls.Add(note);
  cancel.Text="취소";cancel.SetBounds(482,276,100,34);Controls.Add(cancel);
  cancel.Click+=delegate{p.Cancel=true;cancel.Enabled=false;cancel.Text="취소하는 중…";};
  FormClosing+=delegate(object s,FormClosingEventArgs e){if(!finished){e.Cancel=true;p.Cancel=true;cancel.Enabled=false;cancel.Text="취소하는 중…";}};
  timer.Interval=150;timer.Tick+=delegate{Show2();};timer.Start();Show2();
 }
 void Show2(){
  int st=Math.Max(1,Math.Min(4,p.Step));
  if(st!=lastStep){lastStep=st;t0=DateTime.Now;d0=p.Done;}
  for(int i=0;i<4;i++){bool now=i==st-1,done=i<st-1||st==4;steps[i].BackColor=now?Color.FromArgb(42,111,214):done?Color.FromArgb(207,232,212):Color.FromArgb(221,221,221);steps[i].ForeColor=now?Color.White:done?Color.FromArgb(29,107,47):Color.FromArgb(119,119,119);steps[i].Font=now?fBold:fReg;}
  long total=p.Total,done2=p.Done;int per=total>0?(int)Math.Min(1000,done2*1000/total):0;
  bar.Value=per;pct.Text=(per/10)+"%";line.Text=p.Text;
  string d=p.Detail;
  if(st==1&&total>1&&done2>d0){double sec=(DateTime.Now-t0).TotalSeconds;if(sec>1){double rate=(done2-d0)/sec;if(rate>0){int rem=(int)((total-done2)/rate);d=(d!=""?d+"\r\n":"")+"남은 시간 약 "+(rem>=60?rem/60+"분 "+rem%60+"초":rem+"초");}}}
  detail.Text=d;
  if(st==1)head.Text="통합패치 "+tag+" 받는 중";else if(st==2)head.Text="파일 확인 중";else if(st==3)head.Text="새 폴더 확인 중";
 }
 // 받는 쪽이 끝났을 때(UI 스레드). 성공이면 완료 화면을 잠깐 보이고 바로 닫음 → 새 판 설치기가 열림(사용자 11-11: 버튼 한 번 더 누르는 과정 없이). 실패면 바로 닫음(호출한 쪽이 오류 창)
 public void Finish(bool ok){
  finished=true;timer.Stop();
  if(!ok){Close();return;}
  p.Step=4;Show2();bar.Value=1000;pct.Text="100%";head.Text="새 판을 만들었습니다 — 새 판 설치기를 엽니다";
  detail.Text="새 폴더: "+p.Dest;note.Text="이 창과 지금 설치기는 닫히고 새 판 설치기가 열립니다. 앞으로는 새 폴더의 설치.cmd 를 쓰세요.";
  cancel.Visible=false;OpenNew=true;
  var t=new System.Windows.Forms.Timer{Interval=1200};t.Tick+=delegate{t.Stop();t.Dispose();Close();};t.Start();
 }
}
// 버전 되돌리기·판 고르기 창, 새 판 확인 창
public static class ReleasePicker {
 public static bool Confirm(IWin32Window owner,string title,string head,string notes,string yes){
  bool ok=false;
  using(var f=new Form{Text=title,ClientSize=new Size(720,480),StartPosition=FormStartPosition.CenterParent,MinimizeBox=false,MaximizeBox=false,ShowInTaskbar=false,Font=new Font("Malgun Gothic",10)}){
   var top=new Label{Dock=DockStyle.Top,Height=64,Padding=new Padding(10,8,10,0),Text=head};
   var box=new TextBox{Dock=DockStyle.Fill,Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,BackColor=SystemColors.Window,Text=notes};
   var panel=new FlowLayoutPanel{Dock=DockStyle.Bottom,Height=46,FlowDirection=FlowDirection.RightToLeft,Padding=new Padding(6)};
   var no=new Button{Text="취소",AutoSize=true,Height=32};var go=new Button{Text=yes,AutoSize=true,Height=32};
   go.Click+=delegate{ok=true;f.Close();};no.Click+=delegate{f.Close();};
   panel.Controls.Add(no);panel.Controls.Add(go);f.Controls.Add(box);f.Controls.Add(panel);f.Controls.Add(top);f.AcceptButton=go;f.CancelButton=no;
   f.ShowDialog(owner);
  }
  return ok;
 }
 public static Release Pick(IWin32Window owner,List<Release> all,uint gameStamp){
  Release result=null;
  var shown=all.Where(r=>r.HasPatch).ToList();
  string latest=Updates.Latest(all)!=null?Updates.Latest(all).Tag:"";
  using(var f=new Form{Text="버전 되돌리기",ClientSize=new Size(720,460),StartPosition=FormStartPosition.CenterParent,MinimizeBox=false,MaximizeBox=false,ShowInTaskbar=false,Font=new Font("Malgun Gothic",10)}){
   var head=new Label{Dock=DockStyle.Top,Height=58,Padding=new Padding(10,8,10,0),Text="설치할 판을 고르세요. 고른 판의 새 폴더를 만들고 그 판 설치기를 엽니다.\r\n세이브·게임 원본 파일·지금 폴더는 건드리지 않습니다. (지금 설치기: "+Spec.Version+")"};
   var list=new ListBox{Dock=DockStyle.Fill,IntegralHeight=false,DrawMode=DrawMode.OwnerDrawFixed,ItemHeight=26};
   var notes=new TextBox{Dock=DockStyle.Bottom,Height=150,Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,BackColor=SystemColors.Window};
   var panel=new FlowLayoutPanel{Dock=DockStyle.Bottom,Height=46,FlowDirection=FlowDirection.RightToLeft,Padding=new Padding(6)};
   var close=new Button{Text="닫기",AutoSize=true,Height=32};var go=new Button{Text="이 판으로 설치",AutoSize=true,Height=32,Enabled=false};
   foreach(var r in shown)list.Items.Add(r);
   Func<Release,bool> ok=r=>gameStamp==0||r.Stamp==gameStamp;
   list.DrawItem+=delegate(object s,DrawItemEventArgs e){
    if(e.Index<0)return;var r=(Release)list.Items[e.Index];bool can=ok(r);e.DrawBackground();
    string t=r.Label(Spec.Version,latest)+(can?"":"   — 다른 게임판("+Updates.StampText(r.Stamp)+") 전용이라 설치 불가");
    TextRenderer.DrawText(e.Graphics,t,list.Font,new Point(e.Bounds.X+6,e.Bounds.Y+3),!can?Color.Gray:(e.State&DrawItemState.Selected)!=0?SystemColors.HighlightText:SystemColors.ControlText);
   };
   list.SelectedIndexChanged+=delegate{var r=list.SelectedItem as Release;go.Enabled=r!=null&&ok(r)&&r.Tag!=Spec.Version;notes.Text=r==null?"":r.Title+"  ("+r.Date+")\r\n\r\n"+r.Notes;};
   go.Click+=delegate{result=list.SelectedItem as Release;f.Close();};close.Click+=delegate{f.Close();};
   panel.Controls.Add(close);panel.Controls.Add(go);
   f.Controls.Add(list);f.Controls.Add(notes);f.Controls.Add(panel);f.Controls.Add(head);
   if(shown.Count==0)notes.Text="깃허브에 올라온 판이 아직 없습니다.";
   f.ShowDialog(owner);
  }
  return result;
 }
}
}
