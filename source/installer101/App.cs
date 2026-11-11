using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace San14Patch101 {
static class Program {
 // R43.28: 새 판 받기 뒤 새 설치기가 받는 인자 --game <게임 폴더> --after-update <이전 판>
 public static string GameArg="",AfterUpdate="";
 [STAThread] static void Main(string[] args){
  for(int i=0;i+1<args.Length;i++){if(args[i]=="--game")GameArg=args[i+1].Trim('"');else if(args[i]=="--after-update")AfterUpdate=args[i+1].Trim('"');}
  bool first;using(var mutex=new Mutex(true,"Local\\SAN14CE_Patch101",out first)){
   if(!first&&AfterUpdate!=""){try{first=mutex.WaitOne(30000);}catch(AbandonedMutexException){first=true;}}   // 옛 설치기가 닫히기를 기다림
   if(!first){MessageBox.Show("이미 열려 있는 설치창을 종료한 뒤 다시 실행해 주세요.");return;}
   Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
   try{Application.Run(new MainForm());}catch(Exception e){ErrorBox.Show(null,"설치기 실행",ErrorBox.Describe("설치기를 실행하는 중",null,e,"설치기 폴더(ZIP 을 푼 폴더)에 쓰기 권한이 있는지, 백신이 파일을 막지 않는지 확인한 뒤 다시 실행해 주세요."));}
  }
 }
}
// 글자를 복사할 수 있는 오류·안내 창 (카페 댓글용)
public static class ErrorBox {
 public static string Describe(string doing,string file,Exception e,string fix){
  var b=new StringBuilder();b.AppendLine("[통합패치 "+Spec.Version+" 설치기]");
  if(!String.IsNullOrEmpty(doing))b.AppendLine("무엇을 하다가: "+doing);
  if(!String.IsNullOrEmpty(file))b.AppendLine("어떤 파일: "+file);
  if(e!=null){b.AppendLine("오류: "+e.Message);if(e is UnauthorizedAccessException)fix=fix??"게임 폴더에 쓰기 권한이 없습니다. 설치.cmd 를 관리자 권한으로 실행하거나 게임 폴더 권한을 확인해 주세요.";else if(e is IOException&&e.Message.Contains("사용 중"))fix=fix??"게임·런처·다른 설치기를 모두 닫은 뒤 다시 눌러 주세요.";}
  if(!String.IsNullOrEmpty(fix))b.AppendLine("해결: "+fix);
  b.AppendLine();b.AppendLine("이 글자를 그대로 복사해 댓글로 알려 주시면 원인을 확인할 수 있습니다.");
  return b.ToString();
 }
 public static void Show(IWin32Window owner,string title,string text){
  using(var f=new Form{Text=title,ClientSize=new Size(720,360),StartPosition=FormStartPosition.CenterParent,MinimizeBox=false,MaximizeBox=false,ShowInTaskbar=false,Font=new Font("Malgun Gothic",10)}){
   var box=new TextBox{Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill,Text=text,BackColor=SystemColors.Window};
   var panel=new FlowLayoutPanel{Dock=DockStyle.Bottom,Height=44,FlowDirection=FlowDirection.RightToLeft,Padding=new Padding(6)};
   var close=new Button{Text="닫기",AutoSize=true,Height=30};var copy=new Button{Text="이 내용 복사",AutoSize=true,Height=30};
   close.Click+=delegate{f.Close();};copy.Click+=delegate{try{Clipboard.SetText(text);copy.Text="복사했습니다";}catch(Exception){}};
   panel.Controls.Add(close);panel.Controls.Add(copy);f.Controls.Add(box);f.Controls.Add(panel);f.AcceptButton=close;
   if(owner!=null)f.ShowDialog(owner);else f.ShowDialog();
  }
 }
 // 선택지 버튼이 있는 물음 창: 누른 버튼 번호(0~), 창을 닫으면 -1
 public static int Ask(IWin32Window owner,string title,string text,string[] choices){
  int result=-1;
  using(var f=new Form{Text=title,ClientSize=new Size(640,260),StartPosition=FormStartPosition.CenterParent,MinimizeBox=false,MaximizeBox=false,ShowInTaskbar=false,Font=new Font("Malgun Gothic",10),FormBorderStyle=FormBorderStyle.FixedDialog}){
   var label=new Label{Text=text,Dock=DockStyle.Fill,Padding=new Padding(14),AutoSize=false};
   var panel=new FlowLayoutPanel{Dock=DockStyle.Bottom,Height=50,FlowDirection=FlowDirection.RightToLeft,Padding=new Padding(8)};
   for(int i=choices.Length-1;i>=0;i--){int k=i;var b=new Button{Text=choices[i],AutoSize=true,Height=32,Padding=new Padding(6,0,6,0)};b.Click+=delegate{result=k;f.Close();};panel.Controls.Add(b);}
   f.Controls.Add(label);f.Controls.Add(panel);
   if(owner!=null)f.ShowDialog(owner);else f.ShowDialog();
  }
  return result;
 }
}
public sealed class MainForm:Form {
 readonly string package=Spec.FindPackageRoot(AppDomain.CurrentDomain.BaseDirectory);
 readonly Manifest manifest;
 readonly TextBox target=new TextBox(),log=new TextBox(),desc=new TextBox(),detail=new TextBox();readonly DataGridView grid=new DataGridView();
 readonly Button rollbackBtn=new Button(),updateBtn=new Button(),browse=new Button(),refresh=new Button(),tabAll=new Button(),tabNone=new Button(),tabInstall=new Button(),tabRemove=new Button(),updateInstalled=new Button(),allSel=new Button(),allNone=new Button(),allInstall=new Button(),allRemove=new Button(),records=new Button(),openLog=new Button(),copyDetail=new Button();
 readonly Label summary=new Label(),tabLabel=new Label(),descTitle=new Label(),detailTitle=new Label(),pathLabel=new Label(),relHead=new Label();
 readonly TabControl tabs=new TabControl();
 // R43.28: 업데이트·작업 기록 탭, 깃허브 판 목록
 readonly ListBox relList=new ListBox();readonly TextBox relNotes=new TextBox(),recordText=new TextBox();Control updatePanel,recordPanel;
 List<Release> releases;string updateText="공개판 확인 전",stateText="상태: 아직 확인하지 않음";bool stateWarn;readonly System.Windows.Forms.Timer gameTimer=new System.Windows.Forms.Timer();
 int UpdateTab{get{return Spec.CategoryNames.Length+1;}}
 int SpecialTab(){int i=tabs.SelectedIndex-Spec.CategoryNames.Length;return i>=1?i:0;}   // 1=업데이트 2=작업 기록 0=분류 탭
 bool busy,missNoted,modelNoted,copiesNoted;static List<string> otherCopies=new List<string>();State loaded;string loadedRoot="";string lastLoadError="";Engine.AppliedInfo[] appliedInfo;
 int CurrentCat(){return SpecialTab()!=0?-1:tabs.SelectedIndex-1;} // -1 = 전체
 public MainForm(){
  manifest=Manifest.Load(Path.Combine(package,"support","manifest101.xml"),ManifestHash.Value);
  Text="삼국지14 CE 통합패치 "+Spec.Version+" (공식 "+Spec.GameLabel+" 전용)";
  Font=new Font("Malgun Gothic",10);ClientSize=new Size(1400,900);MinimumSize=new Size(1100,700);AutoScaleMode=AutoScaleMode.Font;StartPosition=FormStartPosition.CenterScreen;
  var layout=new TableLayoutPanel{Dock=DockStyle.Fill,Padding=new Padding(14),ColumnCount=2,RowCount=7};
  layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,64));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,36));
  foreach(float h in new float[]{44,50,36,34})layout.RowStyles.Add(new RowStyle(SizeType.Absolute,h));
  layout.RowStyles.Add(new RowStyle(SizeType.Percent,100));layout.RowStyles.Add(new RowStyle(SizeType.Absolute,46));layout.RowStyles.Add(new RowStyle(SizeType.Absolute,46));Controls.Add(layout);
  var title=new Label{Text="삼국지14 CE 통합패치 "+Spec.Version,Font=new Font(Font.FontFamily,19,FontStyle.Bold),Dock=DockStyle.Fill,AutoSize=false};layout.Controls.Add(title,0,0);layout.SetColumnSpan(title,2);
  // R43.28(사용자 11-11): 제목 밑 설명 줄 대신 버튼 줄(버전 되돌리기·업데이트·게임 폴더 변경) → 상태 줄 → 경로 줄
  var topButtons=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.LeftToRight,WrapContents=false,Margin=new Padding(0,4,0,0)};
  rollbackBtn.Text="버전 되돌리기…";updateBtn.Text="업데이트";browse.Text="게임 폴더 변경…";refresh.Text="상태 새로고침";
  foreach(var b in new[]{rollbackBtn,updateBtn,browse,refresh}){b.AutoSize=true;b.Height=36;b.Padding=new Padding(10,0,10,0);topButtons.Controls.Add(b);}
  rollbackBtn.Font=updateBtn.Font=browse.Font=new Font(Font,FontStyle.Bold);
  layout.Controls.Add(topButtons,0,1);layout.SetColumnSpan(topButtons,2);
  summary.Text=stateText;summary.Font=new Font(Font,FontStyle.Bold);summary.Dock=DockStyle.Fill;summary.TextAlign=ContentAlignment.MiddleLeft;summary.AutoEllipsis=true;layout.Controls.Add(summary,0,2);layout.SetColumnSpan(summary,2);
  pathLabel.Dock=DockStyle.Fill;pathLabel.TextAlign=ContentAlignment.MiddleLeft;pathLabel.ForeColor=Color.FromArgb(70,70,70);pathLabel.Font=new Font(Font.FontFamily,9);pathLabel.AutoEllipsis=true;layout.Controls.Add(pathLabel,0,3);layout.SetColumnSpan(pathLabel,2);
  target.Dock=DockStyle.Fill;   // 게임 폴더 값(화면에는 경로 줄로 보임, 바꾸기는 [게임 폴더 변경…])
  // 왼쪽: 탭 + 표 + 기록
  var left=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=3,Margin=new Padding(0,0,8,0)};
  left.RowStyles.Add(new RowStyle(SizeType.Absolute,30));left.RowStyles.Add(new RowStyle(SizeType.Percent,100));left.RowStyles.Add(new RowStyle(SizeType.Absolute,84));
  tabs.Dock=DockStyle.Fill;tabs.SizeMode=TabSizeMode.Normal;tabs.Padding=new Point(8,3);tabs.Appearance=TabAppearance.Normal;
  tabs.TabPages.Add(new TabPage("전체 "+Spec.Shown));
  for(int c=0;c<Spec.CategoryNames.Length;c++)tabs.TabPages.Add(new TabPage(Spec.CategoryNames[c]+" "+Spec.InCategory(c).Length));
  tabs.TabPages.Add(new TabPage("업데이트"));tabs.TabPages.Add(new TabPage("작업 기록"));
  // 탭은 헤더만 쓰고 내용은 하나의 표를 필터
  var tabHost=new Panel{Dock=DockStyle.Fill,Margin=new Padding(0)};tabs.Height=30;tabs.Dock=DockStyle.Top;tabHost.Controls.Add(tabs);
  left.Controls.Add(tabHost,0,0);
  grid.Dock=DockStyle.Fill;grid.ScrollBars=ScrollBars.Both;grid.AllowUserToAddRows=false;grid.AllowUserToDeleteRows=false;grid.AllowUserToResizeRows=false;grid.RowHeadersVisible=false;grid.AutoGenerateColumns=false;grid.MultiSelect=false;grid.SelectionMode=DataGridViewSelectionMode.FullRowSelect;grid.RowTemplate.Height=28;grid.AutoSizeColumnsMode=DataGridViewAutoSizeColumnsMode.Fill;grid.BackgroundColor=SystemColors.Window;grid.BorderStyle=BorderStyle.FixedSingle;grid.ColumnHeadersHeightSizeMode=DataGridViewColumnHeadersHeightSizeMode.DisableResizing;grid.ColumnHeadersHeight=32;
  grid.Columns.Add(new DataGridViewCheckBoxColumn{Name="Wanted",HeaderText="설치",FillWeight=6,MinimumWidth=46,SortMode=DataGridViewColumnSortMode.NotSortable});
  grid.Columns.Add(new DataGridViewTextBoxColumn{Name="Number",HeaderText="번호",ReadOnly=true,FillWeight=5,MinimumWidth=46,SortMode=DataGridViewColumnSortMode.NotSortable});
  grid.Columns.Add(new DataGridViewTextBoxColumn{Name="Feature",HeaderText="패치 이름",ReadOnly=true,FillWeight=52,MinimumWidth=360,SortMode=DataGridViewColumnSortMode.NotSortable});
  grid.Columns.Add(new DataGridViewTextBoxColumn{Name="Current",HeaderText="현재 상태",ReadOnly=true,FillWeight=12,MinimumWidth=100,SortMode=DataGridViewColumnSortMode.NotSortable});
  grid.Columns.Add(new DataGridViewTextBoxColumn{Name="Applied",HeaderText="게임 적용 (마지막 실행)",ReadOnly=true,FillWeight=25,MinimumWidth=200,SortMode=DataGridViewColumnSortMode.NotSortable});
  for(int r=0;r<Spec.Count;r++){int id=Spec.IdAt(r);int index=grid.Rows.Add(false,Spec.Num(id),Spec.Names[id-1]+MarkText(id),"미확인","");grid.Rows[index].Cells[2].ToolTipText=Spec.Details[id-1][0];if(Spec.Mark(id)!="")grid.Rows[index].DefaultCellStyle.BackColor=Color.FromArgb(255,240,220);}
  grid.Rows[Spec.RowOf(Spec.Fog)].Visible=false; // R41: 12번 삭제(빈 번호)
  grid.CurrentCellDirtyStateChanged+=delegate{if(grid.IsCurrentCellDirty)grid.CommitEdit(DataGridViewDataErrorContexts.Commit);};
  grid.SelectionChanged+=delegate{ShowDetail();};
  grid.CellClick+=delegate(object sender,DataGridViewCellEventArgs ev){if(ev.RowIndex>=0)ShowDetail();};
  // 가운데: 분류 탭 = 표, 업데이트 탭 = 판 목록·바뀐 내용, 작업 기록 탭 = 설치 기록
  var midHost=new Panel{Dock=DockStyle.Fill,Margin=new Padding(0)};
  var up=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=2,Visible=false,Margin=new Padding(0)};
  up.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,32));up.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,68));up.RowStyles.Add(new RowStyle(SizeType.Absolute,30));up.RowStyles.Add(new RowStyle(SizeType.Percent,100));
  relHead.Dock=DockStyle.Fill;relHead.Font=new Font(Font,FontStyle.Bold);relHead.TextAlign=ContentAlignment.MiddleLeft;up.Controls.Add(relHead,0,0);up.SetColumnSpan(relHead,2);
  relList.Dock=DockStyle.Fill;relList.IntegralHeight=false;relList.ItemHeight=22;up.Controls.Add(relList,0,1);
  relNotes.Dock=DockStyle.Fill;relNotes.Multiline=true;relNotes.ReadOnly=true;relNotes.ScrollBars=ScrollBars.Vertical;relNotes.BackColor=SystemColors.Window;up.Controls.Add(relNotes,1,1);
  relList.SelectedIndexChanged+=delegate{var rr=relList.SelectedItem as Release;relNotes.Text=rr==null?"":rr.Title+"  ("+rr.Date+")\r\n\r\n"+rr.Notes;};
  recordText.Dock=DockStyle.Fill;recordText.Multiline=true;recordText.ReadOnly=true;recordText.ScrollBars=ScrollBars.Both;recordText.WordWrap=false;recordText.Font=new Font("Consolas",9);recordText.BackColor=SystemColors.Window;recordText.Visible=false;
  updatePanel=up;recordPanel=recordText;
  midHost.Controls.Add(grid);midHost.Controls.Add(up);midHost.Controls.Add(recordText);
  left.Controls.Add(midHost,0,1);
  log.Dock=DockStyle.Fill;log.Multiline=true;log.ReadOnly=true;log.ScrollBars=ScrollBars.Vertical;log.Font=new Font("Consolas",9);left.Controls.Add(log,0,2);
  layout.Controls.Add(left,0,4);
  // 오른쪽: 설명칸 + 게임 적용 확인
  var right=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=5,Margin=new Padding(0)};
  right.RowStyles.Add(new RowStyle(SizeType.Absolute,26));right.RowStyles.Add(new RowStyle(SizeType.Percent,52));right.RowStyles.Add(new RowStyle(SizeType.Absolute,26));right.RowStyles.Add(new RowStyle(SizeType.Percent,48));right.RowStyles.Add(new RowStyle(SizeType.Absolute,36));
  descTitle.Text="설명";descTitle.Font=new Font(Font,FontStyle.Bold);descTitle.Dock=DockStyle.Fill;descTitle.TextAlign=ContentAlignment.MiddleLeft;right.Controls.Add(descTitle,0,0);
  desc.Multiline=true;desc.ReadOnly=true;desc.ScrollBars=ScrollBars.Vertical;desc.Dock=DockStyle.Fill;desc.BackColor=Color.FromArgb(255,253,245);desc.Text="항목을 누르면 설명이 나옵니다.";right.Controls.Add(desc,0,1);
  detailTitle.Text="게임 적용 확인 (마지막 실행)";detailTitle.Font=new Font(Font,FontStyle.Bold);detailTitle.Dock=DockStyle.Fill;detailTitle.TextAlign=ContentAlignment.MiddleLeft;right.Controls.Add(detailTitle,0,2);
  detail.Multiline=true;detail.ReadOnly=true;detail.ScrollBars=ScrollBars.Vertical;detail.Dock=DockStyle.Fill;detail.BackColor=Color.FromArgb(243,251,251);right.Controls.Add(detail,0,3);
  var dbtn=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.LeftToRight,Margin=new Padding(0)};
  openLog.Text="기록 열기";copyDetail.Text="이 내용 복사";foreach(var b in new[]{openLog,copyDetail}){b.AutoSize=true;b.Height=28;dbtn.Controls.Add(b);}right.Controls.Add(dbtn,0,4);
  layout.Controls.Add(right,1,4);
  // 버튼 두 줄
  var row1=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.LeftToRight,WrapContents=false,Margin=new Padding(0,6,0,0)};
  tabLabel.Text="이 탭만:";tabLabel.Font=new Font(Font,FontStyle.Bold);tabLabel.AutoSize=false;tabLabel.Width=150;tabLabel.Height=32;tabLabel.TextAlign=ContentAlignment.MiddleLeft;row1.Controls.Add(tabLabel);
  tabAll.Text="이 탭 전체 선택";tabNone.Text="이 탭 전체 해제";tabInstall.Text="이 탭 선택 설치";tabRemove.Text="이 탭 선택 제거";
  foreach(var b in new[]{tabAll,tabNone,tabInstall,tabRemove}){b.AutoSize=true;b.Height=32;b.Padding=new Padding(8,0,8,0);row1.Controls.Add(b);}
  layout.Controls.Add(row1,0,5);layout.SetColumnSpan(row1,2);
  var row2=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.LeftToRight,WrapContents=false,Margin=new Padding(0)};
  var allLabel=new Label{Text="모든 탭:",Font=new Font(Font,FontStyle.Bold),AutoSize=false,Width=150,Height=32,TextAlign=ContentAlignment.MiddleLeft};row2.Controls.Add(allLabel);
  allSel.Text="전체 선택";allNone.Text="전체 해제";allInstall.Text="선택 설치";allRemove.Text="선택 제거";records.Text="기록 폴더 열기";updateInstalled.Text="설치된 항목만 갱신";
  foreach(var b in new[]{allSel,allNone,allInstall,allRemove,updateInstalled,records}){b.AutoSize=true;b.Height=32;b.Padding=new Padding(8,0,8,0);row2.Controls.Add(b);}
  tabInstall.ForeColor=allInstall.ForeColor=Color.FromArgb(0,80,160);updateInstalled.ForeColor=Color.FromArgb(0,110,60);tabRemove.ForeColor=allRemove.ForeColor=Color.FromArgb(170,40,30);
  layout.Controls.Add(row2,0,6);layout.SetColumnSpan(row2,2);
  // 동작
  tabs.SelectedIndexChanged+=delegate{ApplyFilter();};
  browse.Click+=delegate{using(var f=new OpenFileDialog{Title="게임의 SAN14CE_KO.exe 선택",Filter="삼국지14 CE|SAN14CE_KO.exe",CheckFileExists=true})if(f.ShowDialog(this)==DialogResult.OK){target.Text=Path.GetDirectoryName(f.FileName);LoadState();}};
  refresh.Click+=delegate{LoadState();};updateBtn.Click+=delegate{DoUpdate();};rollbackBtn.Click+=delegate{DoRollback();};
  gameTimer.Interval=3000;gameTimer.Tick+=delegate{ShowStatus();};
  target.Leave+=delegate{if(!String.Equals(CleanRoot(),loadedRoot,StringComparison.OrdinalIgnoreCase))LoadState();};
  tabAll.Click+=delegate{SetChecks(true,true);};tabNone.Click+=delegate{SetChecks(false,true);};allSel.Click+=delegate{SetChecks(true,false);};allNone.Click+=delegate{SetChecks(false,false);};
  tabInstall.Click+=delegate{Start(true,true);};tabRemove.Click+=delegate{Start(false,true);};allInstall.Click+=delegate{Start(true,false);};allRemove.Click+=delegate{Start(false,false);};
  // P218(사용자 11-10): 지금 깔려 있는 번호만 새 파일로 다시 설치. 체크 상태와 상관없고, 안 깔린 번호는 건드리지 않음
  updateInstalled.Click+=delegate{if(loaded==null){Start(true,false);return;}if(!loaded.On.Any(x=>x)){MessageBox.Show(this,"설치된 항목이 없습니다. 항목을 체크하고 [선택 설치]를 눌러 주세요.");return;}Start(true,false,(bool[])loaded.On.Clone());};
  records.Click+=delegate{OpenRecords();};openLog.Click+=delegate{OpenRecords();};
  copyDetail.Click+=delegate{try{Clipboard.SetText(detailTitle.Text+"\r\n"+detail.Text);}catch(Exception){}};
  FormClosing+=delegate(object sender,FormClosingEventArgs e){if(busy){e.Cancel=true;}};
  Shown+=delegate{var work=Screen.FromControl(this).WorkingArea;if(Height>work.Height)Height=work.Height;if(Width>work.Width)Width=work.Width;
   try{target.Text=Program.GameArg!=""&&Directory.Exists(Program.GameArg)?Program.GameArg:Detect();LoadState();}catch(Exception e){WriteLog(e.Message);}
   gameTimer.Start();StartUpdateCheck();
   if(Program.AfterUpdate!="")BeginInvoke(new Action(AfterUpdatePrompt));};
  ApplyFilter();
 }
 string CleanRoot(){return target.Text.Trim().Trim('"');}
 Engine Make(){var e=new Engine(CleanRoot(),package,manifest);e.Log=WriteLog;return e;}
 void WriteLog(string text){if(InvokeRequired){BeginInvoke(new Action<string>(WriteLog),text);return;}if(log.TextLength>250000)log.Text=log.Text.Substring(log.TextLength-160000);log.AppendText("["+DateTime.Now.ToString("HH:mm:ss")+"] "+text+Environment.NewLine);}
 bool InTab(int id){if(Spec.Retired(id))return false;int c=CurrentCat();return c<0||Spec.Category(id)==c;}
 void ApplyFilter(){
  int sp=SpecialTab();
  grid.Visible=sp==0;updatePanel.Visible=sp==1;recordPanel.Visible=sp==2;TabButtons();
  if(sp==1){FillReleases();return;}
  if(sp==2){LoadRecordText();tabLabel.Text="이 탭만:";return;}
  grid.CurrentCell=null;grid.ClearSelection();
  for(int r=0;r<Spec.Count;r++)grid.Rows[r].Visible=InTab(Spec.IdAt(r));
  int c=CurrentCat();tabLabel.Text=c<0?"이 탭(전체)만:":"이 탭("+Spec.CategoryNames[c]+")만:";
 }
 void TabButtons(){bool on=SpecialTab()==0&&!busy;foreach(var b in new[]{tabAll,tabNone,tabInstall,tabRemove})b.Enabled=on;}
 void SetChecks(bool on,bool tabOnly){for(int r=0;r<Spec.Count;r++)if(!tabOnly||InTab(Spec.IdAt(r)))grid.Rows[r].Cells[0].Value=on;}
 bool[] Selected(bool tabOnly){grid.EndEdit();var b=new bool[Spec.Count];for(int i=0;i<Spec.Count;i++){int id=i+1;b[i]=Convert.ToBoolean(grid.Rows[Spec.RowOf(id)].Cells[0].Value)&&(!tabOnly||InTab(id));}return b;}
 void ShowDetail(){
  // R43.29: 고른 줄이 없을 때도 오른쪽 칸을 지금 상태로(게임 폴더를 읽은 뒤에도 "게임 폴더를 먼저 확인" 이 남던 것)
  if(grid.CurrentRow==null||grid.CurrentRow.Index<0){detailTitle.Text="게임 적용 확인";detail.Text=loaded==null?"게임 폴더를 먼저 확인해 주세요.":"목록에서 번호를 누르면 그 번호가 게임에서 적용됐는지 나옵니다.";return;}
  int id=Spec.IdAt(grid.CurrentRow.Index);var d=Spec.Details[id-1];
  descTitle.Text=id+"번 "+Spec.Names[id-1]+MarkText(id);
  desc.Text=(Spec.Changed.ContainsKey(id)?"■ "+Spec.Version+" 에서 바뀐 것\r\n"+Spec.Changed[id]+"\r\n\r\n":"")+(id==1&&LockedSettings()?"■ 잠김\r\n다른 번호가 깔려 있어 함께 설치됩니다(다른 번호의 게임 안 스위치가 모두 이 창에 있음).\r\n\r\n":"")+"■ 무엇을 하나\r\n"+d[0]+"\r\n\r\n■ 켜면 이렇게 됩니다\r\n"+d[1]+"\r\n\r\n■ 게임 안에서 켜고 끄는 곳\r\n"+d[2]+"\r\n\r\n■ 같이 못 쓰는 것·주의\r\n"+d[3];
  var a=appliedInfo!=null?appliedInfo[id-1]:null;
  if(loaded==null){detailTitle.Text="게임 적용 확인";detail.Text="게임 폴더를 먼저 확인해 주세요.";return;}
  if(!loaded.On[id-1]){detailTitle.Text="게임 적용 확인";detail.Text="이 번호는 설치되어 있지 않습니다.";return;}
  detailTitle.Text="게임 적용 확인 (마지막 실행"+(a!=null&&a.Time!=""?": "+a.Time:"")+")";
  string diag=DiagSummary();
  detail.Text=a==null?"기록을 읽지 못했습니다.":a.Detail()+(diag!=""?"\r\n"+diag:"")+"\r\n기록 위치: 게임 폴더\\"+Engine.RecordsDir+"\\ (적용확인.log · runtime.log · 각 모드 기록)";
 }
 // R43: 게임 안 종합 진단(SAN14_Diag, 시작 30·90초)의 마지막 결과
 string DiagSummary(){
  try{string ap=Path.Combine(CleanRoot(),Engine.ApplyLogRel.Replace('/','\\'));if(!File.Exists(ap))return "";
   var d=Engine.LastDiag(File.ReadAllText(ap,Encoding.UTF8));if(d==null)return "";
   var b=new StringBuilder();b.AppendLine("■ 게임 안 종합 진단 ("+d[0]+", "+(d[1]=="check90"?"시작 90초":"시작 30초")+"): "+d[2]);
   for(int i=3;i<d.Length&&i<3+12;i++)b.AppendLine("  ✖ "+d[i]);
   return b.ToString().TrimEnd();}
  catch(Exception){return "";}
 }
 void OpenRecords(){try{string dir=Path.Combine(CleanRoot(),Engine.RecordsDir);if(!Directory.Exists(dir)&&loaded!=null)dir=Make().CollectRecords(log.Text);if(Directory.Exists(dir))System.Diagnostics.Process.Start("explorer.exe","\""+dir+"\"");else MessageBox.Show(this,"아직 기록 폴더가 없습니다. 게임 폴더를 확인한 뒤 다시 눌러 주세요.");}catch(Exception e){ErrorBox.Show(this,"기록 폴더",ErrorBox.Describe("기록 폴더를 여는 중",Path.Combine(CleanRoot(),Engine.RecordsDir),e,null));}}
 static string Detect(){
  var candidates=new List<string>();
  try{foreach(string key in new[]{@"HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Valve\Steam",@"HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam",@"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam"}){
   string steam=Microsoft.Win32.Registry.GetValue(key,"InstallPath",null) as string??Microsoft.Win32.Registry.GetValue(key,"SteamPath",null) as string;
   if(String.IsNullOrEmpty(steam))continue;candidates.Add(Path.Combine(steam,"steamapps","common","Romance_of_the_Three_Kingdoms_14"));
   string vdf=Path.Combine(steam,"steamapps","libraryfolders.vdf");
   if(File.Exists(vdf))foreach(var m in System.Text.RegularExpressions.Regex.Matches(File.ReadAllText(vdf),"\"path\"\\s+\"([^\"]+)\"").Cast<System.Text.RegularExpressions.Match>())candidates.Add(Path.Combine(m.Groups[1].Value.Replace("\\\\","\\"),"steamapps","common","Romance_of_the_Three_Kingdoms_14"));
  }}catch{}
  candidates.Add(@"C:\Program Files (x86)\Steam\steamapps\common\Romance_of_the_Three_Kingdoms_14");
  // R40 A22: 스팀이 실제로 깔아 둔 라이브러리(appmanifest_872410.acf 가 있는 곳)를 먼저. 옛 복사본 폴더에 깔리는 일 막기
  var found=new List<string>();foreach(string c in candidates){try{string full=Path.GetFullPath(c).TrimEnd('\\');if(File.Exists(Path.Combine(full,"SAN14CE_KO.exe"))&&!found.Any(x=>x.Equals(full,StringComparison.OrdinalIgnoreCase)))found.Add(full);}catch{}}
  string pick=found.FirstOrDefault(c=>{try{string sa=Path.GetDirectoryName(Path.GetDirectoryName(c));return File.Exists(Path.Combine(sa,"appmanifest_872410.acf"));}catch{return false;}})??found.FirstOrDefault()??"";
  otherCopies=found.Where(c=>!c.Equals(pick,StringComparison.OrdinalIgnoreCase)).ToList();
  return pick;
 }
 void LoadState(){
  if(busy)return;loaded=null;appliedInfo=null;lastLoadError="";
  SetPathLabel();
  if(String.IsNullOrWhiteSpace(CleanRoot())||!Directory.Exists(CleanRoot())){stateText="상태: 게임 폴더를 찾지 못함 — [게임 폴더 변경…]으로 골라 주세요";stateWarn=true;ShowStatus();UpdateTabCounts();return;}
  try{
   var engine=Make();engine.Tx.Recover();if(engine.Tx.RecoverNote!="")WriteLog(engine.Tx.RecoverNote);var s=engine.Load();var st=engine.Status(s);loaded=s;loadedRoot=CleanRoot();
   Engine.AppliedInfo[] ap;try{ap=engine.AppliedDetail(s,Engine.RuntimeLogPath());}catch(Exception e){ap=null;WriteLog("게임 적용 기록을 읽지 못했습니다: "+e.Message);}
   appliedInfo=ap;int okN=0,partN=0,failN=0;
   for(int i=0;i<Spec.Count;i++){var row=grid.Rows[Spec.RowOf(i+1)];row.Cells[0].Value=false;row.Cells[3].Value=st[i];row.Cells[3].Style.ForeColor=st[i].StartsWith("설치됨")?Color.DarkGreen:st[i]=="미설치"?Color.DimGray:Color.DarkOrange;
    var a=ap!=null?ap[i]:null;string cell=a==null?"":a.Cell;row.Cells[4].Value=cell;row.Cells[4].ToolTipText=cell;
    Color col=Color.DimGray;if(a!=null){if(a.State=="OK"||a.State=="FILE"){col=Color.DarkGreen;okN++;}else if(a.State=="PART"){col=Color.FromArgb(196,127,0);partN++;}else if(a.State=="FAIL"){col=Color.Firebrick;failN++;}}
    row.Cells[4].Style.ForeColor=col;}
   int n=s.On.Count(x=>x);
   stateText="상태: "+(n==0?"설치된 항목 없음":"설치됨 "+(s.Installer!=""?s.Installer:"(판 모름)")+" ("+n+"개)")+(n>0?" · 게임 적용: 확인 "+okN+" / 일부 "+partN+" / 실패 "+failN:"")+(s.Version!=0&&s.Version!=7?" (이전 설치기 기록 — [선택 설치]를 누르면 지금 판용으로 갱신·정리합니다)":"");
   try{var miss=engine.MissingPayloads(Enumerable.Repeat(true,Spec.Count).ToArray());if(miss.Count>0&&!missNoted){missNoted=true;WriteLog("참고: 설치 자료 일부가 보이지 않습니다("+miss[0]+(miss.Count>1?" 외 "+(miss.Count-1)+"개":"")+"). 받은 ZIP 을 모두 같은 새 폴더에 풀어 주세요. 그 자료가 필요한 번호를 고르면 설치하지 않고 안내합니다.");}}catch(Exception){}
   try{var mm=engine.MissingModelPayloads();if(mm.Count>0&&!modelNoted){modelNoted=true;WriteLog("참고: 별도 ZIP 자료(10번 추가 3D 모델·31번 그래픽) "+mm.Count+"개가 보이지 않습니다. 이대로 설치하면 그 부분만 건너뜁니다(10번은 기본 모델 0~24만). 받은 ZIP 을 모두 같은 폴더에 풀어 주세요.");}}catch(Exception){}
   // R40 A22: 실행 파일·공통 파일 문제는 아래 기록줄만이 아니라 맨 위 상태줄에 빨갛게
   string exeKind=Engine.ExeKind(Path.Combine(CleanRoot(),"SAN14CE_KO.exe"));List<string> baseCh;try{baseCh=engine.BaseChanged(s);}catch(Exception){baseCh=new List<string>();}
   string warn=exeKind!=""?" · ⚠ "+exeKind:baseCh.Count>0?" · ⚠ 공통 파일 "+baseCh.Count+"개가 바뀜([선택 설치]로 복구)":"";
   if(s.RetiredOn){stateText+=" · 12번 안개는 이번 판에서 빠짐([선택 설치]를 누르면 지움)";WriteLog("참고: 12번(필드 안개)은 R41 부터 통합패치에서 빠졌습니다. 설치돼 있던 안개 파일은 [선택 설치]를 누르면 지웁니다. 미니맵은 27번(독립 미니맵)으로 쓰세요.");}
   stateText+=warn;stateWarn=warn!="";ShowStatus();UpdateTabCounts();UpdateLockRow();
   if(exeKind!="")WriteLog("주의: "+exeKind+". 통합패치는 스팀 원본 "+Spec.GameLabel+" 실행 파일(SAN14CE_KO.exe)에서만 켜집니다. 스팀 [게임 파일 무결성 확인]으로 원본을 되돌리세요.");
   if(baseCh.Count>0)WriteLog("주의: 통합패치 공통 파일이 설치 때와 다릅니다(다른 패치가 덮어썼을 수 있음): "+String.Join(", ",baseCh.Take(4).ToArray())+(baseCh.Count>4?" 외 "+(baseCh.Count-4)+"개":"")+" — [선택 설치]를 누르면 되돌립니다.");
   if(otherCopies.Count>0&&!copiesNoted){copiesNoted=true;WriteLog("참고: 게임 폴더가 여러 곳에 있습니다("+String.Join(" / ",otherCopies.ToArray())+"). 스팀이 실제로 실행하는 폴더에 설치해야 합니다.");}
   if(failN>0)WriteLog("게임 적용 실패 "+failN+"개: 목록에서 그 번호를 누르면 오른쪽에 이유가 나옵니다.");
   ShowDetail();
  }catch(Exception e){stateText="상태: 확인할 수 없음 — "+e.Message;stateWarn=true;ShowStatus();lastLoadError=e.Message;WriteLog(e.Message);}
 }
 void SetBusy(bool b){busy=b;foreach(Control c in new Control[]{grid,browse,refresh,rollbackBtn,updateBtn,tabAll,tabNone,tabInstall,tabRemove,allSel,allNone,allInstall,allRemove,updateInstalled,records,target,tabs})c.Enabled=!b;Cursor=b?Cursors.WaitCursor:Cursors.Default;if(!b)TabButtons();}
 // 선택 설치/제거 → 원하는 결과 집합(현재 설치 ∪ 선택 / 현재 설치 − 선택)으로 바꿔 기존 설치 엔진에 넘김
 void Start(bool install,bool tabOnly,bool[] forced=null){
  if(busy)return;
  if(loaded==null){MessageBox.Show(this,lastLoadError!=""?"게임 폴더를 확인하지 못했습니다:\n"+lastLoadError:"먼저 게임 폴더를 확인해 주세요.");return;}
  var sel=forced??Selected(tabOnly);
  if(!sel.Any(x=>x)){MessageBox.Show(this,install?"설치할 항목을 체크해 주세요.":"제거할 항목을 체크해 주세요.");return;}
  var wanted=Engine.Compose(loaded.On,sel,install);
  var engine=Make();
  if(install){
   Spec.ForeignFont=delegate(string root){return engine.IsForeignFont();};
   if(!ResolveConflicts(wanted,engine))return;
  }else{
   var removing=sel;string note=Spec.RemovalNote(loaded.On,removing);
   string list=String.Join(", ",Spec.Order.Where(id=>removing[id-1]&&loaded.On[id-1]).Select(id=>Spec.Num(id)+"번").ToArray());
   if(list=="")list="(설치된 항목 없음)";
   bool all=!wanted.Any(x=>x);
   if(ErrorBox.Ask(this,"선택 제거",(all?"설치된 패치를 모두 제거하고 게임 데이터 파일을 "+Spec.GameLabel+" 원본으로 되돌립니다.\r\n":"다음 항목을 제거합니다: "+list+"\r\n")+(note!=""?"\r\n같이 바뀌는 것:\r\n"+note:"")+"\r\n사용자 음원·저장 데이터·다른 패치 파일은 건드리지 않습니다. 계속할까요?",new[]{"제거","취소"})!=0)return;
  }
  Run(wanted,engine,install);
 }
 bool ResolveConflicts(bool[] wanted,Engine engine){
  string root=CleanRoot();var notices=new List<string>();var resolved=new HashSet<string>();
  foreach(var c in Spec.Conflicts){
   if(!c.Relevant(loaded.On,wanted))continue; // 이번에 새로 까는 번호와 관계없는 충돌·알림은 안 띄움
   string msg=c.Check(wanted,root);if(msg==null)continue;
   if(c.Kind==Spec.ConflictKind.Notice){notices.Add("· "+c.Title+": "+c.Message);continue;}
   int pick=ErrorBox.Ask(this,c.Title,msg,c.Choices);
   WriteLog("충돌 "+c.Key+": "+(pick<0?"취소":c.Choices[pick]));
   if(!Spec.ApplyChoice(c.Key,pick,wanted,resolved))return false;
   if(c.Key=="20-font"&&resolved.Contains("20-font"))engine.ForceFont=true;
  }
  // 선택이 바뀌었으면 다시 한 번(빼기로 다른 충돌이 사라질 수 있음) — 차단 규칙만 재검사, "둘 다 설치"로 해결한 규칙은 제외
  var left=Spec.RemainingBlock(wanted,root,resolved,loaded.On);
  if(left!=null){MessageBox.Show(this,"아직 같이 못 쓰는 조합이 남아 있습니다: "+left.Title);return false;}
  if(notices.Count>0&&ErrorBox.Ask(this,"참고",String.Join("\r\n\r\n",notices.ToArray())+"\r\n\r\n이대로 설치할까요?",new[]{"설치","취소"})!=0)return false;
  return true;
 }
 void Run(bool[] on,Engine engine,bool install){
  SetBusy(true);
  {var fixedOn=Spec.WithSettingsWindow(on);if(fixedOn[0]&&!on[0])WriteLog("1번(모드 설정 창)은 다른 번호의 게임 안 스위치가 모두 들어 있어서 같이 깔립니다(20·31번만 깔 때만 빠짐).");on=fixedOn;}
  var ids=Spec.Order.Where(id=>on[id-1]).Select(id=>Spec.Num(id)).ToArray();
  WriteLog((install?"선택 설치 시작 → 적용 후 설치돼 있을 항목: ":"선택 제거 시작 → 남는 항목: ")+(ids.Length==0?"없음":String.Join(", ",ids)+"번"));
  var worker=new BackgroundWorker();Exception error=null;
  worker.DoWork+=delegate{try{engine.Install(on,delegate(int i,int total){if(i%20==0||i==total)WriteLog("파일 처리 "+i+"/"+total);});}catch(Exception e){error=e;}};
  worker.RunWorkerCompleted+=delegate{
   SetBusy(false);
   try{engine.CollectRecords(log.Text);}catch(Exception){}
   if(error!=null){
    WriteLog(error.Message);
    var ie=error as InstallException;
    ErrorBox.Show(this,install?"설치하지 않음":"제거하지 않음",ErrorBox.Describe(install?"선택한 항목을 설치하는 중":"선택한 항목을 제거하는 중",ie!=null?ie.Target:null,error,
     ie!=null?Fix(ie):(error.Message.Contains(Engine.GameRunningMark)?"게임과 런처 창을 닫고 다시 눌러 주세요. 창이 안 보이는데도 이 글이 뜨면 작업 관리자(Ctrl+Shift+Esc)의 세부 정보에서 위에 적힌 이름을 골라 [작업 끝내기]를 한 뒤 다시 눌러 주세요.":"게임과 런처를 닫고, ZIP 둘을 같은 새 폴더에 풀었는지, 게임 폴더에 쓰기 권한이 있는지 확인한 뒤 다시 눌러 주세요.")));
   }
   else{
    var b=new StringBuilder();b.AppendLine(install?"설치했습니다. 게임을 실행해 확인해 주세요.":(on.Any(x=>x)?"제거했습니다.":"모두 제거했습니다."));
    if(engine.ModelSkip!="")b.AppendLine("· 10번 추가 모델은 건너뜀: "+engine.ModelSkip);
    if(engine.ScreenSkip!="")b.AppendLine("· 31번 그래픽은 건너뜀: "+engine.ScreenSkip);
    if(install)b.AppendLine("게임 안 정보창 → 모드 버튼에서 세부 스위치를 바꿀 수 있습니다. 게임을 한 번 실행한 뒤 [상태 새로고침]을 누르면 번호별 적용 결과가 나옵니다.");
    MessageBox.Show(this,b.ToString(),"완료");
   }
   LoadState();worker.Dispose();
  };
  worker.RunWorkerAsync();
 }
 // ---------- R43.28: 상태 줄·탭 숫자·잠금 표시·업데이트 ----------
 static string MarkText(int id){string m=Spec.Mark(id);return m==""?"":"  "+m;}
 bool LockedSettings(){if(loaded==null)return false;for(int i=1;i<Spec.Count;i++)if(loaded.On[i]&&Spec.UsesSettingsWindow(i+1))return true;return false;}
 void UpdateLockRow(){
  var row=grid.Rows[Spec.RowOf(1)];bool locked=LockedSettings();
  row.Cells[2].Value=(locked?"【다른 번호가 깔려 있어 함께 설치】 ":"")+Spec.Names[0]+MarkText(1);
  row.Cells[1].Style.ForeColor=row.Cells[2].Style.ForeColor=locked?Color.Gray:SystemColors.ControlText;
 }
 void UpdateTabCounts(){
  bool[] on=loaded!=null?loaded.On:null;
  Func<int[],string> cnt=ids=>on==null?ids.Length.ToString():ids.Count(id=>on[id-1])+"/"+ids.Length;
  tabs.TabPages[0].Text="전체 "+cnt(Enumerable.Range(1,Spec.Count).Where(id=>!Spec.Retired(id)).ToArray());
  for(int c=0;c<Spec.CategoryNames.Length;c++)tabs.TabPages[c+1].Text=Spec.CategoryNames[c]+" "+cnt(Spec.InCategory(c));
 }
 void SetPathLabel(){pathLabel.Text="게임 폴더: "+(CleanRoot()==""?"(찾지 못함 — [게임 폴더 변경…]으로 고르기)":CleanRoot())+"     ·     이 설치기: "+package;}
 void ShowStatus(){
  bool run=false;try{var ps=System.Diagnostics.Process.GetProcessesByName("SAN14CE_KO");run=ps.Length>0;foreach(var p in ps)p.Dispose();}catch(Exception){}
  string text=stateText+"   ·   "+updateText+"   ·   게임: "+(run?"켜짐":"종료됨");
  if(summary.Text!=text)summary.Text=text;summary.ForeColor=stateWarn?Color.Firebrick:SystemColors.ControlText;
 }
 void LoadRecordText(){
  try{string p=Path.Combine(Path.Combine(CleanRoot(),Engine.RecordsDir),"설치기.log");
   if(CleanRoot()==""||!File.Exists(p)){recordText.Text="아직 설치 기록이 없습니다. [선택 설치]·[선택 제거]를 하면 그때의 작업 기록이 여기 나옵니다.\r\n(기록 위치: 게임 폴더\\"+Engine.RecordsDir+"\\설치기.log)";return;}
   string s=File.ReadAllText(p,Encoding.UTF8);if(s.Length>200000)s=s.Substring(s.Length-200000);
   recordText.Text="마지막 설치·제거 작업 기록 ("+File.GetLastWriteTime(p).ToString("yyyy-MM-dd HH:mm")+")\r\n"+p+"\r\n\r\n"+s.Replace("\r\n","\n").Replace("\n","\r\n");
  }catch(Exception e){recordText.Text="작업 기록을 읽지 못했습니다: "+e.Message;}
 }
 // 설치기를 켤 때 한 번만 깃허브 배포 목록을 읽는다(계속 읽지 않음)
 void StartUpdateCheck(){
  updateText="공개판 확인 중…";ShowStatus();
  var th=new Thread(delegate(){
   List<Release> got=null;string err="";
   try{got=Updates.Fetch();}catch(Exception e){err=e.Message;}
   try{BeginInvoke(new Action(delegate{OnReleases(got,err);}));}catch(Exception){}
  });
  th.IsBackground=true;th.Start();
 }
 void OnReleases(List<Release> got,string err){
  releases=got;
  if(got==null){updateText="공개판 확인 불가";WriteLog("깃허브 공개판 확인 실패(설치·제거는 그대로 쓸 수 있습니다): "+err);}
  else{
   var l=Updates.Latest(got);
   if(l==null)updateText="공개판 아직 없음";
   else if(Updates.Compare(l.Tag,Spec.Version)>0){
    updateText="공개 최신: "+l.Tag+" ● 업데이트 가능";updateBtn.Text="업데이트  ● 새 판 "+l.Tag;updateBtn.BackColor=Color.FromArgb(255,236,200);updateBtn.ForeColor=Color.FromArgb(150,70,0);
    tabs.TabPages[UpdateTab].Text="업데이트 ●";WriteLog("새 판이 있습니다: "+l.Tag+" ("+l.Date+") — [업데이트] 버튼");
   }
   else updateText="공개 최신: "+l.Tag+(l.Tag==Spec.Version?" (지금 판이 최신)":"");
  }
  ShowStatus();if(SpecialTab()==1)FillReleases();
 }
 void FillReleases(){
  relList.Items.Clear();
  relHead.Text="지금 설치기: "+Spec.Version+"   ·   "+updateText;
  if(releases==null){relNotes.Text="깃허브에서 판 목록을 아직 읽지 못했습니다(인터넷 연결 확인). [업데이트]를 누르면 다시 확인합니다.\r\n\r\n깃허브: "+Updates.PagesUrl;return;}
  var latest=Updates.Latest(releases);int pick=-1;
  foreach(var r in releases){if(!r.HasPatch)continue;r.Mark=(r==latest?"   (최신판)":"")+(r.Tag==Spec.Version?"   (지금 설치기)":"");int k=relList.Items.Add(r);if(r.Tag==Spec.Version||pick<0&&r==latest)pick=k;}
  if(relList.Items.Count==0){relNotes.Text="깃허브에 올라온 판이 아직 없습니다.\r\n\r\n깃허브: "+Updates.PagesUrl;return;}
  relList.SelectedIndex=pick>=0?pick:0;
 }
 bool EnsureReleases(){
  if(releases!=null)return true;
  Cursor=Cursors.WaitCursor;
  try{OnReleases(Updates.Fetch(),"");return true;}
  catch(Exception e){ErrorBox.Show(this,"공개판 확인 실패",ErrorBox.Describe("깃허브에서 통합패치 판 목록을 읽는 중",Updates.PagesUrl,e,"인터넷 연결을 확인한 뒤 다시 눌러 주세요. 회사·학교 망이나 백신이 깃허브(raw.githubusercontent.com)를 막으면 이 오류가 납니다."));return false;}
  finally{Cursor=Cursors.Default;}
 }
 void DoUpdate(){
  if(busy||!EnsureReleases())return;
  var l=Updates.Latest(releases);
  if(l==null||Updates.Compare(l.Tag,Spec.Version)<=0){MessageBox.Show(this,"지금 설치기("+Spec.Version+")가 공개 최신판입니다."+(l==null?"":"\r\n(깃허브 최신: "+l.Tag+")"),"업데이트");return;}
  if(!ReleasePicker.Confirm(this,"새 판 받기","통합패치 "+l.Tag+" 새 판이 있습니다 ("+l.Date+").\r\n받아서 지금 폴더 옆 새 폴더에 만들고 새 판 설치기를 엽니다. 지금 폴더와 게임은 그대로 둡니다.",l.Title+"\r\n\r\n"+l.Notes,"받기"))return;
  InstallRelease(l);
 }
 void DoRollback(){
  if(busy||!EnsureReleases())return;
  var r=ReleasePicker.Pick(this,releases,CleanRoot()!=""?Updates.ExeStamp(CleanRoot()):0u);
  if(r!=null)InstallRelease(r);
 }
 void InstallRelease(Release r){
  if(loaded==null){MessageBox.Show(this,"먼저 게임 폴더를 확인해 주세요([게임 폴더 변경…]).");return;}
  uint gs=Updates.ExeStamp(CleanRoot());
  if(gs!=0&&r.Stamp!=gs){ErrorBox.Show(this,"설치할 수 없는 판",ErrorBox.Describe("통합패치 "+r.Tag+" 를 고르는 중",null,null,"이 판은 "+Updates.StampText(r.Stamp)+" 전용인데, 게임은 "+Updates.StampText(gs)+" 입니다. 게임판에 맞는 판을 골라 주세요."));return;}
  SetBusy(true);string game=CleanRoot();string dest=null;Exception error=null;
  WriteLog("새 판 받기 시작: "+r.Tag+" ("+r.AssetName+")");
  // R43.30(사용자 11-11): 진행 창(단계·막대·%·지금 하는 일·남은 시간·취소)
  var prog=new UpdateProgress();var form=new ProgressForm(r.Tag,prog);
  var worker=new BackgroundWorker();
  worker.DoWork+=delegate{try{dest=Updates.Build(r,releases,package,delegate(string m){WriteLog(m);},prog);}catch(Exception e){error=e;}};
  worker.RunWorkerCompleted+=delegate{
   worker.Dispose();
   bool cancelled=error is OperationCanceledException;
   form.Finish(error==null);
   if(cancelled)WriteLog("새 판 받기를 취소했습니다. 만들던 새 폴더는 지웠고, 지금 폴더와 게임은 그대로입니다.");
   else if(error!=null){WriteLog(error.Message);ErrorBox.Show(this,"새 판 받기 실패",ErrorBox.Describe("통합패치 "+r.Tag+" 새 판을 받는 중",null,error,"인터넷 연결과 지금 폴더 옆에 쓰기 권한이 있는지 확인한 뒤 다시 눌러 주세요. 지금 폴더와 게임은 바뀌지 않았습니다. 계속 안 되면 깃허브("+Updates.PagesUrl+")의 updates 폴더에서 직접 받을 수 있습니다."));}
  };
  worker.RunWorkerAsync();
  form.ShowDialog(this);
  SetBusy(false);form.Dispose();
  if(error!=null||dest==null||!form.OpenNew){if(error==null&&dest!=null)WriteLog("새 판 폴더를 만들었습니다: "+dest+" — 그 폴더의 설치.cmd 로 열 수 있습니다.");return;}
  try{Updates.Launch(dest,game,Spec.Version);}
  catch(Exception e){ErrorBox.Show(this,"새 판 설치기 열기 실패",ErrorBox.Describe("새 판 설치기를 여는 중",dest,e,"새 폴더의 설치.cmd 를 직접 실행해 주세요."));return;}
  Close();
 }
 void AfterUpdatePrompt(){
  if(loaded==null)return;int n=loaded.On.Count(x=>x);
  if(n==0){MessageBox.Show(this,"통합패치 "+Spec.Version+" 새 판 설치기입니다(이전 설치기 "+Program.AfterUpdate+").\r\n게임에 깔린 항목이 없으니 원하는 번호를 골라 [선택 설치]를 눌러 주세요.","새 판");return;}
  if(ErrorBox.Ask(this,"새 판으로 갱신","통합패치 "+Spec.Version+" 새 판 설치기입니다(이전 설치기 "+Program.AfterUpdate+").\r\n지금 게임에 깔려 있는 "+n+"개 항목을 새 판 파일로 갱신할까요?\r\n안 깔린 번호는 건드리지 않습니다. 게임과 런처는 닫아 주세요.",new[]{"갱신","나중에"})==0)Start(true,false,(bool[])loaded.On.Clone());
 }
 static string Fix(InstallException e){
  string m=e.Message;
  if(m.Contains("게임과 게임 런처"))return "게임과 런처를 닫은 뒤 다시 눌러 주세요.";
  if(m.Contains("같은 새 폴더"))return "ZIP 을 전부(1of n … n of n) 같은 새 폴더에 풀고 다시 실행해 주세요. 백신이 파일을 지웠으면 예외 처리 후 다시 풀어 주세요.";
  if(m.Contains("손상"))return "압축을 새 폴더에 다시 풀어 주세요.";
  if(m.Contains(" 전용입니다")||m.Contains("확인할 수 없습니다"))return "Steam 에서 게임을 최신("+Spec.GameLabel+")으로 업데이트한 뒤 다시 실행해 주세요.";
  if(m.Contains("다른 패치")||m.Contains("다른 프로그램"))return "그 파일을 쓰는 다른 패치를 먼저 지우거나, 해당 번호의 체크를 풀고 설치해 주세요.";
  if(m.Contains("무결성"))return "Steam 라이브러리 → 게임 속성 → 설치된 파일 → 무결성 확인을 한 뒤 다시 설치해 주세요.";
  return "오류 글자를 댓글로 알려 주세요.";
 }
}
}
