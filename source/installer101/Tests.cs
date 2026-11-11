using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace San14Patch101 {
// 리눅스 하네스용 자체 시험: 가짜 게임 폴더에서 설치·갱신·제거·이어받기(R20 기록)·충돌 보존을 확인
public static class Tests {
 static string ShaSource(Manifest m,string target,bool[] sel){foreach(var f in m.Entries)if(f.Target==target&&f.Applies(sel))return f.Source;return "";}
 static string ShaOf(Manifest m,string target,bool[] sel){foreach(var f in m.Entries)if(f.Target==target&&f.Applies(sel))return f.Sha;return "";}
 static int fails;static string pkg,work,gameExe,fixOrig,expOrig,fontOrig;static Manifest M;
 static void Check(bool ok,string what){Console.WriteLine((ok?"PASS ":"FAIL ")+what);if(!ok)fails++;}
 static string H(string root,string rel){return Files.HashFile(Path.Combine(root,rel.Replace('/',Path.DirectorySeparatorChar)));}
 static void Put(string root,string rel,byte[] b){string p=Path.Combine(root,rel.Replace('/',Path.DirectorySeparatorChar));Directory.CreateDirectory(Path.GetDirectoryName(p));File.WriteAllBytes(p,b);}
 static string NewGame(string name){
  string root=Path.Combine(work,name);if(Directory.Exists(root))Directory.Delete(root,true);Directory.CreateDirectory(root);
  File.Copy(gameExe,Path.Combine(root,"SAN14CE_KO.exe"));File.WriteAllBytes(Path.Combine(root,"SAN14_Launcher.exe"),new byte[]{1});
  Put(root,Engine.FixRel,File.ReadAllBytes(fixOrig));Put(root,Engine.ExpRel,File.ReadAllBytes(expOrig));Put(root,Engine.FontRel,File.ReadAllBytes(fontOrig));
  Put(root,"0002/data/0x9da765bf.file",new byte[]{9});
  foreach(string r in Engine.ModelIndexRels)Put(root,r,File.ReadAllBytes(Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"model101"),Path.GetFileName(r))));Put(root,"San14BgmExpansion/music/user_song.mp3",Encoding.ASCII.GetBytes("user"));
  string so=Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"screen101"),"ScreenLayout.rdb");if(File.Exists(so))Put(root,Engine.ScreenIndexRel,File.ReadAllBytes(so));
  return root;
 }
 static Dictionary<string,string> Snapshot(string root){var d=new Dictionary<string,string>();foreach(string f in Directory.GetFiles(root,"*",SearchOption.AllDirectories)){string rel=f.Substring(root.Length+1).Replace('\\','/');if(rel.StartsWith(".san14ce_compat_r3/"))continue;d[rel]=Files.HashFile(f);}return d;}
 // 색인에서 레코드 하나의 위치 문자열 한 글자를 바꿈(다른 도구가 다른 레코드를 고친 상황 흉내)
 static byte[] Tweak(byte[] b,uint id){b=(byte[])b.Clone();long pos=32;uint n=BitConverter.ToUInt32(b,16);
  for(uint i=0;i<n;i++){long size=BitConverter.ToInt64(b,(int)pos+8),ls=BitConverter.ToInt64(b,(int)pos+16);if(BitConverter.ToUInt32(b,(int)pos+36)==id){int k=(int)(pos+size-ls);b[k]=(byte)(b[k]=='1'?'2':'1');return b;}pos=(pos+size+3)&~3L;}
  throw new Exception("no record "+id.ToString("x"));}
 static Engine Make(string root){var e=new Engine(root,pkg,M);e.Log=delegate(string s){Console.WriteLine("   log: "+s);};return e;}
 static bool[] All(){var a=Enumerable.Repeat(true,Spec.Count).ToArray();a[Spec.Fog-1]=false;return a;} // R41: 12번 삭제
 static bool[] Only(params int[] ids){var b=new bool[Spec.Count];foreach(int i in ids)b[i-1]=true;return b;}
 static Dictionary<string,string> Ini(string root,string rel){return Engine.ParseIni(Engine.Text(File.ReadAllBytes(Path.Combine(root,rel))));}
 static bool Throws(Action a,string contains,out string msg){try{a();msg="";return false;}catch(Exception e){msg=e.Message;return e.Message.Contains(contains);}}

 public static int Main(string[] args){
  pkg=args[0];work=args[1];gameExe=args[2];fixOrig=Path.Combine(pkg,"data","fixdata_original_101.s14");expOrig=Path.Combine(pkg,"data","experience_original.bin");fontOrig=args[3];
  string sep=Path.DirectorySeparatorChar.ToString();
  Environment.SetEnvironmentVariable("LOCALAPPDATA",Path.Combine(work,"localappdata"));
  Check(Path.GetFullPath(Spec.FindPackageRoot(Path.Combine(Path.Combine(pkg,"support"),"installer101")+sep))==Path.GetFullPath(pkg).TrimEnd(Path.DirectorySeparatorChar),"launch path: package root found from support/installer101 (EXE location)");
  Check(Path.GetFullPath(Spec.FindPackageRoot(pkg))==Path.GetFullPath(pkg).TrimEnd(Path.DirectorySeparatorChar),"launch path: package root found from root");
  M=Manifest.Load(Path.Combine(pkg,"support","manifest101.xml"),ManifestHash.Value);
  Check(M.Entries.Count>50&&M.Known.Count>100,"manifest loaded: entries="+M.Entries.Count+" known="+M.Known.Count);
  Check(Files.HashFile(gameExe)==Spec.ExpectedGame,"test game exe hash matches Spec.ExpectedGame");
  Check(Files.HashFile(fontOrig)==M.Values["font_original"],"font original hash");
  if(args.Length>6&&args[6]=="split"){ // R29: ZIP 을 따로 푼 경우 — 1번만 있는 폴더 + 옆 폴더(2번 자료)
   string baseDir=Path.Combine(work,"splitpkg");if(Directory.Exists(baseDir))Directory.Delete(baseDir,true);
   string A=Path.Combine(baseDir,"pkg_1of2"),B=Path.Combine(baseDir,"pkg_2of2");
   foreach(string f in Directory.GetFiles(pkg,"*",SearchOption.AllDirectories)){string rel=f.Substring(pkg.TrimEnd(Path.DirectorySeparatorChar).Length+1);
    bool second=rel.Replace('\\','/').StartsWith("support/payload/f08/")||rel.Replace('\\','/').StartsWith("support/payload/f12/")||rel.Replace('\\','/').StartsWith("support/payload/f20/")||rel.Replace('\\','/')=="support/payload/base/SAN14_IntegratedConvenience.dll";
    string dst=Path.Combine(second?B:A,rel);Directory.CreateDirectory(Path.GetDirectoryName(dst));File.Copy(f,dst);}
   var sel=All();string root=NewGame("split");
   var ea=new Engine(root,A,M);ea.Log=delegate(string s2){};
   Check(ea.MissingPayloads(sel).Count==0,"split: sibling folder found (nothing missing)");
   string msg="";try{ea.Install(sel,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","split: install ok using sibling folder "+msg);
   int bad=0;foreach(var f in M.Entries)if(f.Applies(sel)&&!f.Target.EndsWith(".ini")&&H(root,f.Target)!=f.Sha)bad++;
   Check(bad==0,"split: all files in place");
   File.Copy(Path.Combine(B,"support","payload","base","SAN14_IntegratedConvenience.dll"),Path.Combine(A,"support","payload","base","SAN14_IntegratedConvenience.dll")); // R29 배포 구성: 공통 자료는 1번 ZIP
   Directory.Delete(B,true);string root2=NewGame("split2");var eb=new Engine(root2,A,M);eb.Log=delegate(string s2){};
   msg="";try{eb.Install(sel,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg.Contains("같은 새 폴더")&&!msg.Contains("인덱스"),"split: second ZIP missing -> clear message: "+msg.Split('\n')[0]);
   Check(Directory.GetFiles(root2,"*",SearchOption.AllDirectories).Length==Directory.GetFiles(NewGame("split3"),"*",SearchOption.AllDirectories).Length,"split: game folder untouched when missing");
   msg="";try{var part=All();part[7]=false;part[11]=false;part[19]=false;eb.Install(part,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","split: without second ZIP, items not needing it still install "+msg);
   Console.WriteLine(fails+" FAILURES");return fails==0?0:1;
  }
  if(args.Length>7&&args[6]=="upgradeprev"){ // 직전 배포판(R27) 설치 → 이번 판 설치: 전체 켬, 9번만(13 없이) 두 경우
   string prev=args[7];var MP=Manifest.Load(Path.Combine(Path.Combine(prev,"support"),"manifest101.xml"),Files.HashFile(Path.Combine(Path.Combine(prev,"support"),"manifest101.xml")));
   { // R41: 직전 판에서 12번(안개)을 깔았던 사용자 → 이번 판 설치 때 안개 파일 지우고 11번은 일반판
    var sel=All();sel[Spec.Fog-1]=true;sel[26]=false;string root=NewGame("upprev_fog");
    Engine.KeepRetired=true;var ep=new Engine(root,prev,MP);ep.Log=delegate(string s){};ep.Install(sel,null);Engine.KeepRetired=false;
    Check(H(root,"SAN14_FogTrial.dll")!=""&&H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll")!="","fog: previous build installed with 12");
    var ls=Make(root).Load();Check(ls.RetiredOn&&!ls.On[Spec.Fog-1],"fog: record read as retired 12");
    string msg="";try{Make(root).Install(All(),null);}catch(Exception ex){msg=ex.Message;}Check(msg=="","fog: new install ok "+msg);
    Check(H(root,"SAN14_FogTrial.dll")==""&&H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll")==""&&H(root,"SAN14_FogFieldPair_Entry.dll")==""&&H(root,"SAN14Mods/modules/FogFieldPair.ini")=="","fog: fog files removed");
    string nr=M.Entries.First(f=>f.Target.StartsWith("FieldLeader3D")&&f.When=="11&!12").Sha;Check(H(root,"FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe")==nr,"fog: 11 now normal runtime");
    var st=Make(root).Status(Make(root).Load());Check(st.Where((x,i)=>All()[i]).All(x=>x.StartsWith("설치됨"))&&st[Spec.Fog-1]=="미설치","fog: status ok, 12 미설치");
    Check(!Make(root).Load().RetiredOn,"fog: retired flag gone after install");
   }
   foreach(var mode in new[]{"all","t9"}){
    var sel=All();if(mode=="t9"){sel[12]=false;sel[13]=false;sel[11]=false;}
    string root=NewGame("upprev_"+mode);
    var ep=new Engine(root,prev,MP);ep.Log=delegate(string s){};ep.Install(sel,null);
    string hov=mode=="all"?"SAN14Mods/MeritTraits/UI_Traits9_Hover.dll":"SAN14_Traits9_UI.dll";
    string prevHov=ShaOf(MP,hov,sel),newHov=ShaOf(M,"SAN14_Traits9_UI.dll",sel); /* R42 A10: 이번 판은 늘 게임 폴더 공용 이름 */Check(prevHov!=""&&H(root,hov)==prevHov,mode+": previous build installed (trait UI file)");
    string msg="";try{Make(root).Install(sel,null);}catch(Exception ex){msg=ex.Message;}
    Check(msg=="",mode+": new over previous: install ok "+msg);
    int bad=0;foreach(var f in M.Entries)if(f.Applies(sel)&&!f.Target.EndsWith(".ini")&&H(root,f.Target)!=f.Sha){bad++;Console.WriteLine("   mismatch "+f.Target);}
    Check(bad==0,mode+": new files all in place after upgrade");
    if(mode=="all")Check(H(root,hov)=="","all: old merit-folder trait UI copy removed (R42 A10)");
    Check(newHov!=""&&H(root,"SAN14_Traits9_UI.dll")==newHov,mode+": trait UI file is this build's ("+newHov.Substring(0,16)+(newHov==prevHov?", same as previous":", previous "+prevHov.Substring(0,16))+")");
    if(mode=="all"){string rv="SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll";Check(ShaOf(M,rv,sel)!=""&&H(root,rv)==ShaOf(M,rv,sel),"all: merit resolver is this build's ("+ShaOf(M,rv,sel).Substring(0,16)+")");}
    var st=Make(root).Status(Make(root).Load());Check(st.Where((x,i)=>sel[i]).All(x=>x.StartsWith("설치됨")),mode+": status all installed after upgrade");
    Make(root).Install(new bool[Spec.Count],null);
    Check(H(root,hov)=="",mode+": remove all after upgrade: trait UI file gone");
   }
   Console.WriteLine(fails+" FAILURES");return fails==0?0:1;
  }
  if(args.Length>8&&args[6]=="userstate"){ // R37: 사용자 PC 실제 기록(R35 설치 뒤 맵 확장 도구가 KIDS 를 원본으로 되돌림) → 전부 제거 → 다시 전체 설치
   string root=NewGame("userstate");var tx=new Transaction(root);var doc=Files.Xml(args[7]);
   var byHash=new Dictionary<string,string>();foreach(string dir in new[]{pkg,args[8]})foreach(string f in Directory.GetFiles(dir,"*",SearchOption.AllDirectories))byHash[Files.HashFile(f)]=f;
   string pay=Path.Combine(Path.Combine(Path.Combine(pkg,"support"),"payload"),"f10");
   foreach(var el in doc.Root.Element("Files").Elements("File")){
    string path=Files.Attr(el,"path"),aft=Files.Attr(el,"after"),bef=Files.Attr(el,"before");byte[] b=null;
    if(path=="0002/KIDSSystemResource.rdb")b=File.ReadAllBytes(Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"model101"),"KIDSSystemResource.rdb")); // 지금 PC: 진짜 원본으로 되돌려짐
    else if(byHash.ContainsKey(aft))b=File.ReadAllBytes(byHash[aft]);
    else if(path==Engine.FixRel||path==Engine.ExpRel)b=null;
    else{b=Encoding.UTF8.GetBytes("r35 file "+path);el.SetAttributeValue("after",Files.Hash(b));}
    if(b!=null)Put(root,path,b);
    if(bef!=""&&path!=Engine.FixRel&&path!=Engine.ExpRel){if(byHash.ContainsKey(bef))tx.Put(File.ReadAllBytes(byHash[bef]));else el.SetAttributeValue("before",tx.Put(Encoding.UTF8.GetBytes("before "+path)));}
   }
   Put(root,Engine.StateRel,Files.XmlBytes(doc));
   var e=Make(root);var old=e.Load();Check(old.On[9]&&old.Items.ContainsKey("0002/KIDSSystemResource.rdb"),"userstate: R35 record loaded");
   string msg="";try{e.Install(new bool[Spec.Count],null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","userstate: remove all works "+msg.Replace("\r\n"," / "));
   Check(Engine.ModelIndexRels.All(r=>H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)])&&H(root,"0002/MaterialEditor.rdb.bin3369")==""&&H(root,Engine.ModelCatalogRel)=="","userstate: indexes 1.0.1 original, model files gone");
   msg="";try{Make(root).Install(All(),null);}catch(Exception ex){msg=ex.Message;}
   var st=Make(root).Status(Make(root).Load());Check(msg==""&&st[9]=="설치됨 (추가 모델 149개)","userstate: full install again "+st[9]+" "+msg);
   Console.WriteLine(fails+" FAILURES");return fails==0?0:1;
  }
  if(args.Length>7&&args[6]=="upgrade23"){ // R23(배포판, 12번 안개 포함) 설치 → 이번 판 설치
   string r23=args[7];var M23=Manifest.Load(Path.Combine(Path.Combine(r23,"support"),"manifest101.xml"),Files.HashFile(Path.Combine(Path.Combine(r23,"support"),"manifest101.xml")));
   string root=NewGame("upgrade23");var sel=All();
   var e23=new Engine(root,r23,M23);e23.Log=delegate(string s){Console.WriteLine("   r23: "+s);};e23.Install(sel,null);
   string oldEng=H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll");Check(oldEng.StartsWith("c2a8f626"),"r23 installed (fog 1.6 original engine)");
   File.WriteAllText(Path.Combine(root,Engine.FogIniRel),"Wind=0\r\n");
   string msg="";try{Make(root).Install(sel,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","new over r23: install ok "+msg);
   int bad=0;foreach(var f in M.Entries)if(f.Applies(sel)&&!f.Target.EndsWith(".ini")&&H(root,f.Target)!=f.Sha){bad++;Console.WriteLine("   mismatch "+f.Target);}
   Check(bad==0,"new files all in place after upgrade");
   Check(!H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll").StartsWith("c2a8f626"),"fog engine replaced by new build");
   Check(File.ReadAllText(Path.Combine(root,Engine.FogIniRel))=="Wind=0\r\n","FogAI.ini (user file) untouched");
   var st=Make(root).Status(Make(root).Load());Check(st.Where((x,i)=>sel[i]).All(x=>x.StartsWith("설치됨")),"status all installed after upgrade");
   var e3=Make(root);e3.Install(new bool[Spec.Count],null);
   Check(File.ReadAllText(Path.Combine(root,Engine.FogIniRel))=="Wind=0\r\n","remove all: user FogAI.ini kept");
   Console.WriteLine(fails+" FAILURES");return fails==0?0:1;
  }
  if(args.Length>7&&args[6]=="upgrade"){ // R21(배포판) 설치 → 진단용 직접 교체 파일 → R22 설치
   string r21=args[7];var M21=Manifest.Load(Path.Combine(Path.Combine(r21,"support"),"manifest101.xml"),Files.HashFile(Path.Combine(Path.Combine(r21,"support"),"manifest101.xml")));
   string root=NewGame("upgrade");var sel=All();sel[11]=false;
   var e21=new Engine(root,r21,M21);e21.Log=delegate(string s){Console.WriteLine("   r21: "+s);};e21.Install(sel,null);
   Check(Files.HashFile(Path.Combine(root,"FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe".Replace('/',Path.DirectorySeparatorChar)))=="156659b9aafea42e4747aa54023891264701619646920f6a50b10ac442d014a3","r21 installed (3D runtime r21)");
   foreach(var hf in new[]{new[]{"FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe",args[8]},new[]{"SAN14_CrashLog.dll",args[9]},new[]{"SAN14Mods/modules/CrashLog.ini",args[10]}})Put(root,hf[0],File.ReadAllBytes(hf[1]));
   string msg="";try{Make(root).Install(sel,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","r22 over r21+hotfix: install ok "+msg);
   int bad=0;foreach(var f in M.Entries)if(f.Applies(sel)&&!f.Target.EndsWith(".ini")&&H(root,f.Target)!=f.Sha){bad++;Console.WriteLine("   mismatch "+f.Target);}
   Check(bad==0,"r22 files all in place after upgrade");
   var st=Make(root).Status(Make(root).Load());Check(st.Where((x,i)=>sel[i]).All(x=>x.StartsWith("설치됨")),"status all installed after upgrade");
   var ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tAudienceButton"]=="1"&&ini["Convenience\tAudienceGate"]!="9","audience keys after upgrade");
   Console.WriteLine(fails+" FAILURES");return fails==0?0:1;
  }
  if(args.Length>6&&args[6]=="fullonly"){string r=NewGame("emu_full");Make(r).Install(All(),null);string r2=NewGame("emu_sub");var sub=All();sub[8]=false;sub[12]=false;sub[11]=false;Make(r2).Install(sub,null);string r3=NewGame("emu_t9");var t9=All();t9[12]=false;t9[13]=false;t9[11]=false;Make(r3).Install(t9,null);string r4=NewGame("emu_gsjin");var gs=All();foreach(int k in new[]{10,11,12,18,22,24,25})gs[k-1]=false;Make(r4).Install(gs,null);Console.WriteLine("prepared "+r+" and "+r2+" and "+r3+" and "+r4);return 0;}
  // 1. 전체 설치 → 부분 변경 → 전부 제거
  {
   string root=NewGame("full");var before=Snapshot(root);
   var e=Make(root);int n=e.Install(All(),null);Check(n>60,"full install ops="+n);
   var s=e.Load();Check(s.Version==7&&s.On.Where((x,i)=>i!=Spec.Fog-1).All(x=>x)&&!s.On[Spec.Fog-1],"state v7 all on (R41: 12 retired off)");
   foreach(var f in M.Entries)if(f.Applies(All()))Check(H(root,f.Target)==f.Sha||f.Target.EndsWith(".ini"),"file present "+f.Target);
   Check(H(root,Engine.FixRel)==M.Values["fix_nostat"],"24: fixdata nostat");
   Check(H(root,Engine.ExpRel)==M.Values["exp_modified"],"18: experience modified");
   Check(H(root,Engine.FontRel)==M.Values["font_rdb"]&&H(root,Engine.FontBinRel)!="","20: font files");
   Check(H(root,"dxgi.dll")==M.Values["loader_dxgi.dll"]&&H(root,"version.dll")==""&&H(root,"dinput8.dll")=="","loader dxgi only");
   var ini=Ini(root,Engine.SettingsRel);
   Check(ini["Convenience\tDualCredit"]=="0"&&ini["Modules\tTraits9Module"]=="0"&&ini["Camera\tAutoFocus"]=="0"&&ini["Convenience\tAudienceGate"]=="2"&&ini["Convenience\tNoPositionTroops"]=="5000"&&ini["Modules\tModSettingsUI"]=="1","settings ini defaults (Traits9Module=0 because 13 on)");
   var br=Ini(root,Engine.BridgeIniRel);Check(br["ModPack\tStats200"]=="0"&&br["ModPack\tTraitHover"]=="1"&&br["ModPack\tCollab"]=="1"&&br["ModPack\tField3D"]=="1"&&br["ModPack\tBattleDeaths"]=="0","bridge ini all-on (TraitHover coexist with 9/13)");
   Check(H(root,"SAN14Mods/ModPack/SAN14_TraitHover.dll")==M.Entries.First(f=>f.Target=="SAN14Mods/ModPack/SAN14_TraitHover.dll"&&f.When.Contains("|")).Sha,"14 with 9/13: coexist TraitHover installed");
   Check(H(root,"SAN14_Traits9.dll")!=""&&H(root,"SAN14_Traits9_UI.dll")!=""&&H(root,"SAN14_Traits9Check.dll")!=""&&H(root,"SAN14Mods/MeritTraits/UI_Traits9_Hover.dll")=="","9+13: shared Traits9 files installed once (R42 A10), no merit-folder copy");
   string normalRuntime=M.Entries.First(f=>f.Target.StartsWith("FieldLeader3D")&&f.When=="11&!12").Sha;
   Check(!M.Entries.Any(f=>f.Id==Spec.Fog||f.When.Contains("11&12")),"R41: manifest has no 12 (fog) entries");
   Check(H(root,"FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe")==normalRuntime&&H(root,"SAN14_FogTrial.dll")=="","R41: 11 uses normal runtime, no fog files");
   Check(File.Exists(Path.Combine(root,Engine.BgmIniRel)),"8: bgm ini created");
   var st=e.Status(e.Load());Check(st.Where((x,i)=>i!=Spec.Fog-1).All(x=>x.StartsWith("설치됨"))&&st[Spec.Fog-1]=="미설치"&&st[9]=="설치됨 (추가 모델 149개)","status all 설치됨(12 미설치): "+String.Join("|",st.Distinct().ToArray()));
   // R24 12.5번(내부 26) 기록 이어받기: 12번으로, R24 가 만든 SAN14_FogAI.ini(Weather=0) 는 그대로 유지
   {string sx=File.ReadAllText(Path.Combine(root,Engine.StateRel));string fogIni=Path.Combine(root,Engine.FogIniRel);
    var doc=XDocument.Parse(sx);foreach(var f in doc.Root.Element("Selected").Elements("Feature"))if((string)f.Attribute("id")=="12")f.Attribute("enabled").Value="false";
    doc.Root.Element("Selected").Add(new XElement("Feature",new XAttribute("id",26),new XAttribute("enabled",true)));doc.Root.SetAttributeValue("installer","R24");
    File.WriteAllText(fogIni,"Weather=0\r\n");string fh=Files.HashFile(fogIni);
    doc.Root.Element("Files").Add(new XElement("File",new XAttribute("path",Engine.FogIniRel),new XAttribute("before",""),new XAttribute("after",fh),new XAttribute("kind","owned")));
    File.WriteAllText(Path.Combine(root,Engine.StateRel),doc.ToString());
    var ls=Make(root).Load();Check(!ls.On[Spec.Fog-1]&&ls.RetiredOn,"R41: R24 12.5 record read as retired 12 (off)");
    string imsg="";try{Make(root).Install(All(),null);}catch(Exception ex){imsg=ex.Message;}Check(imsg=="","R41: install over R24 12.5 record ok "+imsg);
    Check(H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll")=="","R41: no fog engine after install");Console.WriteLine("   R24 FogAI.ini after: "+(File.Exists(fogIni)?"kept":"removed"));}
   // 사용자 조정: 게임 안에서 SkipReports 끔, 사용자 키 추가
   string txt=Engine.Text(File.ReadAllBytes(Path.Combine(root,Engine.SettingsRel)));var d=Engine.ParseIni(txt);d["Convenience\tSkipReports"]="0";d["Custom\tMine"]="7";File.WriteAllText(Path.Combine(root,Engine.SettingsRel),Engine.WriteIni(d,null));
   // 부분 변경: 12,13,24,18,9 끄기
   var on=All();on[11]=false;on[12]=false;on[23]=false;on[17]=false;on[8]=false;
   n=Make(root).Install(on,null);Check(n>0,"partial update ops="+n);
   Check(H(root,"SAN14_FogTrial.dll")==""&&H(root,"SAN14Mods/Fog121/SAN14_FogEngine121.dll")==""&&H(root,"SAN14_FogFieldPair_Entry.dll")=="","12 off: fog files removed");
   Check(H(root,"FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe")==normalRuntime,"11 without 12: normal runtime");
   Check(H(root,"SAN14_MeritTraits.dll")==""&&H(root,"SAN14Mods/MeritTraits/UI_Traits9_Hover.dll")=="","13 off: merit removed");
   Check(H(root,"SAN14_Traits9_UI.dll")==""&&H(root,"SAN14_Traits9.dll")==""&&H(root,"SAN14_Traits9Check.dll")=="","9·13 off: shared traits9 files removed");
   Check(H(root,Engine.FixRel)==M.Values["fix_original"]&&H(root,Engine.ExpRel)==M.Values["exp_original"],"24/18 off: data restored to 1.0.1 originals");
   Check(H(root,"SAN14Mods/DataToggle/fixdata_nostat.s14")=="","24 off: toggle copies removed");
   ini=Ini(root,Engine.SettingsRel);br=Ini(root,Engine.BridgeIniRel);
   Check(ini["Convenience\tSkipReports"]=="0"&&ini["Custom\tMine"]=="7","user-tuned settings kept on update");
   Check(ini["Modules\tTraits9Module"]=="0"&&br["ModPack\tCollab"]=="0"&&br["ModPack\tTraitHover"]=="1","9 off+13 off: Traits9Module 0, Collab 0, TraitHover now applied");
   Check(H(root,"SAN14Mods/ModPack/SAN14_TraitHover.dll")==M.Entries.First(f=>f.Target=="SAN14Mods/ModPack/SAN14_TraitHover.dll"&&f.When.Contains("!")).Sha,"14 alone: original TraitHover installed");
   Check(H(root,"SAN14_SettingsHelpScenic.dll")==M.Entries.First(f=>f.Target=="SAN14_SettingsHelpScenic.dll"&&f.When=="base&22").Sha,"scenic help domestic variant (22 on)");
   // 4번 끄고 켜기
   on[3]=false;Make(root).Install(on,null);ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tSkipReports"]=="0"&&ini["Camera\tAutoFocus"]=="1"&&ini["Convenience\tCityNoFade"]=="0","4 off: keys zeroed, AutoFocus 1");
   on[2]=false;Make(root).Install(on,null);ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tAudienceGate"]=="0"&&ini["Convenience\tAudienceButton"]=="0","3 off: AudienceGate=0, AudienceButton=0 (button hidden)");
   {var o=Ini(root,Engine.Patch27IniRel);Check(o["Features\tPatch27Features"]=="1"&&File.Exists(Path.Combine(root,"SAN14_Patch27.dll")),"Patch27 options: features on with 25");}
   on[2]=true;Make(root).Install(on,null);ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tAudienceGate"]=="2"&&ini["Convenience\tAudienceButton"]=="1","3 on again: AudienceGate=2, AudienceButton=1");
   {var on2=(bool[])on.Clone();on2[24]=false;Make(root).Install(on2,null);var o=Ini(root,Engine.Patch27IniRel);Check(o["Features\tPatch27Features"]=="0"&&File.Exists(Path.Combine(root,"SAN14_Patch27.dll")),"25 off, 1 on: Patch27 kept for settings pages, features off");
    var engApplied=Make(root);var ap=engApplied.Applied(engApplied.Load(),Path.Combine(root,"no_such_runtime.log"));Check(ap[0]=="게임 실행 전"&&ap[24]=="","applied column: before first run / off item blank");
    // R32: 13번 적용 판정 — 시작 기록만 있으면 '시작 실패', ACTIVE 면 개성 9개 화면 기록까지
    {on[12]=true;Make(root).Install(on,null);engApplied=Make(root);string rl=Path.Combine(root,"rt_test.log");System.Threading.Thread.Sleep(1100);
     File.WriteAllText(rl,"SAN14 ModCore 1.0 / x\r\nMeritTraits: prerequisite/hook verification failed; merit awards not enabled.\r\n");
     var a1=engApplied.Applied(engApplied.Load(),rl);Check(a1[12].StartsWith("적용 안 됨 – 13번 시작 실패(prerequisite"),"applied 13: start failure reason shown: "+a1[12]);
     File.WriteAllText(rl,"SAN14 ModCore 1.0 / x\r\nMeritTraits: existing Hover renderer fingerprint verified.\r\nSAN14CE.MeritTraits.HoverX RVA=00491f01 len=14\r\n");
     File.WriteAllText(Path.Combine(root,"SAN14_Traits9_UI.log"),"SAN14 Traits9 UI: SKIPPED - editor lifecycle hook conflict; no UI code changed.\r\n");
     var a2=engApplied.Applied(engApplied.Load(),rl);Check(a2[12].StartsWith("일부 적용 – 개성 9칸 화면 실패(editor lifecycle"),"applied 13: traits9 ui skip reason shown: "+a2[12]);
     File.WriteAllText(Path.Combine(root,"SAN14_Traits9_UI.log"),"SAN14 Traits9 UI v2.6: APPLIED - nine-trait UI.\r\n");
     var a3=engApplied.Applied(engApplied.Load(),rl);Check(a3[12]=="적용됨 (개성 9칸 화면 정상)","applied 13: active + traits9 ui applied: "+a3[12]);
     // R42d: 성공 기록이 APPLIED 가 아니어도 성공(실제 PC 에서 노란 표시 났던 경우), 덮어써진 실패는 적용확인.log R42T9 줄로 잡음
     string uiLog=Path.Combine(root,"SAN14_Traits9_UI.log"),apLog=Path.Combine(root,Engine.ApplyLogRel);Directory.CreateDirectory(Path.GetDirectoryName(apLog));
     foreach(string okLine in new[]{"SAN14 Traits9 UI: ALLSCREEN_GRID_CREATED - nine native labels created with original label height and row positions.","SAN14 Traits9 UI: TABLE9_CREATED - native trait columns 1 through 9 installed.","SAN14 Traits9 UI: GRID_CREATED - native 3x3 editor trait labels and tooltip entries created.","SAN14 Traits9 UI: TABLE9_COPIED - screen-specific columns preserved; trait columns 1 through 9 installed."}){
      File.WriteAllText(uiLog,okLine+"\r\n");var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12]=="적용됨 (개성 9칸 화면 정상)","applied 13: success line not APPLIED still ok: "+okLine.Substring(18,22)+" -> "+ax[12]);}
     File.WriteAllText(uiLog,"SAN14 Traits9 UI: APPLIED_WITH_WARNING - cache flush or protection restore failed.\r\n");{var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12]=="적용됨 (개성 9칸 화면 정상, 경고: cache flush or protection restore failed.)","applied 13: warning shown: "+ax[12]);}
     string now=DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),old=DateTime.Now.AddMinutes(-30).ToString("yyyy-MM-dd HH:mm:ss");
     File.WriteAllText(apLog,"R42T9\tC\t"+old+"\tSAN14 Traits9 UI: ALLSCREEN_SKIPPED - native label creation or measured geometry failed.\r\nR39\t9\tSAN14_Traits9Check.dll\t"+now+"\tOK\t6/6\tNotice\tnine slots + 3x3 UI\r\nR42T9\tD\t"+now+"\twhy=G kind=0 parent=CUtilPInfoLayout boxes=(635,58 158x40) relaxed=ok(+)\r\n",new UTF8Encoding(false));
     File.WriteAllText(uiLog,"SAN14 Traits9 UI: ALLSCREEN_GRID_CREATED - nine native labels created with original label height and row positions.\r\n");
     {var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12]=="적용됨 (개성 9칸 화면 정상, 일부 화면 칸 간격 보정)","applied 13: relaxed ok + old failure before this run ignored: "+ax[12]);}
     File.AppendAllText(apLog,"R42T9\tC\t"+now+"\tSAN14 Traits9 UI: ALLSCREEN_SKIPPED - unknown layout or panel capacity. | why=P kind=1\r\n");
     {var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12]=="일부 적용 – 개성 9칸 화면 실패(unknown layout or panel capacity.)","applied 13: failure overwritten in UI log still caught from apply log: "+ax[12]);
      var dx=engApplied.AppliedDetail(engApplied.Load(),rl);Check(dx[12].State=="PART","applied 13 detail: PART: "+dx[12].State);}
     File.WriteAllText(apLog,"R39\t9\tSAN14_Traits9Check.dll\t"+now+"\tOK\t6/6\tNotice\tnine slots + 3x3 UI\r\nR42T9\tD\t"+now+"\twhy=P kind=0 parent=? boxes=(1,1 1x1) relaxed=fail(P)\r\n",new UTF8Encoding(false));
     {var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12].StartsWith("일부 적용 – 개성 9칸 화면 실패(화면 칸 모양을 못 맞춤(why=P"),"applied 13: relaxed fail: "+ax[12]);}
     File.Delete(uiLog);File.WriteAllText(apLog,"R39\t9\tSAN14_Traits9Check.dll\t"+now+"\tOK\t6/6\tNotice\tnine slots + 3x3 UI\r\n",new UTF8Encoding(false));
     {var ax=engApplied.Applied(engApplied.Load(),rl);Check(ax[12]=="적용됨 (개성 9칸 화면은 아직 안 열어 봄)","applied 13: no screen yet: "+ax[12]);}
     File.Delete(apLog);
     File.Delete(rl);File.Delete(Path.Combine(root,"SAN14_Traits9_UI.log"));on[12]=false;Make(root).Install(on,null);}
    Make(root).Install(on,null);}
   on[3]=true;Make(root).Install(on,null);ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tSkipReports"]=="1"&&ini["Camera\tAutoFocus"]=="0","4 on again: defaults restored");
   // 24 on, 게임 안에서 OFF(원본으로) → 재설치 시 존중
   on[23]=true;Make(root).Install(on,null);Check(H(root,Engine.FixRel)==M.Values["fix_nostat"],"24 on again: nostat");
   Put(root,Engine.FixRel,File.ReadAllBytes(fixOrig));Make(root).Install(on,null);Check(H(root,Engine.FixRel)==M.Values["fix_original"],"24: in-game OFF respected on reinstall");
   st=Make(root).Status(Make(root).Load());Check(st[23]=="설치됨(게임 안에서 OFF)","status 24 in-game off: "+st[23]);
   // 전부 제거 → 처음과 같은 파일 집합
   var e2=Make(root);e2.Install(new bool[Spec.Count],null);var after=Snapshot(root);
   var extra=after.Keys.Except(before.Keys).ToList();var missing=before.Keys.Except(after.Keys).ToList();var changed=before.Keys.Intersect(after.Keys).Where(k=>before[k]!=after[k]).ToList();
   Check(extra.Count==0&&missing.Count==0&&changed.Count==0,"remove all restores original set (extra="+String.Join(",",extra.ToArray())+" missing="+String.Join(",",missing.ToArray())+" changed="+String.Join(",",changed.ToArray())+")");
   Check(!File.Exists(Path.Combine(root,Engine.StateRel)),"state removed");
  }
  // 2. 충돌 보존: 다른 파일이 자리에 있음 → 아무것도 바꾸지 않음
  {
   string root=NewGame("conflict");Put(root,"SAN14_Patch27.dll",Encoding.ASCII.GetBytes("someone else"));var before=Snapshot(root);string msg;
   Check(Throws(delegate{Make(root).Install(All(),null);},"25번",out msg),"conflict reported for 25번: "+msg.Replace("\r\n"," / "));
   var after=Snapshot(root);Check(before.Count==after.Count&&before.All(kv=>after[kv.Key]==kv.Value),"conflict: nothing changed");
   File.Delete(Path.Combine(root,"SAN14_Patch27.dll"));
   Put(root,Engine.FixRel,Encoding.ASCII.GetBytes("broken data"));Check(Throws(delegate{Make(root).Install(All(),null);},"24번",out msg),"unknown fixdata reported: "+msg.Replace("\r\n"," / "));
   Put(root,Engine.FixRel,File.ReadAllBytes(fixOrig));
   Check(Throws(delegate{Make(root).Install(Only(20),null);},"",out msg)==false,"20 alone installs");
   Check(H(root,"SAN14Mods/Core/SAN14_ModCore.dll")==""&&!File.Exists(Path.Combine(root,Engine.SettingsRel)),"20 alone: no runtime installed");
   Put(root,Engine.FontRel,Encoding.ASCII.GetBytes("bgm changed rdb"));Check(Throws(delegate{Make(root).Install(Only(20,1),null);},"20번",out msg),"font changed by other tool → reported: "+msg.Replace("\r\n"," / "));
   // 구버전 EXE
   string root2=NewGame("oldexe");File.WriteAllBytes(Path.Combine(root2,"SAN14CE_KO.exe"),new byte[]{1,2,3});Check(Throws(delegate{Make(root2).Install(All(),null);},"확인할 수 없습니다",out msg),"unknown exe refused");
  }
  // 4. R35 10번 추가 3D 모델 (색인 합본·새 컨테이너·목록·무장 연결)
  {
   string root=NewGame("models");string ldir=Engine.LinksDir(root);Directory.CreateDirectory(ldir);string lpath=Path.Combine(ldir,"OfficerModels.tsv");
   var bom=new UTF8Encoding(true);File.WriteAllText(lpath,"SAN14_OFFICER_MODELS_V1\n5\t3\t테스트\n440\t5\t제갈량\n",bom);
   var logs=new List<string>();Func<Engine> ME=delegate{var x=Make(root);x.Log=delegate(string t){logs.Add(t);Console.WriteLine("   log: "+t);};return x;};
   var sel=Only(1,10);ME().Install(sel,null);
   foreach(string r in Engine.ModelIndexRels)Check(H(root,r)==ShaOf(M,r,sel)&&H(root,r)!=M.Values["model_orig_"+Path.GetFileName(r)],"models: merged index "+r);
   int nf=0;foreach(var f in M.Entries)if(f.Id==10&&f.Target.StartsWith("0002/")){nf++;Check(H(root,f.Target)==f.Sha,"models: file "+f.Target);}
   Check(nf>=12&&H(root,Engine.ModelCatalogRel)==ShaOf(M,Engine.ModelCatalogRel,sel),"models: "+nf+" files + catalog");
   string lt=Engine.Text(File.ReadAllBytes(lpath));
   Check(lt.StartsWith("SAN14_OFFICER_MODELS_V1\n")&&lt.Contains("\n5\t3\t테스트\n")&&lt.Contains("\n440\t73\t제갈량\n")&&lt.Contains("\n1\t-1\t")&&lt.Split('\n').Count(x=>x.Length>0)==1+115+1,"models: links merged (user row kept, old package wins, 115 rows)");
   Check(File.ReadAllBytes(lpath)[0]==0xEF&&File.Exists(lpath+".R35설치전.bak")&&File.Exists(Path.Combine(ldir,Engine.LinksMarker)),"models: links BOM + backup + marker");
   var st=Make(root).Status(Make(root).Load());Check(st[9]=="설치됨 (추가 모델 149개)","models: status "+st[9]);
   File.WriteAllText(lpath,lt.Replace("\n440\t73\t","\n440\t10\t"),bom);string lh=Files.HashFile(lpath);
   ME().Install(sel,null);Check(Files.HashFile(lpath)==lh,"models: in-game link edits kept on reinstall");
   {var ea=Make(root);string rl=Path.Combine(root,"rt_models.log");System.Threading.Thread.Sleep(1100);File.WriteAllText(rl,"SAN14 ModCore 1.0 / x\r\n");
    File.WriteAllBytes(Path.Combine(root,"SAN14CE_11Patch.log"),Encoding.Unicode.GetBytes("지원 실행 파일 확인 완료\r\n고유무장 등록: 원본 ID 0~24, 확장 ID 25~173 예약. 개인 모델/자동 시험 연결 미포함.\r\n"));
    var ap=ea.Applied(ea.Load(),rl);Check(ap[9]=="적용됨 (모델 0~173)","models: applied "+ap[9]);File.Delete(rl);File.Delete(Path.Combine(root,"SAN14CE_11Patch.log"));}
   // 10번 끄기 → 색인 1.0.1 원본, 새 파일 삭제, 연결 파일은 사용자 자료라 유지
   ME().Install(Only(1),null);
   foreach(string r in Engine.ModelIndexRels)Check(H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)],"models off: original index "+r);
   Check(M.Entries.Where(f=>f.Id==10&&f.Target.Contains(".rdb.bin")).All(f=>H(root,f.Target)=="")&&H(root,Engine.ModelCatalogRel)=="","models off: containers + catalog removed");
   Check(Files.HashFile(lpath)==lh,"models off: links file kept");
   // 다른 모드가 색인을 바꿈 → 추가 모델만 건너뛰고 나머지 설치, 그 색인 보존
   string ceo=Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"model101"),"CharacterEditor.rdb");
   foreach(string bad in new[]{"0002/CharacterEditor.rdb","0002/KIDSSystemResource.rdb"}){
    byte[] keep=File.ReadAllBytes(Path.Combine(root,bad.Replace('/',Path.DirectorySeparatorChar)));Put(root,bad,Encoding.ASCII.GetBytes("other mod index"));string oh=H(root,bad);
    logs.Clear();string msg="";try{ME().Install(sel,null);}catch(Exception ex){msg=ex.Message;}
    Check(msg==""&&H(root,bad)==oh&&H(root,Engine.ModelCatalogRel)==""&&Engine.ModelIndexRels.Where(r=>r!=bad).All(r=>H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)])&&H(root,"SAN14_IntegratedConvenience.dll")!="","models: foreign "+bad+" → model data skipped, rest installed "+msg);
    Check(logs.Any(x=>x.Contains("추가 모델은 설치하지 않았습니다")&&x.Contains(bad)),"models: skip reason logged");
    st=Make(root).Status(Make(root).Load());Check(st[9]=="설치됨 (추가 모델 없음)","models: status without models "+st[9]);
    Put(root,bad,keep);
   }
   // R37 레코드 단위: 다른 도구가 KIDS 의 다른 레코드를 고친 상태에서 설치·재설치·제거
   {string K="0002/KIDSSystemResource.rdb";string kp=Path.Combine(root,K.Replace('/',Path.DirectorySeparatorChar));
    byte[] origK=File.ReadAllBytes(Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"model101"),"KIDSSystemResource.rdb"));
    byte[] other=Tweak(origK,0x811f81af);Put(root,K,other);   // 맵 확장처럼 다른 레코드만 바뀐 KIDS(우리 레코드 없음)
    logs.Clear();ME().Install(sel,null);byte[] k1=File.ReadAllBytes(kp);
    Check(H(root,Engine.ModelCatalogRel)!=""&&!k1.SequenceEqual(other)&&Tweak(k1,0x811f81af).Length>0,"records: other tool's KIDS → ours added, install ok");
    Check(k1.SequenceEqual(Tweak(File.ReadAllBytes(Path.Combine(Path.Combine(Path.Combine(Path.Combine(Path.Combine(pkg,"support"),"payload"),"f10"),"0002"),"KIDSSystemResource.rdb")),0x811f81af)),"records: result = ours + other tool's change kept");
    Put(root,K,other);logs.Clear();ME().Install(sel,null);Check(File.ReadAllBytes(kp).SequenceEqual(k1),"records: other tool reverted KIDS → reinstall puts ours back, keeps its change");
    ME().Install(Only(1),null);Check(File.ReadAllBytes(kp).SequenceEqual(other)&&Engine.ModelIndexRels.Where(r=>r!=K).All(r=>H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)]),"records: 10 off → only ours removed, other tool's change kept");
    Put(root,K,origK);ME().Install(sel,null);Put(root,K,origK);   // R35 상황: KIDS 만 원본으로 되돌려진 채 전부 제거
    string m4="";try{ME().Install(new bool[Spec.Count],null);}catch(Exception ex){m4=ex.Message;}
    Check(m4==""&&Engine.ModelIndexRels.All(r=>H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)])&&H(root,"0002/CharacterEditor.rdb.bin3369")==""&&H(root,"SAN14_IntegratedConvenience.dll")=="","records: KIDS reverted by other tool, remove all → ok, originals "+m4);
    ME().Install(sel,null);Put(root,K,Tweak(File.ReadAllBytes(kp),0xb290631c));string ch2=H(root,K);   // 우리 레코드를 남이 바꿈
    logs.Clear();ME().Install(sel,null);Check(H(root,K)==ch2&&logs.Any(x=>x.Contains("같은 레코드를 다른 모드가")),"records: our record changed by other → skip, kept");
    m4="";try{ME().Install(Only(1),null);}catch(Exception ex){m4=ex.Message;}
    Check(m4==""&&H(root,K)==ch2&&H(root,"0002/CharacterEditor.rdb")==M.Values["model_orig_CharacterEditor.rdb"],"records: removal leaves foreign KIDS, cleans the rest "+m4);
    Put(root,K,origK);}
   // 이 패치가 아닌 같은 이름 컨테이너가 자리에 있음 → 건너뜀, 그 파일 보존
   Put(root,"0002/MaterialEditor.rdb.bin3369",Encoding.ASCII.GetBytes("old package container"));string ch=H(root,"0002/MaterialEditor.rdb.bin3369");
   logs.Clear();ME().Install(sel,null);Check(H(root,"0002/MaterialEditor.rdb.bin3369")==ch&&H(root,"0002/MaterialEditor.rdb")==M.Values["model_orig_MaterialEditor.rdb"]&&logs.Any(x=>x.Contains("자리에 이 패치가 아닌 파일")),"models: foreign container kept, model data skipped");
   File.Delete(Path.Combine(root,"0002/MaterialEditor.rdb.bin3369".Replace('/',Path.DirectorySeparatorChar)));
   // 추가 모델 자료 ZIP 빠짐: 새로 깔 때는 건너뜀, 이미 깔린 것은 그대로
   ME().Install(sel,null);Check(H(root,Engine.ModelCatalogRel)!="","models: reinstalled");
   string src=Path.Combine(pkg,"support/payload/f10/0002/MaterialEditor.rdb.bin3374".Replace('/',Path.DirectorySeparatorChar)),hold=src+".hold";File.Move(src,hold);
   try{
    Check(Make(root).MissingPayloads(sel).Count==0&&Make(root).MissingModelPayloads().Count==1,"models zip missing: not blocking, reported");
    var snap=Snapshot(root);logs.Clear();ME().Install(sel,null);var snap2=Snapshot(root);
    Check(snap.Count==snap2.Count&&snap.All(kv=>snap2.ContainsKey(kv.Key)&&snap2[kv.Key]==kv.Value)&&logs.Any(x=>x.Contains("그대로 둡니다")),"models zip missing on update: installed models kept");
    string r3=NewGame("models_nozip");var e3=Make(r3);var l3=new List<string>();e3.Log=delegate(string t){l3.Add(t);};string m3="";try{e3.Install(sel,null);}catch(Exception ex){m3=ex.Message;}
    Check(m3==""&&H(r3,Engine.ModelCatalogRel)==""&&H(r3,"0002/CharacterEditor.rdb")==M.Values["model_orig_CharacterEditor.rdb"]&&l3.Any(x=>x.Contains("자료 ZIP 이 없음")),"models zip missing on fresh install: skipped, rest installed "+m3);
    Directory.Delete(r3,true);
   }finally{File.Move(hold,src);}
   // 전부 제거
   ME().Install(new bool[Spec.Count],null);
   Check(Engine.ModelIndexRels.All(r=>H(root,r)==M.Values["model_orig_"+Path.GetFileName(r)])&&M.Entries.Where(f=>f.Id==10).All(f=>Engine.ModelIndexRels.Contains(f.Target)||H(root,f.Target)==""),"models: remove all → originals, no model files");
   Directory.Delete(root,true);
  }
  // 5. R39: 선택 설치/제거 집합, 하나씩 설치·제거, 충돌 규칙, 31번 화면 색인, 적용확인 기록, 기록 폴더, 설정 창 키
  {
   // 5a. 선택 집합 계산
   var cur=Only(1,2,13);var sel=Only(2,27);
   var w1=Engine.Compose(cur,sel,true);Check(w1[0]&&w1[1]&&w1[12]&&w1[26]&&!w1[11],"compose install = current ∪ selected");
   var w2=Engine.Compose(cur,sel,false);Check(w2[0]&&!w2[1]&&w2[12]&&!w2[26],"compose remove = current − selected");
   // 5b. 하나씩만 설치 → 제거 → 처음과 같음
   string root=NewGame("each");var before=Snapshot(root);int badIds=0;
   for(int id=1;id<=Spec.Count;id++){if(Spec.Retired(id))continue;
    string msg="";try{Make(root).Install(Engine.Compose(new bool[Spec.Count],Only(id),true),null);}catch(Exception ex){msg=ex.Message;}
    var st=Make(root).Status(Make(root).Load());
    if(msg!=""||!st[id-1].StartsWith("설치됨")){badIds++;Console.WriteLine("   each "+id+": "+st[id-1]+" "+msg.Replace("\r\n"," / "));}
    try{Make(root).Install(Engine.Compose(Make(root).Load().On,Only(id),false),null);}catch(Exception ex){badIds++;Console.WriteLine("   each remove "+id+": "+ex.Message.Replace("\r\n"," / "));}
    var after=Snapshot(root);if(after.Count!=before.Count||before.Any(kv=>!after.ContainsKey(kv.Key)||after[kv.Key]!=kv.Value)){badIds++;Console.WriteLine("   each "+id+": not restored: "+String.Join(",",after.Keys.Except(before.Keys).Concat(before.Keys.Where(k=>after.ContainsKey(k)&&after[k]!=before[k])).ToArray()));}
   }
   Check(badIds==0,"each number alone: install → 설치됨 → remove → original set (bad="+badIds+")");
   Check(!File.Exists(Path.Combine(root,Engine.StateRel)),"each: state removed at end");
   // 5b2. 게임 안 설정 창이 모듈 설정 파일을 바꾼 뒤에도 다시 설치·제거가 막히지 않음
   {string rr=NewGame("runtimeini");var b0=Snapshot(rr);string m1="",m2="",m3="";
    try{Make(rr).Install(Engine.Compose(new bool[Spec.Count],Only(32),true),null);}catch(Exception ex){m1=ex.Message;}
    string hp=Path.Combine(rr,"SAN14Mods/modules/HiredFace.ini");bool had=File.Exists(hp);if(had)File.AppendAllText(hp,"; changed in game\r\n[HiredFace]\r\nEnabled=0\r\n");
    string kp=Path.Combine(rr,"SAN14_WheelRegion.ini");
    try{Make(rr).Install(Engine.Compose(Make(rr).Load().On,Only(30),true),null);}catch(Exception ex){m2=ex.Message;}
    if(File.Exists(kp))File.AppendAllText(kp,"; changed\r\n");
    try{Make(rr).Install(Engine.Compose(Make(rr).Load().On,Only(32),true),null);}catch(Exception ex){m2+=ex.Message;}
    bool kept=had&&File.ReadAllText(hp).Contains("changed in game");
    try{Make(rr).Install(Engine.Compose(Make(rr).Load().On,All(),false),null);}catch(Exception ex){m3=ex.Message;}
    Check(m1==""&&m2==""&&kept,"runtime-changed module ini: reinstall ok, user value kept "+m1+m2);
    Check(m3==""&&!File.Exists(hp)&&!File.Exists(kp),"runtime-changed module ini: remove ok, files gone "+m3);}
   // 5b3. 31번 제거 때 합본 색인 자료(뒤쪽 ZIP)가 없으면 아무것도 안 바꾸고 멈춤(색인 남기고 컨테이너만 지우는 일 방지)
   {string rr=NewGame("rm31nozip");string m1="",m2="";try{Make(rr).Install(Engine.Compose(new bool[Spec.Count],Only(31),true),null);}catch(Exception ex){m1=ex.Message;}
    var snap=Snapshot(rr);string src=M.Entries.First(f=>f.Id==31&&f.Target=="0002/ScreenLayout.rdb").Source;string sp=Path.Combine(pkg,src.Replace('/',Path.DirectorySeparatorChar));File.Move(sp,sp+".hide");
    try{try{Make(rr).Install(Engine.Compose(Make(rr).Load().On,Only(31),false),null);}catch(Exception ex){m2=ex.Message;}}finally{File.Move(sp+".hide",sp);}
    var snap2=Snapshot(rr);bool same=snap.Count==snap2.Count&&snap.All(kv=>snap2.ContainsKey(kv.Key)&&snap2[kv.Key]==kv.Value);
    Check(m1==""&&m2.Contains("31번")&&same,"31 remove without merged index payload: stopped, game untouched ("+m2.Split('\r')[0]+")");
    try{Make(rr).Install(Engine.Compose(Make(rr).Load().On,Only(31),false),null);m2="";}catch(Exception ex){m2=ex.Message;}
    Check(m2==""&&H(rr,Engine.ScreenIndexRel)==M.Values["screen_orig_ScreenLayout.rdb"]&&H(rr,"0002/ScreenLayout.rdb.bin3375")=="","31 remove with payload back: index original, container gone "+m2);}
   // 5b4. 30번 키트를 따로 깔아 둔 사용자: 30번 설치 → 제거해도 그 키트 파일은 남음
   {string rr=NewGame("kit30");foreach(var f in M.Entries.Where(f=>f.Id==30&&f.Target.EndsWith(".dll")))Put(rr,f.Target,File.ReadAllBytes(Path.Combine(pkg,f.Source.Replace('/',Path.DirectorySeparatorChar))));
    var b0=Snapshot(rr);string m="";try{Make(rr).Install(Engine.Compose(new bool[Spec.Count],Only(30),true),null);Make(rr).Install(Engine.Compose(Make(rr).Load().On,Only(30),false),null);}catch(Exception ex){m=ex.Message;}
    bool kept=M.Entries.Where(f=>f.Id==30&&f.Target.EndsWith(".dll")).All(f=>File.Exists(Path.Combine(rr,f.Target.Replace('/',Path.DirectorySeparatorChar))));
    Check(m==""&&kept,"standalone kit 30 files kept after 30 install/remove "+m);}
   // 5c. 충돌 규칙
   {var all=All();var e=Make(root);Spec.ForeignFont=delegate(string r){return e.IsForeignFont();};
    var hits=Spec.Conflicts.Where(c=>c.Check(all,root)!=null).Select(c=>c.Key).ToList();
    Check(hits.Contains("13-2")&&!hits.Contains("12-27")&&!hits.Contains("9-13")&&hits.Contains("22-28")&&!hits.Contains("11-2D")&&!hits.Contains("20-font"),"conflicts on all: "+String.Join(",",hits.ToArray()));
    all[26]=false;all[12]=false;Check(Spec.Conflicts.Where(c=>c.Kind==Spec.ConflictKind.Block).All(c=>c.Check(all,root)==null),"conflicts resolved after dropping 27 and 13");
    // 설치기 창에서 고르는 답 그대로: 13-2 "2번은 자동 논공만 끄고 둘 다 설치"(0) → 13·2 둘 다 남고 더 막는 것 없음 → 실제 설치 → AutoCredit=0, 2번 나머지 켬
    {var w=All();var res=new HashSet<string>();Check(Spec.ApplyChoice("13-2",0,w,res),"choices 13-2=both accepted");
     var left=Spec.RemainingBlock(w,root,res);Check(left==null&&w[12]&&w[1]&&w[26],"after choices: no remaining block, 13 and 2 kept ("+(left==null?"":left.Key)+")");
     var wq=All();var rq=new HashSet<string>();Spec.ApplyChoice("13-2",1,wq,rq);Check(!wq[12]&&wq[1]&&Spec.RemainingBlock(wq,root,rq)==null,"13-2=drop13: 13 off, nothing else blocks (R41: no 12-27)");
     Check(!Spec.ApplyChoice("13-2",2,All(),new HashSet<string>())&&!Spec.ApplyChoice("13-2",-1,All(),new HashSet<string>()),"13-2 cancel/close = stop");
     {var c=Spec.Conflicts.First(x=>x.Key=="13-2");var bf=new bool[Spec.Count];bf[1]=bf[12]=true;var af=(bool[])bf.Clone();af[28]=true;
      Check(!c.Relevant(bf,af),"13-2: 2 and 13 already installed, installing 29 -> no prompt");
      var b2=new bool[Spec.Count];b2[1]=true;var a2=(bool[])b2.Clone();a2[12]=true;Check(c.Relevant(b2,a2),"13-2: adding 13 while 2 installed -> prompt");
      var b3=new bool[Spec.Count];b3[12]=true;var a3=(bool[])b3.Clone();a3[1]=true;Check(c.Relevant(b3,a3),"13-2: adding 2 while 13 installed -> prompt");
      Check(Spec.RemainingBlock(af,root,new HashSet<string>(),bf)==null||Spec.RemainingBlock(af,root,new HashSet<string>(),bf).Key!="13-2","13-2 not a remaining block when both were already installed");}
     string rb=NewGame("both13_2");string mb="";try{Make(rb).Install(Only(2,13),null);}catch(Exception ex){mb=ex.Message;}var ini=Ini(rb,Engine.SettingsRel);var st=Make(rb).Status(Make(rb).Load());
     Check(mb==""&&ini["Convenience\tAutoCredit"]=="0"&&st[1].StartsWith("설치됨")&&st[12].StartsWith("설치됨"),"13+2 both installed: AutoCredit=0, 2 and 13 installed "+mb+" "+st[1]+" / "+st[12]);
     var keys=new[]{"AutoPersonnelModule","AutoPersonnel","AutoScenery"};Check(keys.Where(k=>ini.ContainsKey("Convenience\t"+k)).All(k=>ini["Convenience\t"+k]!="0"),"13+2: 2's other automation not forced off ("+String.Join(",",keys.Where(k=>ini.ContainsKey("Convenience\t"+k)).Select(k=>k+"="+ini["Convenience\t"+k]).ToArray())+")");}
    Put(root,"d3d11.dll",Encoding.ASCII.GetBytes("x San14_2D x"));Check(Spec.Conflicts.First(c=>c.Key=="11-2D").Check(All(),root)!=null,"conflict 11-2D when d3d11.dll present");File.Delete(Path.Combine(root,"d3d11.dll"));
    Check(Spec.Conflicts.First(c=>c.Key=="11-2D").Check(All(),root)==null,"no 11-2D without d3d11.dll");
    {var c2=Spec.Conflicts.First(c=>c.Key=="11-2D");Put(root,"d3d11.dll",Encoding.ASCII.GetBytes("season"));Put(root,"SeasonSync.dll",Encoding.ASCII.GetBytes("season"));
     Check(c2.Check(All(),root)==null,"season map only (d3d11.dll + SeasonSync.dll): no 11-2D");
     Put(root,"d3d11_chain.dll",Encoding.Unicode.GetBytes("San14_2D"));Check(c2.Check(All(),root)!=null,"season map + 2D (d3d11_chain.dll): 11-2D");
     foreach(var f in new[]{"d3d11.dll","SeasonSync.dll","d3d11_chain.dll"})File.Delete(Path.Combine(root,f));
     Put(root,"d3d11.dll",Encoding.ASCII.GetBytes("ReShade"));Check(c2.Check(All(),root)==null,"other d3d11.dll (ReShade etc): no 11-2D");File.Delete(Path.Combine(root,"d3d11.dll"));
     Put(root,"San14_2D/San14Officers.bin",Encoding.ASCII.GetBytes("2d"));Check(c2.Check(All(),root)!=null,"2D data folder: 11-2D");Directory.Delete(Path.Combine(root,"San14_2D"),true);}
    Put(root,Engine.FontRel,Encoding.ASCII.GetBytes("other font patch"));string fh=H(root,Engine.FontRel);
    Check(Spec.Conflicts.First(c=>c.Key=="20-font").Check(All(),root)!=null,"conflict 20-font when foreign font");
    string m="";try{Make(root).Install(Only(20),null);}catch(Exception ex){m=ex.Message;}Check(m.Contains("20번"),"20 over foreign font without force → refused");
    var ef=Make(root);ef.ForceFont=true;m="";try{ef.Install(Only(20),null);}catch(Exception ex){m=ex.Message;}
    Check(m==""&&H(root,Engine.FontRel)==M.Values["font_rdb"],"20 with ForceFont → installed over foreign font "+m);
    Make(root).Install(new bool[Spec.Count],null);Check(H(root,Engine.FontRel)==fh,"remove after ForceFont → foreign font restored");
    Put(root,Engine.FontRel,File.ReadAllBytes(fontOrig));Spec.ForeignFont=null;
    Check(Spec.RemovalNote(Only(2,13),Only(13)).Contains("13번을 빼면")&&!Spec.RemovalNote(Only(12,27),Only(12)).Contains("27번"),"removal notes");
   }
   // 5d. 31번 화면 색인: 레코드 9개 교체 + 새 컨테이너, 런타임 없이 단독 설치, 제거 원복, 다른 도구와 공존
   {var s31=Only(31);string S=Engine.ScreenIndexRel;byte[] origS=File.ReadAllBytes(Path.Combine(Path.Combine(Path.Combine(pkg,"data"),"screen101"),"ScreenLayout.rdb"));
    Make(root).Install(s31,null);
    Check(H(root,S)==ShaOf(M,S,s31)&&H(root,S)!=M.Values["screen_orig_ScreenLayout.rdb"]&&H(root,"0002/ScreenLayout.rdb.bin3375")==ShaOf(M,"0002/ScreenLayout.rdb.bin3375",s31),"31: merged index + container installed");
    Check(H(root,"SAN14Mods/Core/SAN14_ModCore.dll")==""&&H(root,"dxgi.dll")=="","31 alone: no runtime needed");
    var st31=Make(root).Status(Make(root).Load());Check(st31[30]=="설치됨","31 status: "+st31[30]);
    var ap31=Make(root).AppliedDetail(Make(root).Load(),Path.Combine(root,"none.log"));Check(ap31[30].State=="FILE","31 applied = file "+ap31[30].Cell);
    Make(root).Install(new bool[Spec.Count],null);Check(File.ReadAllBytes(Path.Combine(root,S.Replace('/',Path.DirectorySeparatorChar))).SequenceEqual(origS)&&H(root,"0002/ScreenLayout.rdb.bin3375")=="","31 off: original index, container gone");
    byte[] other=Tweak(origS,0x01655802);Put(root,S,other);Make(root).Install(s31,null);byte[] k1=File.ReadAllBytes(Path.Combine(root,S.Replace('/',Path.DirectorySeparatorChar)));
    Check(!k1.SequenceEqual(other)&&k1.SequenceEqual(Tweak(File.ReadAllBytes(Path.Combine(pkg,ShaSource(M,S,s31).Replace('/',Path.DirectorySeparatorChar))),0x01655802)),"31: other tool's change kept, ours added");
    Make(root).Install(new bool[Spec.Count],null);Check(File.ReadAllBytes(Path.Combine(root,S.Replace('/',Path.DirectorySeparatorChar))).SequenceEqual(other),"31 off: only ours removed");
    Put(root,S,Tweak(origS,0x06f642d8));string ch=H(root,S);var logs=new List<string>();var e2=Make(root);e2.Log=delegate(string t){logs.Add(t);};string m2="";try{e2.Install(s31,null);}catch(Exception ex){m2=ex.Message;}
    Check(m2==""&&H(root,S)==ch&&e2.ScreenSkip!=""&&logs.Any(x=>x.Contains("31번")),"31: our record changed by other → skipped, kept "+m2);
    Check(Make(root).Status(Make(root).Load())[30]=="설치됨 (그래픽 건너뜀)","31 status skipped: "+Make(root).Status(Make(root).Load())[30]);
    Make(root).Install(new bool[Spec.Count],null);Put(root,S,origS);}
   // 5e. 적용확인.log 파서와 상세
   {var sel2=Only(1,22,30,32);Make(root).Install(sel2,null);System.Threading.Thread.Sleep(1100);
    string ap=Path.Combine(root,Engine.ApplyLogRel.Replace('/',Path.DirectorySeparatorChar));Directory.CreateDirectory(Path.GetDirectoryName(ap));
    File.WriteAllText(ap,"R39\t30\tSAN14_ExtBridge.dll\t2026-10-24 14:05:01\tOK\t21/21\t메뉴=닫기+열기;휠클릭=ON\t-\nR39C\t30\tSAN14_ExtBridge.dll\t휠클릭\t12\t2026-10-24 14:30:00\nR39\t22\tSAN14_DomesticCE.dll\t2026-10-24 14:05:02\tPART\t5/6\t자동내정=ON\t모든 내정 자동:OK;AI 명령서:OK;최소 인구:OK;저돌 수정:FAIL:고칠 자리(0x1A4B89) 바이트가 다름\nR39\t32\tSAN14_HiredFace.dll\t2026-10-24 14:05:03\tFAIL\t0/4\t-\t자리 4곳 중 1곳 바이트 다름\n",new UTF8Encoding(false));
    var d=Make(root).AppliedDetail(Make(root).Load(),Path.Combine(root,"none.log"));
    Check(d[29].State=="OK"&&d[29].Cell.StartsWith("적용 확인 ✔")&&d[29].Cell.Contains("휠클릭 12회"),"applied 30 OK + counter: "+d[29].Cell);
    Check(d[21].State=="PART"&&d[21].Cell=="일부 적용 ⚠ 3/4 — 저돌 수정 ✖"&&d[21].Detail().Contains("✖ 저돌 수정 — 고칠 자리"),"applied 22 PART cell/detail: "+d[21].Cell);
    Check(d[31].State=="FAIL"&&d[31].Cell.StartsWith("실패 ✖ — 자리 4곳"),"applied 32 FAIL: "+d[31].Cell);
    Check(d[0].State=="PRE"&&d[0].Cell=="게임 실행 전","applied 1 (no runtime.log) = 게임 실행 전: "+d[0].Cell);
    Check(d[1].State==""&&d[1].Cell=="","applied: not-installed number blank");
    // R43: 실패 보고(R43F)·종합 진단(R43D)·안내창(R43E) 줄
    {string now=DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
     File.AppendAllText(ap,"R43F\t32\tSAN14_HiredFace.dll\t"+now+"\thook\thired-officer face sites @ exe+0x32f196 (bytes differ) planned=0\t66 89 47 10 48 8d 4c 24 20\te9 11 22 33 44 8d 4c 24 20\t고용 무장 원본 얼굴(32번)이 적용 안 됨\tR43F-32-hook\tstamp=0x6a983c25 size=0x29ff000 (=R43 base)\n"
      +"R39\t30\tSAN14_ExtBridge.dll\t"+now+"\tOK\t21/21\t메뉴=닫기+열기;휠클릭=ON\t-\n"
      +"R43F\t30\tSAN14_ItemPicture.dll\t"+now+"\thook\titem select list fn entry exe+0x6839e0 @ exe+0x6839e0 (bytes differ) planned=0\t40 57 48 81 ec 80 04 00 00\te9 00 00 00 00 81 ec 80 04\t보물 그림이 안 나옴\tR43F-30-hook\tstamp=0x6a983c25 size=0x29ff000 (=R43 base)\n"
      +"R43D\t0\tSAN14_Diag.dll\t"+now+"\tcheck90\tregistered=9 loaded=8 started=6 problems=2\t4번 연출 생략: DLL 이 프로세스에 로드되지 않음 [근거: x]\t22번 내정 자동화(DomesticCE): 읽기 전용 모드 [근거: READ-ONLY]\n");
     var f=Engine.ParseFails(File.ReadAllText(ap),DateTime.Now.AddMinutes(-5));
     Check(f.ContainsKey(32)&&f[32].Count==1&&f[32][0][2]=="hook"&&f[32][0][7]=="R43F-32-hook"&&f[32][0][4].StartsWith("66 89 47")&&f.ContainsKey(30),"R43F parse: per-number list with stage/code/expected");
     Check(Engine.ParseFails(File.ReadAllText(ap),DateTime.Now.AddMinutes(5)).Count==0,"R43F parse: lines before 'since' ignored");
     var dg=Engine.LastDiag(File.ReadAllText(ap));
     Check(dg!=null&&dg[1]=="check90"&&dg[2].Contains("problems=2")&&dg.Length==5&&dg[3].StartsWith("4번"),"R43D parse: last diagnosis + problem list");
     var d2=Make(root).AppliedDetail(Make(root).Load(),Path.Combine(root,"none.log"));
     Check(d2[31].State=="FAIL"&&d2[31].Cell.StartsWith("실패 ✖ — hook hired-officer face sites")&&d2[31].Cell.Contains("[R43F-32-hook]")&&d2[31].Detail().Contains("기대: 66 89 47 10")&&d2[31].Detail().Contains("실제: e9 11 22 33 44"),"applied 32: R43F reason in cell + expected/actual in detail: "+d2[31].Cell);
     Check(d2[29].State=="PART"&&d2[29].Cell.StartsWith("일부 적용 ⚠ — hook item select list")&&d2[29].Detail().Contains("✖ hook item select list"),"applied 30: start OK + one R43F → PART with reason: "+d2[29].Cell);
     var n43=Engine.LastNotice("R43E\tMC\t2026-11-05 12:00:00\tR43-MC4-6ad06f53-2a02000\texpect=0x6a983c25/0x29ff000\tactual=0x6ad06f53/0x2a02000\t-\tSAN14_HiredFace.dll,SAN14_Range.dll\n");
     Check(n43!=null&&n43[1]=="R43-MC4-6ad06f53-2a02000"&&n43[3]=="expect=0x6a983c25/0x29ff000 / actual=0x6ad06f53/0x2a02000"&&n43[4].Contains("SAN14_Range.dll")&&Engine.NoticeMeaning(n43[1]).Contains("모르는 판"),"R43E notice parse: code + expected/actual + idle modules");
     File.WriteAllText(ap,File.ReadAllText(ap).Replace("R43F","R43X").Replace("R43D","R43Y"));}
    // 기록 폴더 수집·정리
    string dir=Make(root).CollectRecords("installer log text");
    Check(File.Exists(Path.Combine(dir,"설치기.log"))&&File.Exists(Path.Combine(dir,"적용확인.log")),"records folder: installer log + apply log kept");
    for(int i=0;i<60;i++)File.WriteAllText(Path.Combine(dir,"old_"+i+".log"),"x");Engine.TrimRecords(dir);
    Check(new DirectoryInfo(dir).GetFiles().Length<=50&&File.Exists(Path.Combine(dir,"적용확인.log")),"records folder trimmed to 50, apply log kept");
    Make(root).Install(new bool[Spec.Count],null);}
   // 5f. 설정 창 키: NewSettingsWindow=1, 사용자가 0 으로 바꾸면 유지, 13+2 면 AutoCredit=0
   {Make(root).Install(Only(1,2),null);var ini=Ini(root,Engine.SettingsRel);Check(ini["Modules\tNewSettingsWindow"]=="1"&&ini["Convenience\tAutoCredit"]=="1","settings: new window on, auto credit on (no 13)");
    var txt=Engine.ParseIni(Engine.Text(File.ReadAllBytes(Path.Combine(root,Engine.SettingsRel))));txt["Modules\tNewSettingsWindow"]="0";File.WriteAllText(Path.Combine(root,Engine.SettingsRel),Engine.WriteIni(txt,null));
    Make(root).Install(Only(1,2,13),null);ini=Ini(root,Engine.SettingsRel);Check(ini["Modules\tNewSettingsWindow"]=="1"&&ini["Convenience\tAutoCredit"]=="0","settings: R43 old window removed (NewSettingsWindow forced 1), 13 locks AutoCredit=0");
    Make(root).Install(new bool[Spec.Count],null);}
   Directory.Delete(root,true);
  }
  // 3. R20 기록 이어받기 (실제 사용자 state.xml 구조)
  if(args.Length>5){
   string root=NewGame("migrate");string oldPkg=args[4],stateXml=args[5];
   var byHash=new Dictionary<string,string>();foreach(string f in Directory.GetFiles(oldPkg,"*",SearchOption.AllDirectories))byHash[Files.HashFile(f)]=f;
   var tx=new Transaction(root);var doc=Files.Xml(stateXml);var fixed_=new List<XElement>();int placed=0,dummy=0;
   // 옛 통합편의 설정(사용자 조정 포함)과 BGM ini 는 임의 내용으로
   string oldIni="[Camera]\r\nAutoFocus=0\r\n[Convenience]\r\nAudienceGate=2\r\nDualCredit=1\r\nSkipReports=0\r\nAutoPersonnelState=88584\r\nNoPositionTroops=4000\r\n[Modules]\r\nModSettingsUI=1\r\nTraits9Module=0\r\n[BgmPlayer]\r\nEnabled=1\r\nSelected=5\r\n";
   foreach(var el in doc.Root.Element("Files").Elements("File")){
    string path=Files.Attr(el,"path"),aft=Files.Attr(el,"after"),bef=Files.Attr(el,"before");byte[] b;
    if(path==Engine.FixRel){Put(root,path,File.ReadAllBytes(fixOrig));b=null;} // Steam 이 1.0.1 원본으로 바꿈
    else if(path==Engine.ExpRel){b=File.ReadAllBytes(Path.Combine(pkg,"support","payload","f18","experience_data_exce.bin"));}
    else if(path==Engine.FontRel){b=File.ReadAllBytes(Path.Combine(pkg,"support","payload","f20","0002","RRPreview.rdb"));}
    else if(path==Engine.FontBinRel){b=File.ReadAllBytes(Path.Combine(pkg,"support","payload","f20","0002","RRPreview.rdb.bin99"));}
    else if(path==Engine.SettingsRel){b=Encoding.UTF8.GetBytes(oldIni);bef=tx.Put(Encoding.UTF8.GetBytes("[X]\r\nY=1\r\n"));}
    else if(byHash.ContainsKey(aft)){b=File.ReadAllBytes(byHash[aft]);placed++;}
    else{b=Encoding.UTF8.GetBytes("dummy "+path);dummy++;}
    if(b!=null){Put(root,path,b);aft=Files.Hash(b);}
    if(path==Engine.ExpRel)bef=tx.Put(File.ReadAllBytes(expOrig));
    if(path==Engine.FontRel)bef=tx.Put(File.ReadAllBytes(fontOrig));
    fixed_.Add(new XElement("File",new XAttribute("path",path),new XAttribute("before",bef),new XAttribute("after",aft),new XAttribute("kind",Files.Attr(el,"kind"))));
   }
   Console.WriteLine("   migrate fixture: real files "+placed+", dummies "+dummy);
   // 옛 공용 파일(기록 없음): 로더·ModCore·리졸버(옛 판)
   Put(root,"dxgi.dll",File.ReadAllBytes(Path.Combine(oldPkg,"payload","loaders","dxgi.dll")));Put(root,"SAN14Mods/Core/SAN14_ModCore.dll",File.ReadAllBytes(Path.Combine(oldPkg,"payload","SAN14Mods","Core","SAN14_ModCore.dll")));Put(root,"SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll",File.ReadAllBytes(Path.Combine(oldPkg,"payload","SAN14Mods","resolvers","SAN14_Resolver_CEKO.dll")));
   Put(root,"SAN14_Traits9_UI.log",Encoding.ASCII.GetBytes("log"));
   var nd=new XDocument(new XElement("SAN14Compat",new XAttribute("version",6),doc.Root.Element("Selected"),new XElement("Files",fixed_)));Put(root,Engine.StateRel,Files.XmlBytes(nd));
   var e=Make(root);var old=e.Load();
   Check(old.Version==6&&old.On[0]&&old.On[1]&&!old.On[8]==false,"legacy state mapped: 1,2 on; 9(old 11) on="+old.On[8]);
   Check(old.On[23]&&old.On[17]&&old.On[19]&&old.On[10]==false&&old.On[11]==false,"legacy mapping 24/18/20 on, 11/12 off");
   var st=e.Status(old);Check(st.Count(x=>x.Contains("갱신 필요"))>10,"legacy status asks update: "+st[0]);
   var before=Snapshot(root);
   int n=e.Install((bool[])old.On.Clone(),null);Check(n>40,"migration install ops="+n);
   Check(H(root,"version.dll")==""&&H(root,"SAN14_Compat20_Entry.dll")==""&&H(root,"SAN14Mods/modules/Compat20_Entry.ini")==""&&H(root,"SAN14_Stats200_CEKO.dll")==""&&H(root,"SAN14Mods/modules/Stats200CEKO.ini")==""&&H(root,"SAN14Mods/Compat20/Selections/14.xml")==""&&H(root,"SAN14CE_NoStatBonuses22/state.txt")==""&&H(root,"SAN14Mods/manifests/FPS120.json")=="","old files removed (version.dll, Compat20, Stats200CEKO, receipts, state.txt, manifests)");
   Check(H(root,"SAN14_IntegratedConvenience.dll")==M.Entries.First(f=>f.Target=="SAN14_IntegratedConvenience.dll").Sha&&H(root,"SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll")==M.Entries.First(f=>f.Target=="SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll").Sha,"IC/resolver updated to 1.0.1 ports");
   Check(H(root,"SAN14_CityCaps.dll")!=""&&H(root,"SAN14_Stats200New.dll")!=""&&H(root,"SAN14_FPS120.dll")==M.Entries.First(f=>f.Target=="SAN14_FPS120.dll").Sha,"new modules present (CityCaps, Stats200, FPS120 modpack)");
   Check(H(root,Engine.FixRel)==M.Values["fix_nostat"]&&H(root,Engine.ExpRel)==M.Values["exp_modified"]&&H(root,Engine.FontRel)==M.Values["font_rdb"],"data: nostat, exp modified, font ours");
   var ini=Ini(root,Engine.SettingsRel);Check(ini["Convenience\tDualCredit"]=="0"&&ini["Convenience\tSkipReports"]=="0"&&ini["Convenience\tNoPositionTroops"]=="4000"&&ini["Convenience\tAutoPersonnelState"]=="88584"&&ini["BgmPlayer\tSelected"]=="5"&&ini["Convenience\tPrisonerPersuasion"]=="1","legacy settings merged: DualCredit 0, user values kept, missing keys defaulted");
   Check(File.Exists(Path.Combine(root,"San14BgmExpansion/music/user_song.mp3"))&&File.Exists(Path.Combine(root,"SAN14_Traits9_UI.log")),"user files untouched");
   var s2=e.Load();Check(s2.Version==7&&s2.Items.ContainsKey(Engine.FixRel)&&s2.Items[Engine.FixRel].Before==M.Values["fix_original"],"state v7, fixdata Before = 1.0.1 original");
   Check(s2.Items[Engine.FontRel].Before==M.Values["font_original"],"font record keeps original Before");
   st=e.Status(s2);Check(st.Where((x,i)=>s2.On[i]).All(x=>x.StartsWith("설치됨")),"post-migration status 설치됨: "+String.Join("|",st.Distinct().ToArray()));
   // 전부 제거 → 데이터 원본, 사용자 파일 유지, 설정 보관
   e.Install(new bool[Spec.Count],null);var after=Snapshot(root);
   Check(H(root,Engine.FixRel)==M.Values["fix_original"]&&H(root,Engine.ExpRel)==M.Values["exp_original"]&&H(root,Engine.FontRel)==M.Values["font_original"]&&H(root,Engine.FontBinRel)=="","remove after migration: originals");
   var leftovers=after.Keys.Where(k=>k.EndsWith(".dll")||k.EndsWith(".ini")).ToList();
   Check(leftovers.Count==1&&leftovers[0]==Engine.SettingsRel,"leftovers after remove (settings ini restored to pre-R20 user file only): "+String.Join(",",leftovers.ToArray()));
   Check(Directory.Exists(Path.Combine(root,".san14ce_compat_r3","user_settings")),"user settings copy saved");
  }
  // R40 A22: 일부만 설치한 사용자 — 마지막 실행에서 통합패치가 통째로 멈춘 경우 진짜 이유를 보여 줌
  {
   string root=NewGame("a22");var e=Make(root);var sel=Only(18,19,23,24,25,33);e.Install(sel,null);var s=e.Load();
   Check(H(root,"SAN14Mods/Core/SAN14_Notice.dll")!=""&&H(root,"SAN14Mods/Core/SAN14_ModCore.dll")!="","A22: notice DLL and ModCore installed with partial selection");
   Check(H(root,"SAN14_ModSettings.dll")!=""&&H(root,"SAN14Mods/modules/ModSettings.ini")!="","A22: mod settings installed even without item 1");
   string rl=Path.Combine(root,"rt_a22.log");System.Threading.Thread.Sleep(1100);
   File.WriteAllText(rl,"SAN14 ModCore 1.0 / standalone SortiePresets RC23 startup\r\nSKIPPED: unsupported version, ambiguous signature, native layout or runtime limit mismatch.\r\n");
   var ad=e.AppliedDetail(s,rl);
   Check(ad[18].State=="FAIL"&&ad[18].Cell.Contains("통합패치 전체 꺼짐")&&ad[22].Cell.Contains("통합패치 전체 꺼짐")&&ad[24].Cell.Contains("통합패치 전체 꺼짐"),"A22: resolver stop shown for 19/23/25 instead of 'no start record': "+ad[18].Cell);
   Check(ad[23].State=="FILE","A22: data item 24 still judged by file: "+ad[23].Cell);
   Check(ad[32].Cell.Contains("통합패치 전체 꺼짐"),"A22: item 33 (no legacy judge) also shows whole stop");
   Directory.CreateDirectory(Path.Combine(root,Engine.RecordsDir));File.AppendAllText(Path.Combine(root,Engine.ApplyLogRel),"R40E\tMC\t2026-10-28 20:00:00\tR40-MC2-6a983c25-3b57000\tdinput8.dll\n");
   ad=e.AppliedDetail(s,rl);
   Check(ad[18].Cell.Contains("통째로")&&ad[18].Reason.Contains("R40-MC2")&&ad[18].Reason.Contains("dinput8.dll"),"A22: in-game notice code shown: "+ad[18].Cell);
   File.WriteAllText(rl,"SAN14 ModCore 1.0 / standalone SortiePresets RC23 startup\r\nSKIPPED: Resolver missing.\r\n");
   File.WriteAllText(Path.Combine(root,Engine.ApplyLogRel),"");ad=e.AppliedDetail(s,rl);
   Check(ad[18].Cell.Contains("리졸버"),"A22: resolver missing reason: "+ad[18].Cell);
   File.WriteAllText(rl,"SAN14 ModCore 1.0 / standalone SortiePresets RC23 startup\r\nCE KO: timestamp 6a983c25 / image 029ff000 / layout ABI 1 / unique signature resolution\r\nFPS120 R8 ACTIVE\r\n");
   ad=e.AppliedDetail(s,rl);
   Check(ad[18].Cell.Contains("이 폴더의 통합패치가 실행되지 않음"),"A22: game ran from another folder (no CrashLog line) detected: "+ad[18].Cell);
   File.WriteAllText(rl,"SAN14 ModCore 1.0 / standalone SortiePresets RC23 startup\r\nCE KO: timestamp 6a983c25 / image 029ff000 / layout ABI 1 / unique signature resolution\r\nCrashLog: crash recorder armed (SAN14_CrashLog.log); no game code changed.\r\nFPS120 R8 ACTIVE\r\nCityCaps: city limits extended\r\n");
   ad=e.AppliedDetail(s,rl);
   Check(e.WholeStop(s,rl,DateTime.MinValue)==null&&ad[18].Cell.StartsWith("적용됨")&&ad[22].Cell.StartsWith("적용됨"),"A22: normal run judged normally: "+ad[18].Cell+" / "+ad[22].Cell);
   Check(e.BaseChanged(s).Count==0,"A22: base files unchanged after install");
   // R43: 공용 리졸버(CEKO)는 다른 모드 것이면 그대로 두는 파일이라 BaseChanged 가 일부러 뺌(통합패치는 자기 리졸버 IP01 을 따로 씀) → 우리 리졸버로 확인
   Put(root,Engine.OurResolverRel,new byte[]{1,2,3});
   Check(e.BaseChanged(s).Contains(Engine.OurResolverRel),"A22: overwritten common file detected");
   Put(root,Engine.SharedResolverRel,new byte[]{1,2,3});
   Check(!e.BaseChanged(s).Contains(Engine.SharedResolverRel),"A22: shared resolver (CEKO) owned by other mods is not reported");
   e.Install(sel,null);Check(e.BaseChanged(e.Load()).Count==0,"A22: reinstall restores common file");
   byte[] hdr=new byte[8192];Array.Copy(File.ReadAllBytes(gameExe),hdr,8192);int pe=BitConverter.ToInt32(hdr,0x3c);
   Check(Engine.ExeKindFromHeader(hdr).Contains("내용이 바뀜"),"A22: same-header changed exe: "+Engine.ExeKindFromHeader(hdr));
   var h2=(byte[])hdr.Clone();BitConverter.GetBytes(0x3b57000u).CopyTo(h2,pe+0x18+56);Check(Engine.ExeKindFromHeader(h2).Contains("통째로"),"A22: replaced exe (1000-officer type) classified");
   var h3=(byte[])hdr.Clone();BitConverter.GetBytes(0x6a6d9f72u).CopyTo(h3,pe+8);Check(Engine.ExeKindFromHeader(h3).Contains("이전"),"A22: old exe classified");
   var h4=(byte[])hdr.Clone();BitConverter.GetBytes(0x6a983c25u).CopyTo(h4,pe+8);Check(Engine.ExeKindFromHeader(h4).Contains("옛 1.0.1"),"R43: previous 1.0.1 exe classified as old: "+Engine.ExeKindFromHeader(h4));
   Check(Spec.OldGameIndex(Spec.OldGames[0])==0&&Spec.OldGameIndex("x")<0&&Spec.ExpectedGame!=Spec.OldGames[0],"R43: old game table");
   Check(Engine.ExeKind(gameExe)=="","A22: supported exe passes");
  }
  { // R43 시험2: 설치 기록(state.xml)이 없는 폴더에 이전 시험판이 남긴 '우리' 파일(모듈 ini 는 지금 자료와 같은 내용, Core DLL 은 Known 해시)이 있어도 설치가 막히지 않아야 함
   string root=NewGame("nostate_prev");var sel=All();
   string diagSrc="";foreach(var f in M.Entries){if(f.Target=="SAN14Mods/modules/Diag.ini"){diagSrc=f.Source;break;}}
   Check(diagSrc!="","R43: Diag.ini entry exists");
   byte[] diag=File.ReadAllBytes(Path.Combine(pkg,diagSrc.Replace('/',Path.DirectorySeparatorChar)));Put(root,"SAN14Mods/modules/Diag.ini",diag);
   var e=Make(root);string msg="";try{e.Install(sel,null);}catch(Exception ex){msg=ex.Message;}
   Check(msg=="","R43: install over leftover identical module ini without state.xml succeeds: "+msg);
   var st=e.Load();Record rq;Check(st.Items.TryGetValue("SAN14Mods/modules/Diag.ini",out rq)&&File.Exists(Path.Combine(root,Engine.StateRel)),"R43: state records Diag.ini after install");
  }
  Console.WriteLine(fails==0?"ALL TESTS PASSED":fails+" FAILURES");
  return fails==0?0:1;
 }
}
}
