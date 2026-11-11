#include <stdint.h>
typedef uint64_t U;
typedef U(*D)(U,U,U,U,U,U,U,U,U);
static D d;
void set_dispatch(D p){d=p;}
__attribute__((ms_abi)) U Sound(U a0,U a1,U a2,U a3){return d(0,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U TextParam(U a0,U a1){return d(1,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U SafeFlush(U a0){return d(2,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U AIEligible(U a0){return d(3,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U SteamContext(U a0){return d(4,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U SteamID(U a0,U a1){return d(5,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U HoverShow(U a0,U a1){return d(6,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U HoverRelease(U a0,U a1,U a2){return d(7,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U ResetText(U a0){return d(8,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Speaker(U a0,U a1,U a2){return d(9,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Talk(U a0,U a1){return d(10,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U App(void){return d(11,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Input(void){return d(12,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Target(U a0){return d(13,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U ManagerText(U a0,U a1){return d(14,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FindFirstFileW(U a0,U a1){return d(15,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FindNextFileW(U a0,U a1){return d(16,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FindClose(U a0){return d(17,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U QueryPerformanceCounter(U a0){return d(18,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U QueryPerformanceFrequency(U a0){return d(19,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U RewardName(U a0){return d(20,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U RadioEnable(U a0,U a1,U a2){return d(21,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Selected(U a0,U a1){return d(22,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U AcquireSRWLockExclusive(U a0){return d(23,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U ReleaseSRWLockExclusive(U a0){return d(24,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CloseHandle(U a0){return d(25,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CreateDirectoryW(U a0,U a1){return d(26,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CreateFileW(U a0,U a1,U a2,U a3,U a4,U a5,U a6){return d(27,a0,a1,a2,a3,a4,a5,a6,0);}
__attribute__((ms_abi)) U DeleteFileW(U a0){return d(28,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FlushFileBuffers(U a0){return d(29,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FlushInstructionCache(U a0,U a1,U a2){return d(30,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U GetCurrentProcess(void){return d(31,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetCurrentThreadId(void){return d(32,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetEnvironmentVariableW(U a0,U a1,U a2){return d(33,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U GetFileSizeEx(U a0,U a1){return d(34,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetLastError(void){return d(35,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetModuleFileNameW(U a0,U a1,U a2){return d(36,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U GetModuleHandleW(U a0){return d(37,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetProcAddress(U a0,U a1){return d(38,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetTickCount64(void){return d(39,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U LoadLibraryExW(U a0,U a1,U a2){return d(40,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U MoveFileExW(U a0,U a1,U a2){return d(41,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U ReadFile(U a0,U a1,U a2,U a3,U a4){return d(42,a0,a1,a2,a3,a4,0,0,0);}
__attribute__((ms_abi)) U VirtualProtect(U a0,U a1,U a2,U a3){return d(43,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U WriteFile(U a0,U a1,U a2,U a3,U a4){return d(44,a0,a1,a2,a3,a4,0,0,0);}
__attribute__((ms_abi)) U MessageBoxW(U a0,U a1,U a2,U a3){return d(45,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U BCryptGenRandom(U a0,U a1,U a2,U a3){return d(46,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U VirtualAlloc(U a0,U a1,U a2,U a3){return d(47,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U VirtualFree(U a0,U a1,U a2){return d(48,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U VirtualQuery(U a0,U a1,U a2){return d(49,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U GetAsyncKeyState(U a0){return d(50,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetForegroundWindow(void){return d(51,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetWindowThreadProcessId(U a0,U a1){return d(52,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetCurrentProcessId(void){return d(53,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Readable(U a0,U a1){return d(54,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Patch(U a0,U a1,U a2){return d(55,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Relay(U a0,U a1){return d(56,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U FreeRelay(U a0){return d(57,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Log(U a0){return d(58,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Random(U a0,U a1){return d(59,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Valid(U a0){return d(60,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Eligible(U a0){return d(61,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U RowRefresh(U a0){return d(62,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Status(U a0,U a1,U a2){return d(63,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Confirm(U a0){return d(64,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U StreamOpen(U a0,U a1,U a2,U a3,U a4,U a5,U a6){return d(65,a0,a1,a2,a3,a4,a5,a6,0);}
__attribute__((ms_abi)) U Load(U a0,U a1,U a2){return d(66,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Newgame(U a0,U a1){return d(67,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U SavePrep(U a0,U a1,U a2,U a3,U a4){return d(68,a0,a1,a2,a3,a4,0,0,0);}
__attribute__((ms_abi)) U SaveEnd(U a0,U a1){return d(69,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Bonus(U a0,U a1){return d(70,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CaptionColor(U a0,U a1,U a2){return d(71,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U LabelColor(U a0,U a1){return d(72,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U UIExit(U a0){return d(73,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Text(U a0,U a1){return d(74,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Visible(U a0,U a1){return d(75,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Enabled(U a0,U a1){return d(76,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetForce(U a0){return d(77,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetRuler(U a0){return d(78,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Policy(U a0){return d(79,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Advance(U a0){return d(80,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Pay(U a0,U a1){return d(81,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Extra(void){return d(82,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U NativeWrite(U a0,U a1,U a2,U a3){return d(83,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U ApplyUI(void){return d(84,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U NativeCommit(U a0){return d(85,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U StateTick(U a0){return d(86,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U NativeClose(U a0,U a1){return d(87,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PresentResume(void){return d(88,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PresentFinish(void){return d(89,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PopState(U a0){return d(90,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Manager(void){return d(91,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U ButtonEvent(U a0,U a1){return d(92,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U RowDestroy(U a0,U a1){return d(93,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CreateEventW(U a0,U a1,U a2,U a3){return d(94,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U CreateThread(U a0,U a1,U a2,U a3,U a4,U a5){return d(95,a0,a1,a2,a3,a4,a5,0,0);}
__attribute__((ms_abi)) U WaitForSingleObject(U a0,U a1){return d(96,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U SetEvent(U a0){return d(97,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U GetSystemTimeAsFileTime(U a0){return d(98,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U ChoiceCounts(U a0,U a1,U a2){return d(99,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U ChoiceRefresh(U a0){return d(100,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U NativeLocate(U a0,U a1){return d(101,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetAlloc(U a0){return d(102,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetFree(U a0){return d(103,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetCtor(U a0){return d(104,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetInit(U a0,U a1,U a2,U a3,U a4,U a5,U a6){return d(105,a0,a1,a2,a3,a4,a5,a6,0);}
__attribute__((ms_abi)) U WidgetText(U a0,U a1){return d(106,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetBind(U a0,U a1,U a2){return d(107,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U CallbackType(void){return d(108,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U DynamicText(U a0,U a1,U a2,U a3,U a4,U a5){return d(109,a0,a1,a2,a3,a4,a5,0,0);}
__attribute__((ms_abi)) U PrimText(U a0,U a1){return d(110,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PrimSize(U a0,U a1,U a2){return d(111,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U PrimPos(U a0,U a1,U a2){return d(112,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Align(U a0,U a1){return d(113,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetRegister(U a0){return d(114,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetUnregister(U a0){return d(115,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetDestroy(U a0,U a1){return d(116,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetDelete(U a0,U a1){return d(117,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetLayer(U a0,U a1){return d(118,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U Font(U a0,U a1){return d(119,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionSize(U a0){return d(120,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionReserve(U a0,U a1){return d(121,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CancelButton(U a0,U a1){return d(122,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U ConsumeInput(void){return d(123,0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U TraitAt(U a0,U a1){return d(124,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectTraits(U a0,U a1,U a2,U a3,U a4,U a5,U a6){return d(125,a0,a1,a2,a3,a4,a5,a6,0);}
__attribute__((ms_abi)) U ColumnFill(U a0,U a1,U a2,U a3){return d(126,a0,a1,a2,a3,0,0,0,0);}
__attribute__((ms_abi)) U ColumnOriginal(U a0){return d(127,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U InfoOriginal(U a0){return d(128,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U InfoPerson(U a0){return d(129,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U LabelGet(U a0,U a1){return d(130,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U InfoCache(U a0,U a1){return d(131,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U InfoBuild(U a0,U a1){return d(132,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U UnitPerson(U a0){return d(133,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PresetsRefresh(U a0){return d(134,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U PresetsEntity(U a0,U a1,U a2){return d(135,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U PresetsStatus(U a0){return d(136,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U LayoutCtor(U a0){return d(137,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionCtor(U a0){return d(138,a0,0,0,0,0,0,0,0);}
__attribute__((ms_abi)) U BalloonInit(U a0,U a1){return d(139,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U BalloonAlpha(U a0,U a1){return d(140,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionWidth(U a0,U a1,U a2){return d(141,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionHeight(U a0,U a1,U a2){return d(142,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U CollectionVisible(U a0,U a1,U a2){return d(143,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U Window(U a0,U a1,U a2,U a3,U a4){return d(144,a0,a1,a2,a3,a4,0,0,0);}
__attribute__((ms_abi)) U PrimAlpha(U a0,U a1){return d(145,a0,a1,0,0,0,0,0,0);}
__attribute__((ms_abi)) U WidgetPosition(U a0,U a1,U a2){return d(146,a0,a1,a2,0,0,0,0,0);}
__attribute__((ms_abi)) U NativePicker(U a0){return d(147,a0,0,0,0,0,0,0,0);}
U call8(U f,U a,U b,U c,U e,U g,U h,U i,U j){return ((U(__attribute__((ms_abi))*)(U,U,U,U,U,U,U,U))f)(a,b,c,e,g,h,i,j);}