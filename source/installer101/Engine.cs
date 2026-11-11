using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace San14Patch101 {
public sealed class State {
 public bool[] On=new bool[Spec.Count];
 public Dictionary<string,Record> Items=new Dictionary<string,Record>(StringComparer.OrdinalIgnoreCase);
 public bool RetiredOn; // R41: 옛 기록에 12번(삭제된 안개)이 켜져 있었음
 public int Version; // 읽은 기록 형식 (3~6: R20 이하 설치기, 7: 이 설치기)
 public string Installer=""; // R43.28: 마지막으로 설치한 설치기 판(기록의 installer 속성)
 public HashSet<string> LostBackup=new HashSet<string>(StringComparer.OrdinalIgnoreCase); // R43 7차: 백업 파일이 없어진 기록(백신 격리 등) — 제거 때 되돌릴 원본이 없으므로 그 파일은 남겨 둠
 public byte[] Bytes(){return Files.XmlBytes(new XDocument(new XElement("SAN14Compat",new XAttribute("version",7),new XAttribute("installer",Spec.Version),new XElement("Selected",On.Select((b,i)=>new XElement("Feature",new XAttribute("id",i+1),new XAttribute("enabled",b)))),new XElement("Files",Items.Values.OrderBy(x=>x.Path,StringComparer.Ordinal).Select(x=>x.Xml())))));}
}
public sealed class InstallException:IOException {
 public readonly string Feature,Target;
 public InstallException(string feature,string target,string reason):base(Build(feature,target,reason)){Feature=feature;Target=target;}
 static string Build(string f,string t,string r){var b=new StringBuilder();b.AppendLine("설치를 진행하지 않았습니다. 게임 파일은 바꾸지 않았습니다.");if(!String.IsNullOrEmpty(f))b.AppendLine("관련 항목: "+f);if(!String.IsNullOrEmpty(t))b.AppendLine("대상 파일: "+t);b.Append("이유: "+r);return b.ToString();}
}

public sealed class Engine {
 public const string StateRel=".san14ce_compat_r3/state.xml"; // R20 이하와 같은 자리 (기존 설치를 이어받음)
 public const string SettingsRel="San14ModSettings.ini",BridgeIniRel="SAN14Mods/modules/ModPackBridge.ini",BgmIniRel="San14BgmExpansion.ini",Patch27IniRel="SAN14Mods/Patch27/Options.ini";
 public const string FogIniRel="SAN14_FogAI.ini";
 public const string FixRel="0010_KO/fixdataexce.s14",ExpRel="0000/experience_data_exce.bin",FontRel="0002/RRPreview.rdb",FontBinRel="0002/RRPreview.rdb.bin99";
 static readonly string[] LoaderNames={"dxgi.dll","version.dll","dinput8.dll"};
 // R35 10번 추가 3D 모델(25~173): 0002 색인 3개는 게임 원본 파일 → 데이터 파일처럼(제거·복원은 항상 1.0.1 원본으로), 새 컨테이너·목록은 새 파일
 // R39 31번 주메뉴 그래픽: 0002/ScreenLayout.rdb 색인도 같은 방식(레코드 9개 교체 + 새 컨테이너)
 public sealed class IndexSpec{public int Id;public string Rel,OrigPayload,ValueKey,Label,PrevPayload,PrevKey;}
 public static readonly IndexSpec[] Indexes={
  new IndexSpec{Id=10,Rel="0002/CharacterEditor.rdb",OrigPayload="data/model101/CharacterEditor.rdb",ValueKey="model_orig_CharacterEditor.rdb",Label="10번 추가 모델(25~173)"},
  new IndexSpec{Id=10,Rel="0002/MaterialEditor.rdb",OrigPayload="data/model101/MaterialEditor.rdb",ValueKey="model_orig_MaterialEditor.rdb",Label="10번 추가 모델(25~173)"},
  new IndexSpec{Id=10,Rel="0002/KIDSSystemResource.rdb",OrigPayload="data/model101/KIDSSystemResource.rdb",ValueKey="model_orig_KIDSSystemResource.rdb",Label="10번 추가 모델(25~173)",PrevPayload="data/prev101/KIDSSystemResource.rdb",PrevKey="prev_KIDSSystemResource.rdb"},
  new IndexSpec{Id=31,Rel="0002/ScreenLayout.rdb",OrigPayload="data/screen101/ScreenLayout.rdb",ValueKey="screen_orig_ScreenLayout.rdb",Label="31번 주메뉴 그래픽",PrevPayload="data/prev101/ScreenLayout.rdb",PrevKey="prev_ScreenLayout.rdb"}};
 public static readonly string[] ModelIndexRels={"0002/CharacterEditor.rdb","0002/MaterialEditor.rdb","0002/KIDSSystemResource.rdb"};
 public const string ScreenIndexRel="0002/ScreenLayout.rdb";
 public const string ModelCatalogRel="San14ModelRegistry.tsv",LinksPayload="support/payload/f10/OfficerModels_R35.tsv",LinksMarker="SAN14_R35_links_applied.txt";
 static IndexSpec IndexOf(string r){return Indexes.FirstOrDefault(x=>x.Rel.Equals(r,StringComparison.OrdinalIgnoreCase));}
 static bool IsModelIndex(string r){return IndexOf(r)!=null;}
 // 10번·31번의 '레코드 자료'(색인·컨테이너·목록): 설치는 Plan 함수가 따로 다룸
 static bool ModelData(ManifestFile f){return (f.Id==10&&(f.Target.StartsWith("0002/",StringComparison.OrdinalIgnoreCase)||f.Target.Equals(ModelCatalogRel,StringComparison.OrdinalIgnoreCase)))||(f.Id==31&&f.Target.StartsWith("0002/",StringComparison.OrdinalIgnoreCase));}
 public string ModelSkip="",ScreenSkip=""; public bool ModelInstalled,ScreenInstalled;
 public bool ForceFont; // 20번: 다른 글꼴 패치가 있어도 백업 후 설치(사용자가 알림창에서 고름)
 readonly HashSet<string> leave=new HashSet<string>(StringComparer.OrdinalIgnoreCase); // 제거 때 손대지 않을 파일(이미 깨끗하거나 다른 도구가 바꾼 색인)
 // R37: 0002 색인 3개는 파일 통째가 아니라 '우리 레코드'만 넣고 뺌 → 같은 색인을 고치는 다른 도구(맵 확장 등)와 함께 써도 서로 안 지움
 sealed class Rdb{public byte[] Head;public List<KeyValuePair<uint,byte[]>> Recs=new List<KeyValuePair<uint,byte[]>>();}
 static Rdb ParseRdb(byte[] b){
  if(b==null||b.Length<32||Encoding.ASCII.GetString(b,0,8)!="_DRK0000"||BitConverter.ToInt32(b,8)!=32)return null;
  uint n=BitConverter.ToUInt32(b,16);if(n>200000)return null;var r=new Rdb{Head=b.Take(32).ToArray()};long pos=32;
  for(uint i=0;i<n;i++){
   if(pos+56>b.Length||Encoding.ASCII.GetString(b,(int)pos,8)!="IDRK0000")return null;
   long size=BitConverter.ToInt64(b,(int)pos+8);if(size<56||pos+size>b.Length)return null;
   var blk=new byte[size];Array.Copy(b,pos,blk,0,size);r.Recs.Add(new KeyValuePair<uint,byte[]>(BitConverter.ToUInt32(b,(int)pos+36),blk));pos=(pos+size+3)&~3L;
  }
  if(pos!=b.Length||r.Recs.Select(x=>x.Key).Distinct().Count()!=r.Recs.Count)return null;
  return r;
 }
 static byte[] WriteRdb(Rdb r){var m=new MemoryStream();var h=(byte[])r.Head.Clone();BitConverter.GetBytes((uint)r.Recs.Count).CopyTo(h,16);m.Write(h,0,32);foreach(var kv in r.Recs){m.Write(kv.Value,0,kv.Value.Length);int pad=(4-kv.Value.Length%4)%4;m.Write(new byte[pad],0,pad);}return m.ToArray();}
 static bool Same(byte[] a,byte[] b){return a.Length==b.Length&&a.SequenceEqual(b);}
 // 우리 레코드: 합본 색인 중 1.0.1 원본에 없거나 내용이 다른 것. replaced = 원본에도 있는 것의 원본 레코드
 readonly Dictionary<string,KeyValuePair<List<KeyValuePair<uint,byte[]>>,Dictionary<uint,byte[]>>> modelRecs=new Dictionary<string,KeyValuePair<List<KeyValuePair<uint,byte[]>>,Dictionary<uint,byte[]>>>(StringComparer.OrdinalIgnoreCase);
 KeyValuePair<List<KeyValuePair<uint,byte[]>>,Dictionary<uint,byte[]>> ModelRecs(string r){
  KeyValuePair<List<KeyValuePair<uint,byte[]>>,Dictionary<uint,byte[]>> v;if(modelRecs.TryGetValue(r,out v))return v;
  var spec=IndexOf(r);var f=M.Entries.First(x=>ModelData(x)&&x.Id==spec.Id&&x.Target.Equals(r,StringComparison.OrdinalIgnoreCase));
  var U=ParseRdb(Payload(f.Source,f.Sha));var O=ParseRdb(Payload(spec.OrigPayload,M.Values[spec.ValueKey]));if(U==null||O==null)throw new IOException("설치 자료 색인을 읽지 못했습니다: "+r);
  var om=O.Recs.ToDictionary(x=>x.Key,x=>x.Value);var ours=new List<KeyValuePair<uint,byte[]>>();var rep=new Dictionary<uint,byte[]>();
  foreach(var kv in U.Recs){byte[] o;if(!om.TryGetValue(kv.Key,out o)){ours.Add(kv);}else if(!Same(o,kv.Value)){ours.Add(kv);rep[kv.Key]=o;}}
  v=new KeyValuePair<List<KeyValuePair<uint,byte[]>>,Dictionary<uint,byte[]>>(ours,rep);modelRecs[r]=v;return v;
 }
 // R43: 옛 게임판(1.0.1) 원본 레코드가 그대로 남은 색인(옛 설치기가 1.0.1 원본으로 되돌린 자리) → 그 레코드만 새 판 원본으로.
 // 1.0.1 원본과 한 바이트라도 다르면(다른 모드가 고친 것) 손대지 않음
 readonly Dictionary<string,Dictionary<uint,KeyValuePair<byte[],byte[]>>> staleRecs=new Dictionary<string,Dictionary<uint,KeyValuePair<byte[],byte[]>>>(StringComparer.OrdinalIgnoreCase);
 Dictionary<uint,KeyValuePair<byte[],byte[]>> StaleRecs(string r){
  Dictionary<uint,KeyValuePair<byte[],byte[]>> v;if(staleRecs.TryGetValue(r,out v))return v;
  v=new Dictionary<uint,KeyValuePair<byte[],byte[]>>();var spec=IndexOf(r);string pv,ov;
  if(spec!=null&&spec.PrevPayload!=null&&M.Values.TryGetValue(spec.PrevKey,out pv)&&M.Values.TryGetValue(spec.ValueKey,out ov)){
   var P=ParseRdb(Payload(spec.PrevPayload,pv));var O=ParseRdb(Payload(spec.OrigPayload,ov));if(P==null||O==null)throw new IOException("설치 자료 색인을 읽지 못했습니다: "+r);
   var om=O.Recs.ToDictionary(x=>x.Key,x=>x.Value);
   foreach(var kv in P.Recs){byte[] o;if(om.TryGetValue(kv.Key,out o)&&!Same(o,kv.Value))v[kv.Key]=new KeyValuePair<byte[],byte[]>(kv.Value,o);}
  }
  staleRecs[r]=v;return v;
 }
 int Refresh(string r,Rdb C){
  var s=StaleRecs(r);int n=0;if(s.Count==0)return 0;
  for(int i=0;i<C.Recs.Count;i++){KeyValuePair<byte[],byte[]> p;if(s.TryGetValue(C.Recs[i].Key,out p)&&Same(C.Recs[i].Value,p.Key)){C.Recs[i]=new KeyValuePair<uint,byte[]>(C.Recs[i].Key,p.Value);n++;}}
  return n;
 }
 // 우리 레코드와 상관없이 옛 판 레코드만 새 판 원본으로 바꾼 결과. 바꿀 것이 없으면 null
 byte[] RefreshOnly(string r,byte[] cur){var C=ParseRdb(cur);if(C==null)return null;return Refresh(r,C)>0?WriteRdb(C):null;}
 // 현재 색인에 우리 레코드를 넣은 결과(이미 들어 있으면 그대로). 같은 번호를 다른 내용으로 쓰는 경우 null
 byte[] ModelPatch(string r,byte[] cur){
  var C=ParseRdb(cur);if(C==null)return null;Refresh(r,C);var mr=ModelRecs(r);
  foreach(var kv in mr.Key){
   int i=C.Recs.FindIndex(x=>x.Key==kv.Key);
   if(mr.Value.ContainsKey(kv.Key)){if(i<0)return null;if(Same(C.Recs[i].Value,kv.Value))continue;if(!Same(C.Recs[i].Value,mr.Value[kv.Key]))return null;C.Recs[i]=kv;}
   else{if(i>=0){if(!Same(C.Recs[i].Value,kv.Value))return null;continue;}C.Recs.Add(kv);}
  }
  return WriteRdb(C);
 }
 // 현재 색인에서 우리 레코드만 뺀 결과(없으면 그대로). 우리 것이 아닌 내용이 그 자리에 있으면 null
 byte[] ModelUnpatch(string r,byte[] cur){
  var C=ParseRdb(cur);if(C==null)return null;Refresh(r,C);var mr=ModelRecs(r);
  foreach(var kv in mr.Key){
   int i=C.Recs.FindIndex(x=>x.Key==kv.Key);
   if(mr.Value.ContainsKey(kv.Key)){if(i<0)return null;if(Same(C.Recs[i].Value,mr.Value[kv.Key]))continue;if(!Same(C.Recs[i].Value,kv.Value))return null;C.Recs[i]=new KeyValuePair<uint,byte[]>(kv.Key,mr.Value[kv.Key]);}
   else if(i>=0){if(!Same(C.Recs[i].Value,kv.Value))return null;C.Recs.RemoveAt(i);}
  }
  return WriteRdb(C);
 }
 readonly string root,pkg; readonly Manifest M; public readonly Transaction Tx; public Action<string> Log=delegate{};
 readonly Dictionary<string,string> accepted=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase); // 준비 중 확인한 현재 파일 해시(덮어쓰기 허용)
 public Engine(string gameRoot,string packageRoot,Manifest manifest){root=Path.GetFullPath(gameRoot);pkg=Path.GetFullPath(packageRoot);M=manifest;Tx=new Transaction(root);}
 public const string GameRunningMark="아직 켜져 있습니다";
 // 18차(P224): 무엇이 켜져 있는지 이름을 적는다(창은 닫혔는데 프로세스만 남은 경우를 제보 글자로 가릴 수 있게).
 public static void GameClosed(){string names="";foreach(string n in new[]{"SAN14CE_KO","SAN14_Launcher"})if(Process.GetProcessesByName(n).Length>0)names+=(names.Length>0?", ":"")+n+".exe";
  if(names.Length>0)throw new IOException(names+" 이(가) "+GameRunningMark+". 게임과 게임 런처를 종료한 뒤 다시 눌러 주세요.");}
 public void ValidateGame(){
  GameClosed();string h=Files.HashFile(Files.Child(root,"SAN14CE_KO.exe"));
  {int oi=Spec.OldGameIndex(h);if(oi>=0)throw new IOException("게임 실행 파일이 옛 판("+Spec.OldGameLabels[oi]+")입니다. 이 설치기("+Spec.Version+")는 "+Spec.GameLabel+" 전용입니다. Steam에서 게임을 최신으로 업데이트한 뒤 다시 실행해 주세요(옛 판을 그대로 쓰려면 R42i4 이하 통합패치). 게임 파일은 바꾸지 않았습니다.");}
  if(h!=Spec.ExpectedGame)throw new IOException("지원하는 Steam 한국어 CE "+Spec.GameLabel+" 게임 파일인지 확인할 수 없습니다(SAN14CE_KO.exe, SHA-256 "+h.Substring(0,16)+"…). 게임 파일은 바꾸지 않았습니다.");
  if(!File.Exists(Files.Child(root,"SAN14_Launcher.exe")))throw new IOException("게임 폴더의 SAN14_Launcher.exe를 찾지 못했습니다. 게임 폴더를 다시 선택해 주세요.");
 }
 public string Hash(string r){return Files.HashFile(Files.Child(root,r));}
 byte[] Read(string r){return File.ReadAllBytes(Files.Child(root,r));}
 public static string Text(byte[] b){using(var r=new StreamReader(new MemoryStream(b),Encoding.UTF8,true))return r.ReadToEnd();}
 // R39: 남의 키트 설정 파일(SAN14Mods/Ext/*.ini, SAN14Mods/Minimap/*.ini)과 새 설정 창 저장 파일도 사용자 설정 → 있으면 그대로 둠
 static bool Mutable(string r){return r.Equals(SettingsRel,StringComparison.OrdinalIgnoreCase)||r.Equals(Patch27IniRel,StringComparison.OrdinalIgnoreCase)||r.Equals(BgmIniRel,StringComparison.OrdinalIgnoreCase)||r.Equals("SAN14_DomesticCE.ini",StringComparison.OrdinalIgnoreCase)||r.Equals("SAN14_FogAI.ini",StringComparison.OrdinalIgnoreCase)
  ||r.Equals(ModSettingsRel,StringComparison.OrdinalIgnoreCase)||RuntimeIni.Contains(r)||((r.StartsWith("SAN14Mods/Ext/",StringComparison.OrdinalIgnoreCase)||r.StartsWith("SAN14Mods/Minimap/",StringComparison.OrdinalIgnoreCase))&&r.EndsWith(".ini",StringComparison.OrdinalIgnoreCase));}
 // R39: 게임 안 새 설정 창·모듈이 직접 값을 써 넣는 파일 → 사용자 설정과 같게(있으면 유지, 바뀌어 있어도 갱신·제거가 막히지 않음, 제거 때 사본 보관)
 // 정식판 뒤(제보): 22번 DomesticPlayer·DomesticAI, 2번 AutoMerit 도 모드 설정 창에서 스위치를 바꾸면 자기 ini 에 저장 → 빠져 있어 다음 설치가 "다른 프로그램이 바꾼 파일"로 멈췄음
 static readonly HashSet<string> RuntimeIni=new HashSet<string>(StringComparer.OrdinalIgnoreCase){"SAN14Mods/modules/ModSettings.ini","SAN14Mods/modules/ExtBridge.ini","SAN14Mods/modules/HiredFace.ini","SAN14Mods/modules/StatusColor.ini","SAN14Mods/modules/Range.ini","SAN14Mods/modules/RegionOfficer.ini","SAN14Mods/modules/SceneSkip.ini","SAN14Mods/modules/Traits9Check.ini","SAN14Mods/modules/Mod09_Traits9Check.ini","SAN14Mods/modules/DomesticPlayer.ini","SAN14Mods/modules/DomesticAI.ini","SAN14Mods/modules/AutoPersonnelRepairMerit.ini","SAN14_WheelRegion.ini","SAN14_DragLearn.ini","SAN14_DragGive.ini"};
 // 정식판 뒤(11-08 갤 제보): 9번 확인 모듈 등록 이름을 14번(ModPackBridge.ini)보다 앞 순서로 → 13번 없이도 9칸 화면 부품이 14번보다 먼저 켜져 개성 설명이 나옴
 public const string Traits9CheckIniRel="SAN14Mods/modules/Mod09_Traits9Check.ini",OldTraits9CheckIniRel="SAN14Mods/modules/Traits9Check.ini";
 public const string ModSettingsRel="SAN14Mods/ModSettings.ini",RecordsDir="SAN14_기록",ApplyLogRel="SAN14_기록/적용확인.log";
 static bool ModuleIni(string r){return r.StartsWith("SAN14Mods/modules/",StringComparison.OrdinalIgnoreCase)&&r.EndsWith(".ini",StringComparison.OrdinalIgnoreCase);}
 // 게임 데이터 파일: 제거·복원은 항상 1.0.1 원본으로 (R20 이하가 백업한 1.0.1 이전 원본은 되살리지 않음)
 bool IsData(string r){return r.Equals(FixRel,StringComparison.OrdinalIgnoreCase)||r.Equals(ExpRel,StringComparison.OrdinalIgnoreCase);}
 byte[] DataOriginal(string r){string src=r.Equals(FixRel,StringComparison.OrdinalIgnoreCase)?"data/fixdata_original_101.s14":"data/experience_original.bin";return Payload(src,M.Values[r.Equals(FixRel,StringComparison.OrdinalIgnoreCase)?"fix_original":"exp_original"]);}
 List<string> roots;
 List<string> Roots(){if(roots==null)roots=Spec.PayloadRoots(pkg);return roots;}
 // R29: 설치 자료 읽기 — 설치 자료 폴더에 없으면 옆 폴더(다른 ZIP 을 따로 푼 곳)에서 같은 해시의 파일을 찾음
 byte[] Payload(string rel,string sha){
  bool seen=false;
  foreach(string r in Roots()){string f=Files.Child(r,rel);if(!File.Exists(f))continue;seen=true;byte[] b=File.ReadAllBytes(f);if(Files.Hash(b)==sha)return b;}
  if(!seen)throw new IOException(Spec.MissingText(new[]{rel}));
  throw new IOException("설치 자료가 손상되었습니다(압축을 새 폴더에 다시 풀어 주세요): "+rel);
 }
 bool PayloadPresent(string rel){foreach(string r in Roots())if(File.Exists(Files.Child(r,rel)))return true;return false;} // 있는지만(내용은 읽을 때 해시로 확인)
 // 고른 번호에 필요한 설치 자료 중 없는 것 (설치 전에 미리 확인)
 public List<string> MissingPayloads(bool[] on){
  var miss=new List<string>();
  Action<string,string> need=(rel,sha)=>{if(!miss.Contains(rel)&&!PayloadPresent(rel))miss.Add(rel);};
  foreach(var f in M.Entries)if(f.Applies(on)&&!ModelData(f))need(f.Source,f.Sha);
  foreach(var x in Indexes)if(on[x.Id-1]){string v;if(M.Values.TryGetValue(x.ValueKey,out v))need(x.OrigPayload,v);if(x.PrevPayload!=null&&M.Values.TryGetValue(x.PrevKey,out v))need(x.PrevPayload,v);}
  need("data/fixdata_original_101.s14",M.Values["fix_original"]);need("data/experience_original.bin",M.Values["exp_original"]);
  if(on[23])need("data/fixdata_nostat_101.s14",M.Values["fix_nostat"]);
  if(on[17])need("support/payload/f18/experience_data_exce.bin",M.Values["exp_modified"]);
  if(Spec.AnyRuntime(on))foreach(string n in LoaderNames)need("support/payload/loaders/"+n,M.Values["loader_"+n]);
  return miss;
 }
 bool Known(string h){return h!=""&&M.Known.ContainsKey(h);}
 // 10번 추가 모델 자료 중 받은 ZIP 에 없는 것 (있으면 10번은 기본 모델만 설치)
 public List<string> MissingModelPayloads(){var miss=new List<string>();foreach(var f in M.Entries)if(ModelData(f)&&!PayloadPresent(f.Source))miss.Add(f.Source);return miss;}

 public State Load(){
  var s=new State();string path=Files.Child(root,StateRel);
  if(!File.Exists(path))return s;
  // R43 11-08 7차: 설치 기록을 못 읽으면(깨짐·다른 프로그램 형식) 멈추지 않고 따로 보관한 뒤 새 기록으로 시작
  //  (이미 깔린 우리 파일은 해시로 알아봐서 설치 때 그대로 이어받음 — Plan 의 Known/같은 파일 규칙)
  System.Xml.Linq.XDocument d=null;int v=0;string why="";
  try{d=Files.Xml(path);if(d.Root.Name!="SAN14Compat"||!Int32.TryParse(Files.Attr(d.Root,"version"),out v)||v<3||v>7)why="형식을 알 수 없음";}
  catch(Exception e){why=e.Message;}
  if(why!=""){
   string keep=path+".broken_"+DateTime.Now.ToString("yyyyMMdd_HHmmss");
   try{File.Move(path,keep);}catch(Exception){}
   Log("설치 기록(state.xml)을 읽지 못해 따로 보관하고("+Path.GetFileName(keep)+") 새 기록으로 시작합니다. 이유: "+why+". 이미 깔린 통합패치 파일은 설치할 때 그대로 이어받습니다.");
   return s;
  }
  s.Version=v;var sel=d.Root.Element("Selected");
  // R39: 번호 26 은 R24 기록에서만 '안개 날씨 제거판(12.5)' — 그 뒤 판에서는 26번 기능확장
  string inst=Files.Attr(d.Root,"installer");s.Installer=inst;bool legacyFog=inst=="R21"||inst=="R22"||inst=="R23"||inst=="R24";
  if(sel!=null)foreach(var e in sel.Elements("Feature")){int id;if(!Int32.TryParse(Files.Attr(e,"id"),out id))continue;bool on=Files.Attr(e,"enabled").Equals("true",StringComparison.OrdinalIgnoreCase);int n=v==7?(id==Spec.LegacyFogNoWeather&&legacyFog?Spec.Fog:id>=1&&id<=Spec.Count?id:0):Spec.Legacy(id);if(n!=0&&on)s.On[n-1]=true;}
  if(s.On[Spec.Fog-1]&&!KeepRetired){s.On[Spec.Fog-1]=false;s.RetiredOn=true;} // R41: 12번 삭제 — 갱신 때 그 파일을 지움
  var files=d.Root.Element("Files");
  // R43 7차: 기록 한 줄이 이상해도(잘못된 경로·같은 파일 두 번·백업 파일이 없어짐) 전체를 멈추지 않음
  if(files!=null)foreach(var e in files.Elements("File")){
   Record q;try{q=new Record{Path=Files.Relative(Files.Attr(e,"path")),Before=Files.Attr(e,"before"),After=Files.Attr(e,"after"),Kind=Files.Attr(e,"kind")};}catch(Exception){continue;}
   if(s.Items.ContainsKey(q.Path))continue;
   if(q.Before!=""&&!IsData(q.Path)){try{Tx.Get(q.Before);}catch(Exception){s.LostBackup.Add(q.Path);}}
   s.Items.Add(q.Path,q);}
  if(s.LostBackup.Count>0)Log("참고: 설치 때 보관한 원래 파일 백업 "+s.LostBackup.Count+"개가 없어졌습니다(백신 격리 등). 그 파일들은 제거할 때 되돌릴 원본이 없어 그대로 둡니다: "+String.Join(", ",s.LostBackup.Take(3).ToArray())+(s.LostBackup.Count>3?" 외":""));
  return s;
 }

 // ---------- 원하는 결과 파일 집합 ----------
 public Dictionary<string,byte[]> Desired(State old,bool[] on){
  accepted.Clear();var d=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);var owner=new Dictionary<string,int>(StringComparer.OrdinalIgnoreCase);
  ModelSkip="";ModelInstalled=false;
  foreach(var f in M.Entries){
   if(!f.Applies(on)||ModelData(f))continue;
   if(d.ContainsKey(f.Target))throw new IOException("설치 자료 목록 오류: 같은 대상에 두 자료가 선택되었습니다: "+f.Target);
   owner[f.Target]=f.Id;
   try{
    byte[] b=Payload(f.Source,f.Sha);string h=Hash(f.Target);
    if(Mutable(f.Target)){if(h!="")b=Read(f.Target); // 사용자 설정 파일: 있으면 그대로
     else if(f.Target.Equals(Traits9CheckIniRel,StringComparison.OrdinalIgnoreCase)&&Hash(OldTraits9CheckIniRel)!="")b=Read(OldTraits9CheckIniRel);} // 정식판 뒤: 옛 이름의 사용자 설정(Notice 등)을 새 이름으로 이어받음
    else if(ModuleIni(f.Target)&&h!=""&&SameRegistration(Text(Read(f.Target)),Text(b))){
     // R43: 설치 기록(state.xml)이 없는데 모듈 ini 가 이미 우리 것(지금 자료와 같거나 Known)으로 있으면 '다른 패치 파일'로 막지 않고 받아들임
     //  (R43 시험1 → 시험2 갱신 때 Diag.ini 에서 멈춤: 합친 ini 의 해시가 디스크 파일과 달라 Plan 이 알 수 없는 파일로 봄)
     if(h==f.Sha||Known(h))accepted[f.Target]=h;
     b=Encoding.UTF8.GetBytes(MergeIni(Text(Read(f.Target)),Text(b)));
    }
    d[f.Target]=b;
   }catch(InstallException){throw;}catch(Exception e){throw new InstallException(Spec.Tag(f.Id),f.Target,e.Message);}
  }
  // R40 A22: 통합패치 바탕 모듈(SAN14Mods/Core·우리 리졸버)이 다른 프로그램에 의해 바뀌어 있으면, 보존하고 멈추는 대신 우리 것으로 되돌림
  // (그대로 두면 게임 확인 실패로 통합패치 전체가 안 켜짐 — 그 사용자는 영영 못 고침). 바뀐 파일은 설치 묶음 사본으로 보관되어 [되돌리기]가 가능.
  foreach(var f in M.Entries){
   if(f.Id!=0||!d.ContainsKey(f.Target))continue;
   Record q0;string h0=Hash(f.Target);
   // R42f: 옛 공용 리졸버 이름(CEKO)은 다른 모드도 같은 이름으로 깜 → 그 모드 것이 있으면 그대로 둠(우리 ModCore 는 IP01 을 씀). 없을 때만 우리 것을 깖.
   if(f.Target.Equals(SharedResolverRel,StringComparison.OrdinalIgnoreCase)){if(h0!=""&&h0!=f.Sha){d.Remove(f.Target);Log("다른 모드의 "+f.Target+" 은(는) 그대로 둡니다(통합패치는 자기 리졸버 SAN14_Resolver_IP01.dll 을 따로 씀).");}continue;}
   bool core=f.Target.StartsWith("SAN14Mods/Core/",StringComparison.OrdinalIgnoreCase)||f.Target.Equals(OurResolverRel,StringComparison.OrdinalIgnoreCase);
   if(core&&h0!=""&&h0!=f.Sha&&old.Items.TryGetValue(f.Target,out q0)&&h0!=q0.After&&h0!=q0.Before){accepted[f.Target]=h0;Log("공통 파일 "+f.Target+" 이(가) 다른 프로그램에 의해 바뀌어 있어 통합패치 것으로 되돌립니다(그대로면 통합패치 전체가 안 켜짐).");}
  }
  bool runtime=Spec.AnyRuntime(on);
  if(runtime){
   // 공용 로더 자리
   string slot=LoaderPlan();owner[slot]=0;
   if(Hash(slot)==""||old.Items.ContainsKey(slot)||LoaderOurs(slot))d[slot]=Payload("support/payload/loaders/"+slot,M.Values["loader_"+slot]);
   // 통합편의 설정
   string existing=Hash(SettingsRel)==""?"":Text(Read(SettingsRel));
   d[SettingsRel]=Encoding.UTF8.GetBytes(SettingsIni(existing,old.On,on));owner[SettingsRel]=1;
  }
  if(on[0]||on[24]){ // Patch27 설정(1번 설정 창 5·6쪽 담당): 25번이 꺼져 있으면 게임 기능 훅 없이 설정 쪽만
   var o=ParseIni(Hash(Patch27IniRel)==""?"":Text(Read(Patch27IniRel)));o["Features\tPatch27Features"]=on[24]?"1":"0";
   d[Patch27IniRel]=Encoding.UTF8.GetBytes(WriteIni(o,null));owner[Patch27IniRel]=on[24]?25:1;
  }
  if(on[Spec.Fog-1]&&(Hash(FogIniRel)!=""&&old.Items.ContainsKey(FogIniRel))){ // R24 가 만든 안개 설정 파일: 내용 그대로 유지(날씨는 게임 안 7쪽에서; 옛 Weather=0 은 첫 실행 때 그 값으로 이어받음)
   d[FogIniRel]=Read(FogIniRel);owner[FogIniRel]=Spec.Fog;
  }
  if(on[8]||on[10]||on[13]||on[16]){ // 모음팩 브리지 설정
   bool hover=on[13];
   if(on[13]&&(on[8]||on[12]))Log("14번: 9번/13번의 개성 9개 UI와 함께 — 개성 설명은 개성 9개 UI, 전법 설명은 14번이 띄웁니다.");
   string ini="[Mod]\r\nId=SAN14CE.ModPackBridge\r\nFile=SAN14_ModPackBridge.dll\r\nEnabled=1\r\nCoreABI=65536\r\n[ModPack]\r\nStats200=0\r\nTraitHover="+(hover?1:0)+"\r\nCollab="+(on[8]?1:0)+"\r\nBattleDeaths=0\r\nField3D="+(on[10]?1:0)+"\r\n";
   d[BridgeIniRel]=Encoding.UTF8.GetBytes(ini);owner[BridgeIniRel]=14;
  }
  if(on[7]){ // BGM 플레이어 설정: 있으면 그대로, 없을 때만 새로
   if(Hash(BgmIniRel)!="")d[BgmIniRel]=Read(BgmIniRel);else{
   string basePath=Files.Child(root,"San14BgmExpansion");
   string v="[BGM]\r\nCount=117\r\n[Manager]\r\nPython="+Path.Combine(basePath,"runtime","pythonw.exe")+"\r\nScript="+Path.Combine(basePath,"manager.py")+"\r\n";
   d[BgmIniRel]=Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes(v)).ToArray();}owner[BgmIniRel]=8;
  }
  leave.Clear();
  if(on[9])ModelPlan(old,on,d,owner,10);
  if(on[30])ModelPlan(old,on,d,owner,31);
  foreach(var x in Indexes){
   string r=x.Rel;Record q;if(d.ContainsKey(r))continue;
   if(!old.Items.TryGetValue(r,out q)){ // 설치 기록이 없는 색인: 옛 판(1.0.1) 레코드만 새 판 원본으로(자료가 없거나 읽지 못하면 그대로)
    string h0=Hash(r);if(h0=="")continue;byte[] rf=null;try{rf=RefreshOnly(r,Read(r));}catch(Exception){rf=null;}
    if(rf!=null){d[r]=rf;owner[r]=x.Id;accepted[r]=h0;Log(x.Id+"번: "+r+" 에 옛 게임판 원본 레코드가 남아 있어 새 판 원본으로 되돌립니다(다른 레코드는 그대로).");}
    continue;
   }
   string h=Hash(r);if(h==""){leave.Add(r);continue;}
   // 우리 레코드를 빼려면 합본 색인 자료(분할 ZIP 뒤쪽)가 필요. 자료가 없으면 색인은 두고 컨테이너만 지우는 일이 생기므로 전체를 멈춤
   byte[] cur=Read(r),un=null;try{un=ModelUnpatch(r,cur);}catch(Exception ex){throw new IOException(x.Id+"번을 빼려면 설치 자료(분할 ZIP 전체)가 필요합니다: "+r+"\r\n받은 ZIP 을 모두 같은 새 폴더에 풀고 다시 실행해 주세요. 게임 파일은 바꾸지 않았습니다.\r\n("+ex.Message+")");}
   if(un==null){leave.Add(r);Log(x.Id+"번: "+r+" 은(는) 다른 프로그램이 바꿔 두어 그대로 둡니다(우리 레코드를 빼지 못함).");continue;}
   if(Same(un,cur)){leave.Add(r);continue;}
   d[r]=un;owner[r]=x.Id;accepted[r]=h;
  }
  // 게임 데이터 파일 (18번·24번): 현재 파일이 원본/우리 변형이면 유지하거나 교체, 그 밖이면 보존
  foreach(string r in new[]{FixRel,ExpRel}){
   int id=r==FixRel?24:18;string h=Hash(r);
   if(h=="")throw new InstallException(on[id-1]?Spec.Tag(id):"게임 데이터",r,"게임 파일이 없습니다. Steam에서 게임 파일 무결성 확인을 먼저 해 주세요.");
   string orig=M.Values[r==FixRel?"fix_original":"exp_original"],ours=M.Values[r==FixRel?"fix_nostat":"exp_modified"];
   Record q;bool known=old.Items.TryGetValue(r,out q);
   bool ok=h==orig||h==ours||(known&&(h==q.After||h==q.Before))||Known(h);
   if(!ok&&r==FixRel){try{StatData.Normalize(StatData.Decode(Read(r)).Raw);ok=true;}catch{}}
   if(!ok)throw new InstallException(Spec.Tag(id),r,"현재 게임 데이터 파일이 원본도, 이 패치의 파일도 아닙니다(다른 수정 또는 다른 게임 버전). 보존했습니다. Steam 무결성 확인 뒤 다시 시도해 주세요.");
   accepted[r]=h;
   if(on[id-1]){
    // 게임 안 설정 창에서 끈 상태(원본으로 바뀜)는 존중: 현재가 원본이고 이미 설치돼 있던 항목이면 그대로 둠
    if(old.On[id-1]&&h==orig&&old.Version==7){d[r]=Read(r);}
    else if(h==ours)d[r]=Read(r);
    else d[r]=Payload(r==FixRel?"data/fixdata_nostat_101.s14":"support/payload/f18/experience_data_exce.bin",ours);
   }
   else if(h!=orig&&(known||h==ours||Known(h)))d[r]=DataOriginal(r);
  }
  // 글꼴(20번)과 BGM 공유 파일: 현재 RRPreview.rdb 는 원본/우리 글꼴판/기록된 것만 덮어씀 (R39: 설치기 알림창에서 '백업 후 그래도 설치'를 고르면 ForceFont)
  if(on[19]){
   string h=Hash(FontRel);
   if(h=="")throw new InstallException(Spec.Tag(20),FontRel,"게임 0002 폴더의 RRPreview.rdb가 없습니다. Steam 무결성 확인을 먼저 해 주세요.");
   bool ok=!ForeignFontHash(old,h);
   if(!ok&&!ForceFont)throw new InstallException(Spec.Tag(20),FontRel,"RRPreview.rdb가 원본도 이 패치의 글꼴판도 아닙니다(BGM 적용 등 다른 변경). 보존했습니다. 20번을 끄고 설치하거나 원본으로 되돌린 뒤 다시 시도해 주세요.");
   if(!ok)Log("20번: 다른 글꼴 파일을 백업하고 덮어씁니다(제거하면 그 파일로 되돌림).");
   accepted[FontRel]=h;
  }
  return d;
 }
 bool ForeignFontHash(State old,string h){Record q;if(h=="")return false;return !(h==M.Values["font_original"]||h==M.Values["font_rdb"]||(old.Items.TryGetValue(FontRel,out q)&&(h==q.After||h==q.Before))||Known(h));}
 // 설치기 충돌 검사용: 지금 게임 폴더의 글꼴 파일이 원본도 우리 것도 아닌가
 public bool IsForeignFont(){try{return ForeignFontHash(Load(),Hash(FontRel));}catch(Exception){return false;}}
 // R35 10번 추가 모델 / R39 31번 주메뉴 그래픽: 색인이 1.0.1 원본/우리 합본/기록된 것이고 새 파일 자리가 비어 있을 때만 설치. 아니면 그 번호의 레코드 자료만 건너뛰고 나머지 설치는 계속(다른 모드 파일 보존)
 void ModelPlan(State old,bool[] on,Dictionary<string,byte[]> d,Dictionary<string,int> owner,int id){
  var list=M.Entries.Where(f=>f.Applies(on)&&ModelData(f)&&f.Id==id).ToList();if(list.Count==0)return;
  string label=id==10?"10번 추가 모델(25~173)":"31번 주메뉴 그래픽";
  string why=null;bool missing=false;
  foreach(var f in list)if(!PayloadPresent(f.Source)){why=(id==10?"추가 모델 자료 ZIP 이 없음(":"그래픽 자료 ZIP 이 없음(")+f.Source+(list.Count(x=>!PayloadPresent(x.Source))>1?" 외":"")+")";missing=true;break;}
  if(why==null)foreach(var f in list){
   string h=Hash(f.Target);Record q;bool rec=old.Items.TryGetValue(f.Target,out q);
   if(IsModelIndex(f.Target)){
    if(h==""){why=f.Target+" 파일이 없음(Steam 무결성 확인 필요)";break;}
    if(ModelPatch(f.Target,Read(f.Target))==null){why=f.Target+" 의 같은 레코드를 다른 모드가 바꿔 둠";break;}
   }else if(!(h==""||h==f.Sha||(rec&&h==q.After)||Known(h))){why=f.Target+" 자리에 이 패치가 아닌 파일이 있음";break;}
  }
  if(why!=null){
   // 자료 ZIP 만 빠진 갱신: 이미 깔린 것은 그대로 둠
   if(missing){var keep=old.Items.Values.Where(q=>list.Any(f=>f.Target.Equals(q.Path,StringComparison.OrdinalIgnoreCase))).ToList();
    if(keep.Count>0&&keep.All(q=>Hash(q.Path)==q.After)){foreach(var q in keep){d[q.Path]=Read(q.Path);owner[q.Path]=id;if(IsModelIndex(q.Path))accepted[q.Path]=q.After;}if(id==10)ModelInstalled=true;else ScreenInstalled=true;Log(label+": 자료 ZIP 이 없어 이미 설치된 것을 그대로 둡니다.");return;}}
   if(id==10){ModelSkip=why;Log("10번 추가 모델(25~173): "+why+" → 추가 모델은 설치하지 않았습니다. 고유 무장 등록 기능과 기본 모델(0~24)은 그대로입니다.");}
   else{ScreenSkip=why;Log("31번 주메뉴 그래픽: "+why+" → 그래픽은 설치하지 않았습니다(다른 파일은 바꾸지 않음).");}
   return;
  }
  foreach(var f in list){
   try{d[f.Target]=IsModelIndex(f.Target)?ModelPatch(f.Target,Read(f.Target)):Payload(f.Source,f.Sha);}catch(Exception e){throw new InstallException(Spec.Tag(id),f.Target,e.Message);}
   owner[f.Target]=id;if(IsModelIndex(f.Target))accepted[f.Target]=Hash(f.Target);
  }
  if(id==10)ModelInstalled=true;else ScreenInstalled=true;
 }
 // 무장→모델 연결 파일 위치: IC 와 같은 규칙(게임 폴더 경로 UTF-16, A~Z 소문자·'/'→'\\', FNV-1a 64)
 public static string LinkKey(string gameRoot){
  string p=Path.GetFullPath(gameRoot).TrimEnd('\\','/');ulong h=0xcbf29ce484222325UL;
  foreach(char ch in p){uint c=ch;if(c=='/')c='\\';else if(c>='A'&&c<='Z')c+=0x20;h^=c;h*=0x100000001b3UL;}
  return h.ToString("X16");
 }
 public static string LinksDir(string gameRoot){string lad=Environment.GetEnvironmentVariable("LOCALAPPDATA");if(String.IsNullOrEmpty(lad))return null;return Path.Combine(Path.Combine(lad,"San14ModelRegistry"),LinkKey(gameRoot));}
 // 옛 통합본의 무장 연결 115명을 한 번만 반영(같은 무장은 옛 연결 우선, 그 밖의 기존 줄 유지). 그 뒤 게임 안에서 바꾼 연결은 다시 덮지 않음
 void ApplyLinks(){
  try{
   string dir=LinksDir(root);if(dir==null){Log("10번: LOCALAPPDATA 를 찾지 못해 무장 연결을 반영하지 않았습니다.");return;}
   string marker=Path.Combine(dir,LinksMarker),path=Path.Combine(dir,"OfficerModels.tsv");
   if(File.Exists(marker)){Log("10번: 무장 연결은 이전 설치 때 반영했습니다(게임 안에서 바꾼 연결 유지).");return;}
   var rows=new SortedDictionary<int,string>();Func<string,int> parse=delegate(string text){int n=0;foreach(string raw in text.Replace("\r\n","\n").Split('\n')){string line=raw.TrimStart('\uFEFF').TrimEnd('\r');var c=line.Split('\t');int id,model;if(c.Length<3||!Int32.TryParse(c[0],out id)||!Int32.TryParse(c[1],out model))continue;rows[id]=id+"\t"+model+"\t"+c[2];n++;}return n;};
   int had=0;if(File.Exists(path)){had=parse(Text(File.ReadAllBytes(path)));string bak=path+".R35설치전.bak";if(!File.Exists(bak))File.Copy(path,bak);}
   int ours=parse(Text(Payload(LinksPayload,M.Values["officer_links"])));
   var b=new StringBuilder("SAN14_OFFICER_MODELS_V1\n");foreach(var kv in rows)b.Append(kv.Value+"\n");
   Directory.CreateDirectory(dir);Files.Atomic(path,Encoding.UTF8.GetPreamble().Concat(Encoding.UTF8.GetBytes(b.ToString())).ToArray());
   File.WriteAllText(marker,"R35 설치기가 옛 통합본 무장 연결 "+ours+"명을 반영함 ("+DateTime.Now.ToString("yyyy-MM-dd HH:mm")+"). 이 파일을 지우면 다음 설치 때 다시 반영합니다.\r\n",Encoding.UTF8);
   Log("10번: 무장 연결 "+ours+"명 반영(기존 "+had+"줄, 같은 무장은 옛 통합본 연결 우선) → "+path);
  }catch(Exception e){Log("10번: 무장 연결 반영 중 문제(설치는 유지): "+e.Message);}
 }
 string LoaderPlan(){
  foreach(string n in LoaderNames)if(LoaderOurs(n))return n;
  foreach(string n in LoaderNames)if(Hash(n)=="")return n;
  throw new IOException("게임 폴더의 공용 로더 자리(dxgi.dll / version.dll / dinput8.dll)가 모두 다른 패치에서 사용 중입니다. 기존 파일은 보존했습니다.");
 }
 bool LoaderOurs(string n){string h=Hash(n);if(h=="")return false;string note;return M.Known.TryGetValue(h,out note)&&note.StartsWith("loader");}
 // 구역 없는 'Key=값' 설정 파일(안개 SAN14_FogAI.ini)에서 한 키만 바꾸거나 더함. 다른 줄·주석은 그대로
 public static string SetFlatKey(string text,string key,string value){
  var lines=new List<string>((text??"").Replace("\r\n","\n").Split('\n'));if(lines.Count>0&&lines[lines.Count-1]=="")lines.RemoveAt(lines.Count-1);
  bool done=false;
  for(int i=0;i<lines.Count;i++){string t=lines[i].Trim().TrimStart('\uFEFF');int eq=t.IndexOf('=');if(eq>0&&!t.StartsWith(";")&&t.Substring(0,eq).Trim().Equals(key,StringComparison.OrdinalIgnoreCase)){if(!done){lines[i]=key+"="+value;done=true;}else{lines.RemoveAt(i);i--;}}}
  if(!done)lines.Add(key+"="+value);
  return String.Join("\r\n",lines.ToArray())+"\r\n";
 }
 static bool SameRegistration(string current,string wanted){var a=ParseIni(current);var b=ParseIni(wanted);string ai,bi;return a.TryGetValue("Mod\tId",out ai)&&b.TryGetValue("Mod\tId",out bi)&&ai==bi;}
 public static Dictionary<string,string> ParseIni(string text){var d=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);string sec="";foreach(var raw in text.Split('\n')){string v=raw.Trim().TrimStart('﻿');if(v.StartsWith("[")&&v.EndsWith("]")){sec=v.Substring(1,v.Length-2);continue;}int eq=v.IndexOf('=');if(eq>0&&!v.StartsWith(";")&&!v.StartsWith("#"))d[sec+"\t"+v.Substring(0,eq).Trim()]=v.Substring(eq+1).Trim();}return d;}
 public static string WriteIni(Dictionary<string,string> d,string header){
  var b=new StringBuilder();if(header!=null)b.Append(header+"\r\n");string sec=null;
  foreach(var p in d.OrderBy(x=>x.Key,StringComparer.OrdinalIgnoreCase)){int i=p.Key.IndexOf('\t');string s=p.Key.Substring(0,i);if(sec!=s){sec=s;b.Append("["+s+"]\r\n");}b.Append(p.Key.Substring(i+1)+"="+p.Value+"\r\n");}
  return b.ToString();
 }
 // 모듈 등록 파일: 우리 값(Mod 섹션)은 새 자료대로, 그 밖의 사용자 값은 유지
 public static string MergeIni(string current,string wanted){var c=ParseIni(current);var w=ParseIni(wanted);foreach(var kv in w)c[kv.Key]=kv.Value;string first=wanted.Split('\n')[0].Trim();return WriteIni(c,first.StartsWith(";")?first:null);}
 // 통합편의 설정: 켜는 항목은 기존 값 유지(처음 켤 때만 기본값), 끄는 항목은 0 (AutoFocus 는 1)
 public static string SettingsIni(string existing,bool[] oldOn,bool[] on){
  var d=ParseIni(existing??"");
  foreach(var kv in Spec.IniKeys){
   int id=kv.Key;bool wasOn=oldOn!=null&&oldOn[id-1];
   foreach(string spec in kv.Value){
    int eq=spec.IndexOf('=');string key=spec.Substring(0,eq),val=spec.Substring(eq+1);
    bool focus=key=="Camera\tAutoFocus";
    if(on[id-1]){string cur;if(!(wasOn&&d.TryGetValue(key,out cur)&&cur!="9"))d[key]=val;} // 이미 켜져 있던 항목은 게임 안 설정 창에서 바꾼 값을 그대로 둠
    else d[key]=focus?"1":"0"; // 3번을 끄면 AudienceGate=0(원래 접견)·AudienceButton=0(외교 메뉴 버튼 숨김)
   }
  }
  d["Convenience\tDualCredit"]="0";
  if(on[0])d["Modules\tNewSettingsWindow"]="1"; // R39 새 설정 창. R43 11-08: 옛 창 선택 없앰(늘 1)
  if(on[12]&&on[1])d["Convenience\tAutoCredit"]="0"; // 13번과 같이 쓰면 2번 자동 논공은 끔(설치기 알림창의 '자동 논공만 끄고 둘 다 설치')
  string gate;if(d.TryGetValue("Convenience\tAudienceGate",out gate)&&gate=="9")d["Convenience\tAudienceGate"]="0"; // R21 의 '9'(숨김 표시) 정리
  d["Modules\tTraits9Module"]=on[8]&&!on[12]?"1":"0";
  if(!d.ContainsKey("BgmPlayer\tEnabled")){d["BgmPlayer\tEnabled"]="0";d["BgmPlayer\tSelected"]="117";}
  return WriteIni(d,"; SAN14CE 통합패치 "+Spec.Version+" (공식 "+Spec.GameLabel+") — 이 파일은 설치기와 게임 안 모드 설정 창이 관리합니다.");
 }

 // ---------- 작업 계획 ----------
 public List<Record> Plan(State old,bool[] on,Dictionary<string,byte[]> desired,bool remove){
  var next=new State{On=(bool[])on.Clone()};var ops=new List<Record>();
  foreach(var pair in desired.OrderBy(x=>x.Key,StringComparer.OrdinalIgnoreCase)){
   string r=pair.Key,h=Hash(r),a=Tx.Put(pair.Value);Record q;bool known=old.Items.TryGetValue(r,out q);
   string before;
   if(IsData(r))before=Tx.Put(DataOriginal(r));
   else if(IsModelIndex(r))before=known?q.Before:Tx.Snapshot(r);
   else if(known)before=q.Before;
   else if(h=="")before="";
   else if(!Mutable(r)&&h==a&&ForeignKit(r))before=Tx.Snapshot(r); // R39: 남의 키트(26·27·28·30)를 따로 깔아 둔 사용자 → 같은 파일이어도 제거 때 그 사용자 것으로 되돌림(지우지 않음)
   else if(!Mutable(r)&&(h==a||Known(h)))before=""; // 기록 없이 남아 있던 이 패치(옛 판 포함)의 파일: 제거 때 지움 (다른 모듈이 공용 파일을 쓰면 아래 규칙으로 보존)
   else if(Mutable(r)||accepted.ContainsKey(r)&&accepted[r]==h)before=Tx.Snapshot(r);
   else throw new InstallException(FeatureOf(r),r,"다른 패치의 파일이 같은 자리에 있습니다(알 수 없는 내용). 보존했습니다.");
   if(known&&h!=""&&h!=q.After&&h!=q.Before&&!Mutable(r)&&!Known(h)&&!(accepted.ContainsKey(r)&&accepted[r]==h))throw new InstallException(FeatureOf(r),r,"이 패치가 설치한 뒤 다른 프로그램이 바꾼 파일입니다. 보존했습니다.");
   if(!(IsData(r)&&a==before))next.Items[r]=new Record{Path=r,Before=before,After=a,Kind=IsBase(r)?"shared":"owned"};
   if(h!=a)ops.Add(new Record{Path=r,Before=Tx.Snapshot(r),After=a});
  }
  // 다른 패치의 모듈 등록이 남아 있으면 공용 런타임(로더·ModCore·리졸버)은 지우지 않음
  string modulesDir=Files.Child(root,"SAN14Mods/modules");
  bool otherModules=Directory.Exists(modulesDir)&&Directory.GetFiles(modulesDir,"*.ini").Any(f=>{string rel="SAN14Mods/modules/"+Path.GetFileName(f);return !old.Items.ContainsKey(rel)&&!desired.ContainsKey(rel);});
  foreach(var q in old.Items.Values.Where(x=>!desired.ContainsKey(x.Path))){
   string r=q.Path,h=Hash(r);
   if(leave.Contains(r))continue;
   if(old.LostBackup.Contains(r)&&h!=q.Before){Log("원래 파일 백업이 없어져 되돌릴 수 없으므로 그대로 둡니다: "+r);continue;}
   if(q.Kind=="shared"&&otherModules&&q.Before==""){Log("다른 모듈이 함께 쓰는 공용 파일은 남겨 둡니다: "+r);continue;}
   if(IsData(r)){string after=Tx.Put(DataOriginal(r));if(h!=after){if(h!=q.After&&h!=q.Before&&!Known(h)&&!(accepted.ContainsKey(r)&&accepted[r]==h))throw new InstallException(FeatureOf(r),r,"게임 데이터 파일이 알 수 없게 바뀌어 있습니다. 보존했습니다.");ops.Add(new Record{Path=r,Before=Tx.Snapshot(r),After=after});}continue;}
   if(h==q.Before)continue;
   if(h==""){continue;} // 이미 지워진 파일: 기록만 정리
   if(q.Kind=="shared"&&h!=q.After)continue; // 다른 패치가 쓰는 공용 파일은 건드리지 않음
   if(h!=q.After&&!Mutable(r)&&!Known(h))throw new InstallException(FeatureOf(r),r,"제거할 파일이 이 패치가 아닌 다른 프로그램에 의해 바뀌어 있습니다. 보존했습니다.");
   if(Mutable(r)){string saved=".san14ce_compat_r3/user_settings/"+h+"_"+Path.GetFileName(r);if(Hash(saved)=="")ops.Add(new Record{Path=saved,Before="",After=Tx.Snapshot(r)});}
   ops.Add(new Record{Path=r,Before=Tx.Snapshot(r),After=q.Before});
  }
  // 정식판 뒤(11-08 갤 제보 '9칸 안 깔았는데 9칸 오류'): 설치 기록 없이 남은 옛 판 통합패치 파일 정리.
  //  통합편의는 게임 폴더에 SAN14_Traits9.dll 이 있기만 하면 불러서, 기록이 사라진(기록 폴더 이름 바꾸기·재설치 등) 옛 9번 파일이 안 고른 9번 오류를 냈음.
  //  대상: 안 고른 항목의 설치 대상 + 지금 판에서 빠진 파일 중 설치 기록에 없는 것.
  //   · 모듈 등록 파일(SAN14Mods/modules/*.ini): 등록 Id 가 우리 것(SAN14CE.)이면 지움(옛 이름 Traits9Check.ini 처럼 두 번 등록되는 것도 막음)
  //   · 그 밖: 해시가 '알려진 옛 판'(Known)이거나 지금 판 그 자리 파일과 같을 때 지움(R43 8차). 모르는 파일·남의 키트 자리·공용 런타임·게임 자료·사용자 설정은 그대로.
  //  지우는 파일은 .san14ce_compat_r3/leftover/ 에 사본을 남김.
  if(!remove){
   var cand=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
   foreach(var f in M.Entries)if(!desired.ContainsKey(f.Target))cand.Add(f.Target);
   foreach(var r0 in RetiredTargets)if(!desired.ContainsKey(r0))cand.Add(r0);
   if(!desired.ContainsKey(BridgeIniRel))cand.Add(BridgeIniRel);   // 설치기가 만드는 모음팩 연결 등록 파일(manifest 에 없음)
   foreach(var r in cand.OrderBy(x=>x,StringComparer.OrdinalIgnoreCase)){
    if(old.Items.ContainsKey(r)||leave.Contains(r)||IsData(r)||IsModelIndex(r)||ForeignKit(r)||IsBase(r))continue;
    string h=Hash(r);if(h=="")continue;
    bool modIni=r.StartsWith("SAN14Mods/modules/",StringComparison.OrdinalIgnoreCase)&&r.EndsWith(".ini",StringComparison.OrdinalIgnoreCase);
    if(modIni){string id;try{var c=ParseIni(Text(Read(r)));if(!c.TryGetValue("Mod\tId",out id)||!OurModuleIds.Contains(id))continue;}catch(Exception){continue;}}   // 남의 모드도 SAN14CE. 를 씀(예: 기색 SAN14CE.Condition) → 우리 Id 목록만
    else{if(Mutable(r))continue;bool cur=M.Entries.Any(f=>f.Target.Equals(r,StringComparison.OrdinalIgnoreCase)&&f.Sha==h);if(!cur&&!Known(h))continue;}   // R43 8차: 지금 판과 같은 파일도 정리(안 고른 항목인데 기록만 사라진 것 — 남으면 통합편의가 9번 본체를 실어 14번과 엉킴). 모르는 내용(남의 모드)은 그대로
    string saved=".san14ce_compat_r3/leftover/"+h.Substring(0,16)+"_"+Path.GetFileName(r);
    if(Hash(saved)=="")ops.Add(new Record{Path=saved,Before="",After=Tx.Snapshot(r)});
    ops.Add(new Record{Path=r,Before=Tx.Snapshot(r),After=""});
    Log("설치 기록 없이 남아 있던 옛 판 통합패치 파일을 정리합니다(사본 "+saved+"): "+r);
   }
  }
  string sb=Tx.Snapshot(StateRel),sa=remove?"":Tx.Put(next.Bytes());if(sb!=sa)ops.Add(new Record{Path=StateRel,Before=sb,After=sa});
  return ops;
 }
 // 지금 판에서 빠졌거나 이름이 바뀐 설치 대상(남아 있으면 위 정리 대상)
 static readonly string[] RetiredTargets={"SAN14Mods/ModPack/SAN14_Stats200.dll","SAN14_Stats200Reader.dll","SAN14Mods/modules/Stats200Reader.ini",OldTraits9CheckIniRel,"SAN14Mods/modules/ExtBridgeTest.ini"};
 // 통합패치 모듈 등록 Id(지금·옛 판 전부 + 설치기가 만드는 것 + 옛 시험 키트)
 static readonly HashSet<string> OurModuleIds=new HashSet<string>(StringComparer.Ordinal){"SAN14CE.AutoMerit","SAN14CE.AutoPersonnelRepair","SAN14CE.CityCaps","SAN14CE.CrashLog","SAN14CE.Diag","SAN14CE.DomesticAI","SAN14CE.DomesticAutomation","SAN14CE.DomesticPlayer","SAN14CE.DomesticTalent","SAN14CE.DragGive","SAN14CE.DragLearn","SAN14CE.ExecuteLock","SAN14CE.ExtBridge","SAN14CE.ExtBridgeTest","SAN14CE.FPS120","SAN14CE.HiredFace","SAN14CE.ICGuard","SAN14CE.IntegratedConvenience.Compat","SAN14CE.ItemPicture","SAN14CE.MeritTraits","SAN14CE.ModPackBridge","SAN14CE.ModSettings","SAN14CE.Patch27","SAN14CE.PrisonerReturn","SAN14CE.Range","SAN14CE.RegionOfficer","SAN14CE.RelatedOfficersTab","SAN14CE.SceneSkip","SAN14CE.SettingsHelpScenic","SAN14CE.SortiePresets","SAN14CE.Stats200New","SAN14CE.Stats200Reader","SAN14CE.StatusColor","SAN14CE.Traits9Check","SAN14CE.WheelRegion"};
 bool ForeignKit(string r){foreach(var f in M.Entries)if(f.Target.Equals(r,StringComparison.OrdinalIgnoreCase)&&(f.Id==26||f.Id==27||f.Id==28||f.Id==30))return true;return false;}
 bool IsBase(string r){if(LoaderNames.Any(n=>n.Equals(r,StringComparison.OrdinalIgnoreCase)))return true;foreach(var f in M.Entries)if(f.Id==0&&f.Target.Equals(r,StringComparison.OrdinalIgnoreCase))return true;return false;}
 string FeatureOf(string r){foreach(var f in M.Entries)if(f.Target.Equals(r,StringComparison.OrdinalIgnoreCase))return f.Id==0?"공통 런타임":Spec.Tag(f.Id);if(r.Equals(SettingsRel,StringComparison.OrdinalIgnoreCase))return "공통 런타임 설정";if(IsData(r))return r==FixRel?Spec.Tag(24):Spec.Tag(18);return "";}

 // R39 설치기: 체크 = 선택. 선택 설치 = 현재 설치 ∪ 선택, 선택 제거 = 현재 설치 − 선택 → 그 결과 집합을 Install 에 넘김
 public static bool[] Compose(bool[] current,bool[] selected,bool install){var w=(bool[])current.Clone();for(int i=0;i<Spec.Count;i++)if(selected[i])w[i]=install;return w;}
 internal static bool KeepRetired=false; // 시험 전용(옛 판 설치 흉내)
 static bool[] Clean(bool[] on){var c=(bool[])on.Clone();if(!KeepRetired)c[Spec.Fog-1]=false;return Spec.WithSettingsWindow(c);} // R41: 삭제된 12번은 어떤 경우에도 설치하지 않음. R43 11-08: 1번 잠금
 public int Install(bool[] on,Action<int,int> progress){
  on=Clean(on);ValidateGame();{var miss=MissingPayloads(on);if(miss.Count>0)throw new IOException(Spec.MissingText(miss));}Tx.Recover();if(Tx.RecoverNote!="")Log(Tx.RecoverNote);var old=Load();
  if(old.RetiredOn)Log("12번(필드 안개)은 R41 부터 통합패치에서 빠졌습니다. 이번 설치에서 그 파일을 지웁니다(미니맵은 27번).");
  if(old.Version!=0&&old.Version!=7)Log("이전 설치기(R20 이하) 기록을 이어받았습니다. 없어진 항목(옛 3·10·26번)과 옛 파일은 이번 설치에서 정리합니다.");
  var desired=Desired(old,on);var ops=Plan(old,on,desired,!on.Any(x=>x));
  GameClosed();int total=ops.Count;if(progress!=null)Tx.AfterWrite=delegate(int i){progress(i,total);};
  Tx.Commit(ops);
  if(on[7])PrepareBgmFolders();
  if(on[9]&&ModelInstalled)ApplyLinks();
  Log(on.Any(x=>x)?"설치 완료: "+ops.Count+"개 파일 처리.":"제거 완료: "+ops.Count+"개 파일 처리.");
  return ops.Count;
 }
 // 8번 BGM: 음원 관리기가 쓰는 폴더(library·live·jobs)를 만들고 현재 사용자에게 쓰기 권한을 준다 (옛 설치기와 같은 방식, 실패해도 설치는 유지)
 void PrepareBgmFolders(){
  try{
   string baseDir=Files.Child(root,"San14BgmExpansion");
   foreach(string sub in new[]{"library","live","jobs"})Directory.CreateDirectory(Path.Combine(baseDir,sub));
   if(Environment.OSVersion.Platform!=PlatformID.Win32NT)return;
   string user=Environment.UserDomainName+"\\"+Environment.UserName;
   var psi=new ProcessStartInfo(Path.Combine(Environment.SystemDirectory,"icacls.exe"),"\""+baseDir+"\" /grant:r \""+user+":(OI)(CI)M\" /inheritance:e"){UseShellExecute=false,CreateNoWindow=true};
   using(var p=Process.Start(psi)){if(!p.WaitForExit(30000)||p.ExitCode!=0)Log("8번: BGM 폴더 쓰기 권한 설정을 확인하지 못했습니다(음원 추가가 안 되면 게임 폴더 권한을 확인해 주세요).");}
  }catch(Exception e){Log("8번: BGM 폴더 준비 중 문제: "+e.Message);}
 }
 public void Remove(){Install(new bool[Spec.Count],null);}
 // 미리 보기: 바뀔 파일 목록 (실제 변경 없음)
 public List<Record> Preview(bool[] on){on=Clean(on);Tx.Recover();if(Tx.RecoverNote!="")Log(Tx.RecoverNote);var old=Load();return Plan(old,on,Desired(old,on),!on.Any(x=>x));}

 // ---------- 상태 표시 ----------
 // ---------- 게임 적용 여부 ----------
 // R39: 모든 번호가 게임 폴더\SAN14_기록\적용확인.log 에 공통 형식으로 기록(기록형식.md). 줄이 없는 옛 번호는 runtime.log·통합편의 기록으로 판정(이전 방식).
 public sealed class AppliedInfo{
  public string State="NONE";   // OK / PART / FAIL / OFF / NONE(실행 기록 없음) / PRE(게임 실행 전) / FILE(파일만)
  public string Cell="";        // 목록 칸 글
  public string Module="",Time="",Sites="",Settings="",Reason="";
  public List<string[]> Features=new List<string[]>(); // PART/OK 세부: {이름, OK|FAIL|OFF, 사유}
  public List<string[]> Counters=new List<string[]>(); // {카운터, 값, 시각}
  public List<string> Files=new List<string>();        // 설치 파일 확인 결과 줄
  public List<string[]> Fails=new List<string[]>();    // R43F 실패 보고: {시각, 모듈, 단계, 어디서, 기대, 실제, 영향, 코드, 게임}
  public string Detail(){
   var b=new StringBuilder();
   b.AppendLine(State=="OK"?"적용 확인 ✔":State=="PART"?"일부 적용 ⚠":State=="FAIL"?"실패 ✖":State=="OFF"?"설정으로 꺼짐":State=="PRE"?"게임 실행 전":State=="FILE"?"파일 적용 ✔":"실행 기록 없음");
   if(Files.Count>0){b.AppendLine("■ 설치 파일");foreach(string f in Files)b.AppendLine("  "+f);}
   if(Time!="")b.AppendLine("■ 게임에서 불러옴: "+Time+(Module!=""?" ("+Module+")":""));
   if(Sites!="")b.AppendLine("■ 고친 자리: "+Sites);
   if(Settings!="")b.AppendLine("■ 현재 설정: "+Settings.Replace(";"," · "));
   if(Features.Count>0){b.AppendLine("■ 기능별");foreach(var f in Features)b.AppendLine("  "+(f[1]=="OK"?"✔":f[1]=="OFF"?"－":"✖")+" "+f[0]+(f[2]!=""&&f[2]!="-"?" — "+f[2]:""));}
   if(Reason!=""&&Reason!="-")b.AppendLine("■ 이유: "+Reason);
   if(Fails.Count>0){b.AppendLine("■ 실패 보고(게임 안 오류창과 같은 내용, R43F)");foreach(var f in Fails){b.AppendLine("  ✖ "+f[1]+" · 단계 "+f[2]+" · 코드 "+f[7]+" · "+f[0]);b.AppendLine("     어디서: "+f[3]);b.AppendLine("     기대: "+f[4]);b.AppendLine("     실제: "+f[5]);b.AppendLine("     영향: "+f[6]);if(f[8]!="")b.AppendLine("     게임 실행 파일: "+f[8]);}}
   if(Counters.Count>0){b.AppendLine("■ 실제 동작");foreach(var c in Counters)b.AppendLine("  "+c[0]+" "+c[1]+"회");}
   return b.ToString();
  }
 }
 public static readonly string[] CollectLogs={"SAN14_CrashLog.log","SAN14CE_11Patch.log","SAN14_DomesticCE.log","SAN14_Traits9_UI.log","SAN14_Stats200.log","SAN14_TraitHover.log","SAN14_FogTrial.log","FieldLeader3D_Loader.log","FieldLeader3D_Runtime.log","SAN14_ModSettings.log","SAN14_ExtBridge.log","SAN14_HiredFace.log","SAN14_Range.log","SAN14_RegionOfficer.log","SAN14_StatusColor.log"};
 public static string RuntimeLogPath(){return Path.Combine(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"SAN14CE_ModCore"),"runtime.log");}
 // 적용확인.log 파싱: 번호별 마지막 시작 줄 + 그 뒤 카운터
 public static Dictionary<int,AppliedInfo> ParseApplyLog(string text){
  var r=new Dictionary<int,AppliedInfo>();if(String.IsNullOrEmpty(text))return r;
  foreach(string raw in text.Split('\n')){
   string line=raw.TrimEnd('\r');var c=line.Split('\t');int id;
   if(c.Length>=8&&c[0]=="R39"&&Int32.TryParse(c[1],out id)&&id>=1&&id<=Spec.Count){
    var a=new AppliedInfo{Module=c[2],Time=c[3],State=c[4],Sites=c[5],Settings=c[6],Reason=c[7]};
    if(a.State!="OK"&&a.State!="PART"&&a.State!="FAIL"&&a.State!="OFF")a.State="FAIL";
    if(a.Reason.Contains(":OK")||a.Reason.Contains(":FAIL")||a.Reason.Contains(":OFF"))foreach(string part in a.Reason.Split(';')){var p=part.Split(new[]{':'},3);if(p.Length>=2)a.Features.Add(new[]{p[0],p[1],p.Length>2?p[2]:""});}
    r[id]=a;
   }else if(c.Length>=5&&c[0]=="R39C"&&Int32.TryParse(c[1],out id)&&r.ContainsKey(id)){
    var a=r[id];var ex=a.Counters.FirstOrDefault(x=>x[0]==c[3]);if(ex!=null)a.Counters.Remove(ex);a.Counters.Add(new[]{c[3],c[4],c.Length>5?c[5]:""});
   }
  }
  return r;
 }
 // R43F 실패 보고 줄: R43F\t번호\t모듈\t시각\t단계\t어디\t기대\t실제\t영향\t코드\t게임 — 번호별(0=공통 → 1번에 붙임), 시각 ≥ since 만
 public static Dictionary<int,List<string[]>> ParseFails(string text,DateTime since){
  var r=new Dictionary<int,List<string[]>>();if(String.IsNullOrEmpty(text))return r;
  foreach(string raw in text.Split('\n')){
   var c=raw.TrimEnd('\r').Split('\t');int id;DateTime t;
   if(c.Length<10||c[0]!="R43F"||!Int32.TryParse(c[1],out id))continue;
   if(!DateTime.TryParseExact(c[3],"yyyy-MM-dd HH:mm:ss",System.Globalization.CultureInfo.InvariantCulture,System.Globalization.DateTimeStyles.None,out t)||t<since)continue;
   if(id<1||id>Spec.Count)id=1;
   if(!r.ContainsKey(id))r[id]=new List<string[]>();
   r[id].Add(new[]{c[3],c[2],c[4],c[5],c[6],c[7],c[8],c[9],c.Length>10?c[10]:""});
  }
  return r;
 }
 // R43D 종합 진단 마지막 줄: {시각, 단계(check30/check90), 요약, 문제들...}
 public static string[] LastDiag(string text){
  string[] r=null;if(String.IsNullOrEmpty(text))return null;
  foreach(string raw in text.Split('\n')){var c=raw.TrimEnd('\r').Split('\t');if(c.Length>=6&&c[0]=="R43D"){var l=new List<string>{c[3],c[4],c[5]};for(int i=6;i<c.Length;i++)l.Add(c[i]);r=l.ToArray();}}
  return r;
 }
 public AppliedInfo[] AppliedDetail(State s,string runtimeLog){
  var res=new AppliedInfo[Spec.Count];for(int i=0;i<Spec.Count;i++)res[i]=new AppliedInfo();
  string statePath=Files.Child(root,StateRel);DateTime installed=File.Exists(statePath)?File.GetLastWriteTime(statePath):DateTime.MinValue;
  Dictionary<int,AppliedInfo> rec=new Dictionary<int,AppliedInfo>();DateTime recTime=DateTime.MinValue;Dictionary<int,List<string[]>> fails=new Dictionary<int,List<string[]>>();
  try{string ap=Files.Child(root,ApplyLogRel);if(File.Exists(ap)){recTime=File.GetLastWriteTime(ap);string txt=TailText(ap,false);rec=ParseApplyLog(txt);fails=ParseFails(txt,LastRunStart(txt,installed));}}catch(Exception){rec=new Dictionary<int,AppliedInfo>();}
  string[] legacy=Applied(s,runtimeLog);
  // R40 A22: 마지막 실행에서 통합패치 전체가 멈춘 경우(게임 확인 실패·공통 파일 없음·exe 바뀜·다른 폴더) → 항목마다 '시작 기록 없음' 대신 진짜 이유
  string[] whole=WholeStop(s,runtimeLog,installed);
  for(int i=0;i<Spec.Count;i++){
   int id=i+1;var a=res[i];if(!s.On[i]){a.State="";a.Cell="";continue;}
   if(fails.ContainsKey(id)&&recTime>installed)a.Fails=fails[id];
   if(whole!=null&&id!=18&&id!=20&&id!=24&&id!=31){
    int nf0=0,bad0=0;foreach(var f in M.Entries){if(f.Id!=id||!f.Applies(s.On)||ModelData(f)&&IsModelIndex(f.Target))continue;Record q;string h=Hash(f.Target);if(Mutable(f.Target)){nf0++;if(h=="")bad0++;continue;}nf0++;if(s.Items.TryGetValue(f.Target,out q)){if(h!=q.After)bad0++;}else if(h!=f.Sha)bad0++;}
    if(nf0>0)a.Files.Add(bad0==0?nf0+"개 모두 정상 (판별 번호 일치)":nf0+"개 중 "+bad0+"개 다름 — [선택 설치]로 다시 설치하세요");
    a.State="FAIL";a.Cell="적용 안 됨 – "+whole[0];a.Reason=whole[1];continue;
   }
   // 설치 파일 확인
   int nf=0,bad=0;foreach(var f in M.Entries){if(f.Id!=id||!f.Applies(s.On)||ModelData(f)&&IsModelIndex(f.Target))continue;Record q;string h=Hash(f.Target);if(Mutable(f.Target)){nf++;if(h=="")bad++;continue;}nf++;if(s.Items.TryGetValue(f.Target,out q)){if(h!=q.After)bad++;}else if(h!=f.Sha)bad++;}
   if(nf>0)a.Files.Add(bad==0?nf+"개 모두 정상 (판별 번호 일치)":nf+"개 중 "+bad+"개 다름 — [선택 설치]로 다시 설치하세요");
   AppliedInfo r;
   if(rec.TryGetValue(id,out r)&&recTime>installed){
    a.State=r.State;a.Module=r.Module;a.Time=r.Time;a.Sites=r.Sites;a.Settings=r.Settings;a.Reason=r.Reason;a.Features=r.Features;a.Counters=r.Counters;
    if(a.Fails.Count>0){ // R43: 실패 보고가 있으면 상태·칸에 바로 사유(단계·어디서)
     foreach(var f in a.Fails)a.Features.Add(new[]{f[2]+" "+Short(f[3]),"FAIL",f[6]+" ["+f[7]+"]"});
     if(a.State=="OK")a.State="PART";
     a.Cell=(a.State=="FAIL"?"실패 ✖ — ":"일부 적용 ⚠ — ")+a.Fails[0][2]+" "+Short(a.Fails[0][3])+(a.Fails.Count>1?" 외 "+(a.Fails.Count-1)+"건":"")+" ["+a.Fails[0][7]+"]";
     continue;}
    int okf=a.Features.Count(x=>x[1]=="OK"),tot=a.Features.Count(x=>x[1]!="OFF");var failed=a.Features.Where(x=>x[1]=="FAIL").Select(x=>x[0]).ToList();
    a.Cell=a.State=="OK"?"적용 확인 ✔"+(a.Counters.Count>0?" ("+a.Counters[0][0]+" "+a.Counters[0][1]+"회)":""):
      a.State=="PART"?"일부 적용 ⚠ "+(tot>0?okf+"/"+tot:"")+(failed.Count>0?" — "+String.Join("·",failed.ToArray())+" ✖":""):
      a.State=="OFF"?"설정으로 꺼짐":"실패 ✖"+(a.Reason!=""&&a.Reason!="-"?" — "+Short(a.Reason):"");
    if(id==9&&a.State=="OK"){var t9=T9Screen(installed);
     if(t9.State=="FAIL"){a.State="PART";a.Cell="일부 적용 ⚠ — 개성 9칸 화면 실패("+t9.Text+")";a.Features.Add(new[]{"개성 9칸 화면","FAIL",t9.Text});}
     else if(t9.State=="OK")a.Features.Add(new[]{"개성 9칸 화면","OK",t9.Relaxed>0?"일부 화면 칸 간격 보정":""});}
    continue;
   }
   // R43: 시작 줄은 없어도 실패 보고(R43F)가 있으면 그것이 이유
   if(a.Fails.Count>0){a.State="FAIL";foreach(var f in a.Fails)a.Features.Add(new[]{f[2]+" "+Short(f[3]),"FAIL",f[6]+" ["+f[7]+"]"});a.Reason=a.Fails[0][6];
    a.Cell="실패 ✖ — "+a.Fails[0][2]+" "+Short(a.Fails[0][3])+(a.Fails.Count>1?" 외 "+(a.Fails.Count-1)+"건":"")+" ["+a.Fails[0][7]+"]";continue;}
   // 공통 기록이 없는 번호: 이전 방식 판정
   string l=legacy[i]??"";
   if(id==18||id==20||id==24||id==31){a.State=l.StartsWith("적용됨")?"FILE":"FAIL";a.Cell=l;if(l.StartsWith("적용 안 됨"))a.Reason=l.Substring(l.IndexOf('–')+1).Trim();continue;}
   if(l=="게임 실행 전"){a.State="PRE";a.Cell=l;continue;}
   if(l=="")a.State="NONE";else if(l.StartsWith("적용됨"))a.State="OK";else if(l.StartsWith("일부 적용"))a.State="PART";else if(l.StartsWith("적용 안 됨"))a.State="FAIL";else a.State="NONE";
   a.Cell=l==""?"실행 기록 없음":l;
   if(l.Contains("–"))a.Reason=l.Substring(l.IndexOf('–')+1).Trim();
  }
  return res;
 }
 // ---------- R40 A22: 통합패치 전체가 안 켜진 이유 ----------
 // 실행 파일 종류: ""=지원판(스팀 1.0.1 원본), 그 밖은 설명. 게임 안 안내창(SAN14_Notice)과 같은 기준.
 public static string ExeKind(string exePath){
  try{
   if(!File.Exists(exePath))return "게임 실행 파일(SAN14CE_KO.exe)이 없음";
   string h=Files.HashFile(exePath);if(h==Spec.ExpectedGame)return "";{int oi=Spec.OldGameIndex(h);if(oi>=0)return "옛 판 실행 파일: "+Spec.OldGameLabels[oi]+"(스팀 업데이트 필요)";}
   byte[] b=new byte[8192];using(var f=new FileStream(exePath,FileMode.Open,FileAccess.Read,FileShare.ReadWrite)){int got=0,k;while(got<b.Length&&(k=f.Read(b,got,b.Length-got))>0)got+=k;}
   return ExeKindFromHeader(b);
  }catch(Exception){return "게임 실행 파일을 읽지 못함";}
 }
 public static string ExeKindFromHeader(byte[] b){
  if(b.Length<0x200||b[0]!='M'||b[1]!='Z')return "게임 실행 파일 머리를 못 읽음";
  int pe=BitConverter.ToInt32(b,0x3c);if(pe<0x40||pe+0x200>b.Length||BitConverter.ToUInt32(b,pe)!=0x4550)return "게임 실행 파일 머리를 못 읽음";
  uint ts=BitConverter.ToUInt32(b,pe+8),sz=BitConverter.ToUInt32(b,pe+0x18+56);int nsec=BitConverter.ToUInt16(b,pe+6),opt=BitConverter.ToUInt16(b,pe+20);bool extra=false;
  for(int i=0;i<nsec&&i<32;i++){int o=pe+24+opt+i*40;if(o+8>b.Length)break;if(Encoding.ASCII.GetString(b,o,8).TrimEnd('\0')==".cecode")extra=true;}
  if(ts==0x6ad06f53u)return sz==0x2a02000u&&!extra?Spec.GameLabel+" 실행 파일이지만 내용이 바뀜(다른 패치가 고침)":"실행 파일이 다른 패치로 통째로 바뀜(예: 신무장 1000명 같은 exe 교체형)";
  if(ts==0x6a983c25u)return "옛 1.0.1 실행 파일(2026-09-02 빌드; 스팀 업데이트 필요, 옛 판은 R42i4 이하용)";
  if(ts==0x6a6d9f72u)return "1.0.1 이전 실행 파일(롤백판 등)";
  return "통합패치가 모르는 판의 실행 파일(게임 업데이트 등, 타임스탬프 "+ts.ToString("x8")+")";
 }
 // 마지막 실행의 시작 시각 근사: 기록의 가장 늦은 시각 − 10분(한 실행의 시작 줄은 보통 2분 안에 다 남음). installed 보다 이르면 installed.
 public static DateTime LastRunStart(string text,DateTime installed){
  DateTime last=DateTime.MinValue;if(String.IsNullOrEmpty(text))return installed;
  foreach(string raw in text.Split('\n')){var c=raw.TrimEnd('\r').Split('\t');DateTime t;
   string ts=c.Length>=4&&(c[0]=="R39"||c[0]=="R43F"||c[0]=="R43D")?c[3]:c.Length>=3&&c[0]=="R43E"?c[2]:null;
   if(ts!=null&&DateTime.TryParseExact(ts,"yyyy-MM-dd HH:mm:ss",System.Globalization.CultureInfo.InvariantCulture,System.Globalization.DateTimeStyles.None,out t)&&t>last)last=t;}
  DateTime inst=installed==DateTime.MinValue?installed:installed.AddSeconds(-2);   // 기록 시각은 초 단위라 설치 직후 실행도 들어오게
  DateTime r=last==DateTime.MinValue?inst:last.AddMinutes(-10);return r<inst?inst:r;
 }
 // 적용확인.log 의 마지막 안내창 줄(R40E\tMC\t시각\t코드\t로더파일 / R43E\tMC\t시각\t코드\t기대\t실제\t로더\t모듈)
 public static string[] LastNotice(string text){
  string[] r=null;if(String.IsNullOrEmpty(text))return null;
  foreach(string raw in text.Split('\n')){var c=raw.TrimEnd('\r').Split('\t');
   if(c.Length>=4&&c[0]=="R40E"&&c[1]=="MC")r=new[]{c[2],c[3],c.Length>4?c[4]:"-","",""};
   else if(c.Length>=4&&c[0]=="R43E"&&c[1]=="MC")r=new[]{c[2],c[3],c.Length>6?c[6]:"-",c.Length>5?c[4]+" / "+c[5]:"",c.Length>7?c[7]:""};   // R43: 기대/실제 판, 꺼진 모듈
  }
  return r;
 }
 public static string NoticeMeaning(string code){
  string k=(code.StartsWith("R40-MC")||code.StartsWith("R43-MC"))&&code.Length>6?code.Substring(6,1):"";
  switch(k){
   case "1":return "다른 모드가 게임 코드를 먼저 바꿔서 게임 확인 실패";
   case "2":return "실행 파일이 다른 패치로 통째로 바뀜(신무장 1000명 등)";
   case "3":return "옛 판 실행 파일(1.0.1 또는 그 이전)";
   case "4":return "모르는 판의 실행 파일(게임 업데이트 등)";
   case "5":return "공통 파일(리졸버) 없음";
   case "6":return "공통 파일(리졸버)이 다른 패치 것으로 바뀜";
  }
  return "게임 확인 실패";
 }
 // 마지막 실행 진단: null=문제 없음, {목록 칸 짧은 글, 오른쪽 자세한 이유}
 public string[] WholeStop(State s,string runtimeLog,DateTime installed){
  if(!Spec.AnyRuntime(s.On))return null;
  string exeKind=ExeKind(Files.Child(root,"SAN14CE_KO.exe"));
  if(exeKind!="")return new[]{"게임 실행 파일이 지원판이 아님",exeKind+". 통합패치 "+Spec.Version+"는 스팀 원본 "+Spec.GameLabel+" 실행 파일에서만 켜집니다. 스팀 [속성 → 설치된 파일 → 게임 파일 무결성 확인]으로 원본을 되돌린 뒤 실행하세요."};
  string rt=null;DateTime rtTime=DateTime.MinValue;
  try{if(!String.IsNullOrEmpty(runtimeLog)&&File.Exists(runtimeLog)){rtTime=File.GetLastWriteTime(runtimeLog);rt=TailText(runtimeLog,false);int k=rt.LastIndexOf("SAN14 ModCore 1.0",StringComparison.Ordinal);if(k<0)rt=null;else rt=rt.Substring(k);}}catch(Exception){rt=null;}
  if(rt==null||rtTime<=installed)return null;
  string head=rt.Length>600?rt.Substring(0,600):rt;
  string[] notice=null;DateTime apTime=DateTime.MinValue;
  try{string ap=Files.Child(root,ApplyLogRel);if(File.Exists(ap)){apTime=File.GetLastWriteTime(ap);notice=LastNotice(TailText(ap,false));}}catch(Exception){notice=null;}
  string code=notice!=null&&apTime>installed?notice[1]:"";
  if(head.Contains("SKIPPED: unsupported version")||head.Contains("SKIPPED: Resolver missing.")){
   string why=code!=""?NoticeMeaning(code):head.Contains("Resolver missing")?"공통 파일(리졸버) 없음":"게임 확인 실패";
   return new[]{"통합패치 전체 꺼짐("+why+")",
    "마지막 실행에서 통합패치가 게임 확인 단계에서 멈춰 모드를 하나도 싣지 않았습니다(모드 설정 창도 안 나옴). 이유: "+why+"."+(code!=""?" 게임 안 안내창 코드: "+code+(notice[3]!=""?" · 기대/실제 판: "+notice[3]:"")+(notice[2]!="-"?" · 게임 폴더의 로더 파일: "+notice[2]:""):"")
    +" 다른 모드를 깐 뒤라면 [선택 설치]를 다시 누르고, 그래도 같으면 같이 깐 모드를 빼고 실행해 보세요."};
  }
  // 게임 확인은 됐는데 이 폴더의 공통 모듈(1번과 함께 늘 깔리는 기록 모듈)이 안 올라옴 → 다른 게임 폴더에서 실행했거나 공통 파일이 바뀜
  if(head.Contains("CE KO:")&&rt.IndexOf("CrashLog: crash recorder armed",StringComparison.Ordinal)<0&&Hash("SAN14_CrashLog.dll")!="")
   return new[]{"이 폴더의 통합패치가 실행되지 않음","마지막 게임 실행 기록에 이 폴더의 통합패치 공통 모듈이 없습니다. 다른 게임 폴더(다른 스팀 라이브러리의 복사본 등)에서 게임이 실행됐거나, 다른 패치가 SAN14Mods 폴더를 바꿨을 수 있습니다. 위쪽 게임 폴더 칸이 실제로 실행하는 게임 폴더인지 확인하고 [선택 설치]를 다시 눌러 주세요."};
  return null;
 }
 // 1번과 함께 늘 깔리는 공통 파일(로더 포함) 중 기록과 다른 것 수
 public const string SharedResolverRel="SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll",OurResolverRel="SAN14Mods/resolvers/SAN14_Resolver_IP01.dll";
 public List<string> BaseChanged(State s){
  var r=new List<string>();if(!Spec.AnyRuntime(s.On))return r;
  foreach(var f in M.Entries){if(f.Id!=0||!f.Applies(s.On)||Mutable(f.Target)||ModuleIni(f.Target)||f.Target.Equals(SharedResolverRel,StringComparison.OrdinalIgnoreCase))continue;Record q;if(!s.Items.TryGetValue(f.Target,out q))continue;string h=Hash(f.Target);if(h!=q.After&&h!=f.Sha)r.Add(f.Target);} // 설치 기록이 있는 파일만(이번 판에 새로 생긴 파일은 설치 전이라 '바뀜' 아님)
  return r;
 }
 static string Short(string t){t=t.Replace("\r"," ").Replace("\n"," ");return t.Length>40?t.Substring(0,40)+"…":t;}
 // 기록 수집: 설치기 기록·runtime.log·각 모드 기록을 게임 폴더\SAN14_기록\ 에 복사, 50개/50MB 넘으면 오래된 것부터 정리
 public string CollectRecords(string installerLog){
  string dir=Files.Child(root,RecordsDir);Directory.CreateDirectory(dir);
  try{File.WriteAllText(Path.Combine(dir,"설치기.log"),installerLog??"",Encoding.UTF8);}catch(Exception){}
  string stamp=DateTime.Now.ToString("yyyyMMdd_HHmmss");
  Action<string,string> copy=(src,name)=>{try{if(File.Exists(src))File.Copy(src,Path.Combine(dir,name),true);}catch(Exception){}};
  copy(RuntimeLogPath(),"runtime.log");
  foreach(string n in CollectLogs)copy(Files.Child(root,n),Path.GetFileName(n));
  try{string crash=Files.Child(root,"SAN14_CrashLog.log");if(File.Exists(crash)&&new FileInfo(crash).Length>0)File.Copy(crash,Path.Combine(dir,"튕김_"+stamp+".log"),true);}catch(Exception){}
  TrimRecords(dir);return dir;
 }
 public static void TrimRecords(string dir){
  try{
   var all=new DirectoryInfo(dir).GetFiles().ToList();long total=all.Sum(f=>f.Length);int count=all.Count;
   var cand=all.Where(f=>f.Name!="적용확인.log"&&f.Name!="설치기.log").OrderBy(f=>f.LastWriteTime).ThenBy(f=>f.Name,StringComparer.Ordinal).ToList();
   while((count>50||total>50L*1024*1024)&&cand.Count>0){var f=cand[0];cand.RemoveAt(0);total-=f.Length;count--;f.Delete();}
   string ap=Path.Combine(dir,"적용확인.log");if(File.Exists(ap)&&new FileInfo(ap).Length>2*1024*1024){string tail=TailText(ap,false);int k=tail.IndexOf('\n');File.WriteAllText(ap,k>=0?tail.Substring(k+1):tail,new UTF8Encoding(false));}
  }catch(Exception){}
 }
 public string[] Applied(State s,string runtimeLog){
  var r=new string[Spec.Count];for(int i=0;i<Spec.Count;i++)r[i]="";
  string statePath=Files.Child(root,StateRel);DateTime installed=File.Exists(statePath)?File.GetLastWriteTime(statePath):DateTime.MinValue;
  string rt=null;DateTime rtTime=DateTime.MinValue;
  try{if(!String.IsNullOrEmpty(runtimeLog)&&File.Exists(runtimeLog)){rtTime=File.GetLastWriteTime(runtimeLog);rt=TailText(runtimeLog,false);int k=rt.LastIndexOf("SAN14 ModCore 1.0",StringComparison.Ordinal);if(k>=0)rt=rt.Substring(k);}}catch(Exception){rt=null;}
  string ic=null;string icPath=Files.Child(root,"SAN14CE_11Patch.log");
  try{if(File.Exists(icPath)){ic=TailText(icPath,true);int k=ic.LastIndexOf("지원 실행 파일 확인 완료",StringComparison.Ordinal);if(k>=0)ic=ic.Substring(k);}}catch(Exception){ic=null;}
  bool ran=rt!=null&&rtTime>installed;
  for(int i=0;i<Spec.Count;i++){
   int id=i+1;if(!s.On[i])continue;
   if(id==18||id==20||id==24||id==31){ // 파일만 바꾸는 항목
    if(id==31){r[i]=s.Items.ContainsKey(ScreenIndexRel)&&Hash(ScreenIndexRel)==s.Items[ScreenIndexRel].After?"적용됨 (파일)":"적용 안 됨 – 그래픽 색인이 바뀌어 있거나 건너뜀";continue;}
    string h=Hash(id==18?ExpRel:id==24?FixRel:FontRel);string ours=M.Values[id==18?"exp_modified":id==24?"fix_nostat":"font_rdb"];
    r[i]=h==ours?"적용됨 (파일)":id==20?"적용 안 됨 – 글꼴 파일이 바뀌어 있음":"적용 안 됨 – 설정 창에서 끔(원본 파일)";continue;
   }
   if(!ran){r[i]="게임 실행 전";continue;}
   r[i]=Judge(id,s.On,rt,ic??"");
   // R32: 13번은 '시작 기록 있음'이 아니라 실제 ACTIVE 로 판정하고, 개성 9개 화면(UI_Traits9_Hover)의 자체 기록도 함께 보여 줌
   if(id==13&&r[i]=="적용됨"){
    if(rt.IndexOf("SAN14CE.MeritTraits.HoverX RVA=",StringComparison.Ordinal)<0)r[i]="적용 안 됨 – 13번 시작 실패("+Reason(rt,"MeritTraits: ")+")";
    else{var t9=T9Screen(installed);
     r[i]=t9.State=="FAIL"?"일부 적용 – 개성 9칸 화면 실패("+t9.Text+")":t9.State=="OK"?"적용됨 (개성 9칸 화면 정상"+(t9.Relaxed>0?", 일부 화면 칸 간격 보정":"")+(t9.Warn!=""?", 경고: "+t9.Warn:"")+")":"적용됨 (개성 9칸 화면은 아직 안 열어 봄)";}
   }
  }
  return r;
 }
 // R42d: 개성 9칸 화면(9·13번 공용 부품) 판정 — 마지막 실행 기준. 화면 부품 기록(SAN14_Traits9_UI.log)은 마지막 한 줄만 남아
 //  성공·실패가 덮어써지므로 Traits9Check 가 적용확인.log 에 남긴 R42T9 줄(C=화면 실패 알림, D=원래 기준 거절 뒤 다시 잰 결과)도 함께 본다.
 //  실패 = SKIPPED(HOVER 제외)/FAILED 글자 또는 R42T9 C·D 실패 / 성공 = 그 밖의 CREATED·COPIED·APPLIED 글자 또는 D relaxed=ok
 public sealed class T9Info{public string State="NONE",Text="",Warn="";public int Relaxed;}
 static bool T9Time(string t,out DateTime d){return DateTime.TryParseExact(t,"yyyy-MM-dd HH:mm:ss",System.Globalization.CultureInfo.InvariantCulture,System.Globalization.DateTimeStyles.None,out d);}
 static string T9Why(string line){string t=line;int k=t.IndexOf("SAN14 Traits9 UI:",StringComparison.Ordinal);if(k>=0)t=t.Substring(k+17).Trim();
  k=t.IndexOf(" | ",StringComparison.Ordinal);if(k>=0)t=t.Substring(0,k);k=t.IndexOf(" - ",StringComparison.Ordinal);if(k>=0)t=t.Substring(k+3);if(t.Length>70)t=t.Substring(0,70)+"…";return t.Trim();}
 public static bool T9Bad(string l){return (l.IndexOf("SKIPPED",StringComparison.Ordinal)>=0&&l.IndexOf("HOVER_SKIPPED",StringComparison.Ordinal)<0)||l.IndexOf("FAILED",StringComparison.Ordinal)>=0;}
 public T9Info T9Screen(DateTime installed){
  var r=new T9Info();DateTime since=installed;bool seen=false;
  try{string ap=Files.Child(root,ApplyLogRel);
   if(File.Exists(ap)&&File.GetLastWriteTime(ap)>installed){
    string[] ls=TailText(ap,false).Split('\n');DateTime d;
    for(int i=ls.Length-1;i>=0;i--){var c=ls[i].TrimEnd('\r').Split('\t');if(c.Length>=5&&c[0]=="R39"&&c[1]=="9"&&c[2]=="SAN14_Traits9Check.dll"&&T9Time(c[3],out d)){d=d.AddSeconds(-60);if(d>since)since=d;break;}}
    foreach(string raw in ls){var c=raw.TrimEnd('\r').Split('\t');DateTime t;if(c.Length<4||c[0]!="R42T9"||!T9Time(c[2],out t)||t<since)continue;
     if(c[1]=="C"){if(r.Text=="")r.Text=T9Why(c[3]);r.State="FAIL";}
     else if(c[1]=="D"){if(c[3].Contains("relaxed=ok")){r.Relaxed++;seen=true;}else if(c[3].Contains("relaxed=fail")||c[3].Contains("label create failed")){r.State="FAIL";if(r.Text=="")r.Text="화면 칸 모양을 못 맞춤("+(c[3].Length>60?c[3].Substring(0,60)+"…":c[3])+")";}}
    }
   }
  }catch(Exception){}
  try{string tp=Files.Child(root,"SAN14_Traits9_UI.log");
   if(File.Exists(tp)&&File.GetLastWriteTime(tp)>since){string[] ls=TailText(tp,false).Split(new[]{'\r','\n'},StringSplitOptions.RemoveEmptyEntries);string l=ls.Length>0?ls[ls.Length-1]:"";
    if(l!=""){if(T9Bad(l)){r.State="FAIL";if(r.Text=="")r.Text=T9Why(l);}else{seen=true;if(l.IndexOf("WARNING",StringComparison.Ordinal)>=0)r.Warn=T9Why(l);}}}
  }catch(Exception){}
  if(r.State!="FAIL"&&seen)r.State="OK";
  return r;
 }
 static string Reason(string text,string key){int k=text.LastIndexOf(key,StringComparison.Ordinal);if(k<0)return "기록 참조";string t=text.Substring(k+key.Length);int e=t.IndexOfAny(new[]{'\r','\n'});if(e>=0)t=t.Substring(0,e);if(t.Length>70)t=t.Substring(0,70)+"…";return t.Trim();}
 static string TailText(string path,bool utf16){
  using(var f=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.ReadWrite|FileShare.Delete)){
   long n=Math.Min(f.Length,3L*1024*1024);f.Seek(f.Length-n,SeekOrigin.Begin);if(utf16&&(n&1)!=0){f.Seek(1,SeekOrigin.Current);n--;}
   var b=new byte[n];int got=0;while(got<n){int k=f.Read(b,got,(int)(n-got));if(k<=0)break;got+=k;}
   return utf16?Encoding.Unicode.GetString(b,0,got):Encoding.UTF8.GetString(b,0,got);
  }
 }
 static string Judge(int id,bool[] on,string rt,string ic){
  Func<string,bool> R=x=>rt.IndexOf(x,StringComparison.Ordinal)>=0;Func<string,bool> I=x=>ic.IndexOf(x,StringComparison.Ordinal)>=0;
  const string ok="적용됨";string no="적용 안 됨 – 시작 기록 없음";
  switch(id){
   case 1: if(R("settings page 6 not attached")||R("page 6 write rejected"))return "적용 안 됨 – 설정 창 5·6쪽 연결 실패";return I("정보 메뉴 모드 설정 설치")&&R("settings pages attached")?ok:I("정보 메뉴 모드 설정 설치")?"일부 적용 – 설정 창 1~4쪽만(5·6쪽 연결 기록 없음)":no;
   case 2: return I("자동인사 설치")?ok:no;
   case 3: if(I("문전박대 설치 실패"))return "적용 안 됨 – 외교 메뉴 코드가 달라 설치 실패";return I("문전박대 설치:")?ok:no;
   case 4: if(!(I("이벤트 자동 포커스")&&I("행동보고·수지보고 화면 생략")&&I("도시 증축 암전 제거 설치")))return no;
    if(!R("SceneSkip R42:"))return "일부 적용 – 재해·등용 결과·보물 발견 생략 시작 기록 없음";
    return R("NOT hooked")&&rt.LastIndexOf("SceneSkip R42:",StringComparison.Ordinal)>=0&&rt.Substring(rt.LastIndexOf("SceneSkip R42:",StringComparison.Ordinal)).Split(new[]{'\n'})[0].IndexOf("NOT hooked",StringComparison.Ordinal)>=0?"일부 적용 – 재해·등용 결과·보물 발견 생략 중 일부 자리가 달라 원래대로":ok;
   case 5: return I("대열 이동")&&I("수행 연결")?ok:no;
   case 6: return I("진행속도 확장")?ok:no;
   case 7: return I("인재 메뉴")?ok:no;
   case 8: return I("BGM 플레이어 연결")?ok:no;
   case 9: return R("Collab entry invoked")?ok:no;
   case 10: if(I("CAPACITY100"))return "적용 안 됨 – 모델 ID 확장 확인 실패";if(I("확장 ID 25~173 예약"))return ok+" (모델 0~173)";return I("확장 ID 25~125 예약")?"적용됨 (이전 판 – 모델 0~125)":no;
   case 11: if(R("automatic runtime launch FAILED"))return "적용 안 됨 – 3D 런타임 시작 실패";return R("FieldLeader3D automatic runtime started")?ok:no;
   case 12: return R("Fog")||I("안개")?ok:"확인 불가 – 안개 기록은 SAN14_FogTrial.log";
   case 13: return R("MeritTraits:")||R("MeritTraits RC4")?ok:no;
   case 14: return R("TraitHover entry invoked")?(on[8]||on[12]?"적용됨 (전법 설명; 개성 설명은 9·13번)":ok):no;
   case 15: return R("RelatedOfficersTab RC2.4: HOOKS_INSTALLED")?ok:no;
   case 16: return R("SortiePresets RVA=")?ok:no;
   case 17: if(R("Stats200New R43: caps 14 sites OK")&&R("xp-growth relay installed")&&R("growth beyond 100 installed"))return ok;if(R("Stats200New R43: caps 14 sites OK")&&R("xp-growth relay installed"))return "일부 적용 – 100 넘는 경험치 성장 미적용(편집·표시 상한 200 은 적용)";if(R("Stats200New R43: caps 14 sites OK"))return "일부 적용 – 경험치 성장 분기 미적용(상한 상수는 적용)";return R("Stats200New R43:")?no:no;
   case 19: return R("FPS120 R8 ACTIVE")?ok:no;
   case 21: return R("PrisonerReturn R4 loaded")?ok:no;
   case 22: if(R("READ-ONLY"))return "적용 안 됨 – 읽기 전용 모드";return R("DomesticCE: ACTIVE")?ok:no;
   case 23: return R("CityCaps: city limits extended")?ok:no;
   case 25: return R("Patch27: checked feature groups")?ok:no;
  }
  return "";
 }
 public string[] Status(State s){
  var r=new string[Spec.Count];
  for(int i=0;i<Spec.Count;i++){
   int id=i+1;if(!s.On[i]){r[i]="미설치";continue;}
   bool ok=s.Version==7;
   // R43 7차: 파일 하나 못 읽어도 상태 확인 전체를 멈추지 않음(그 번호만 '설치 확인 필요')
   foreach(var f in M.Entries){if(f.Id!=id||!f.Applies(s.On))continue;if(ModelData(f)&&(!s.Items.ContainsKey(f.Target)||IsModelIndex(f.Target)))continue;Record q;string h;try{h=Hash(f.Target);}catch(Exception){ok=false;continue;}if(Mutable(f.Target)){if(h=="")ok=false;continue;}if(s.Items.TryGetValue(f.Target,out q)){if(h!=q.After)ok=false;}else if(h!=f.Sha)ok=false;}
   if(id==18||id==24){string h="";try{h=Hash(id==18?ExpRel:FixRel);}catch(Exception){}string orig=M.Values[id==18?"exp_original":"fix_original"],ours=M.Values[id==18?"exp_modified":"fix_nostat"];r[i]=h==ours?"설치됨":h==orig?(ok?"설치됨(게임 안에서 OFF)":"설치 확인 필요"):"설치 확인 필요";continue;}
   r[i]=ok?"설치됨":s.Version==7?"설치 확인 필요":"이전 설치기 기록(갱신 필요)";
   if(id==10&&ok)r[i]+=s.Items.ContainsKey(ModelCatalogRel)?" (추가 모델 149개)":" (추가 모델 없음)";
   if(id==31&&ok)r[i]+=s.Items.ContainsKey(ScreenIndexRel)?"":" (그래픽 건너뜀)";
  }
  return r;
 }
}
}
