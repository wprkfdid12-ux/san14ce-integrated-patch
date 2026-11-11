# R8: latest R7 reproduction and incremental repair

Input: user video `22(3).mp4` (26.517 seconds) and supplied log ZIP
`b2d1f92d-4457-4365-b333-bb5605c5469c.zip`. Read locally from upload.
Video shows Decision highlighted, a discard warning without visible Yes/No,
blank-area clicks, then a return to the map before the recording ends. The user
reports a crash; neither file provides a crash stack or faulting instruction.

Latest relevant event session: 8234258782607733590, mod RC4-R7-NativeDialogs.
Zhao Yun596: preview seed7476374893248464071 -> 견뢰130,
reroll9106355430018118111 -> 원시6, one ticket used.
Jo Gwang619: preview11544960271038789809 -> 보좌192,
reroll5350142522450800474 -> 독려46, one ticket used.
Zhao Yun owned nine traits; the event stream does not prove which old trait was
chosen later for replacement. R8 regression explicitly chooses 상승161 to test
that branch; this is a test input, not an invented observation from the video.
Runtime records BACK_QUEUED / BACK_NOTICE_OPEN / CANCEL_YES / CANCEL_TICK and
repeated UI_UNREGISTER; it does not show CONFIRM_BEGIN in that failed attempt.

## Confirmed routing defects

1. Captured original 0x6be850 stores the supplied end-event code and starts the
   native fade. R7 intercepted EVERY code when a preview existed and queued Back.
   R8 handles only 0x7fffffff as explicit cancel; other native codes pass intact.
2. Constructor 0x6efed3 stores a 0x188-byte bottom command GROUP at layout+0x1c8.
   Original code configures multiple command resources on it. R7's hit fallback
   treated the full group rectangle as a single Back button. R8 removes this
   fallback entirely. The original command dispatcher owns Decision and Back.
3. R7 accepted a popup root hit as Yes/No if it fell in a button's calculated
   rectangle, even though no button was visible. R8 requires a native button
   target/descendant as well as its rectangle; blank window hits grant no consent.

The immutable delivered R7 DLL reproduces defects 1 and 2; the exact R8 DLL
rejects both (`audit/r8_negative_control.json`). This is a DLL harness result,
not a Windows/game run. Earlier tests directly wrote layout state1 for Decision
and therefore bypassed both defects. New R8 tests run the captured shared native
end-event bytes, model a whole bottom group and defer the native fade endpoint.

## Popup and lifecycle changes

The text root remains CSimpleBalloonLayout. Native Yes/No/OK controls now have
no popup parent and use the same layer6, registered after the text root. Their
absolute positions are recalculated with the window before showing. This removes
the old popup-parent/layer dependency. Actual GPU visibility is not certified.

Yes is queued until a later tick after mouse/Enter release. Back starts only the
original cancel fade; there is no extra state pop. Late callbacks during closing
are ignored. Original UI register (vtable0x1447cc8 -> 0x6b4f20, verified pointer)
starts a new lifecycle. Duplicate unregister of that layout is ignored until its
next registration, preventing repeated child detach. Resource release still uses
the original row cleanup context. No native selection counters are zeroed by hand.

These address concrete invalid input and lifetime paths. The exact fault causing
the reported full-game crash remains unconfirmed without a crash stack/live run.

## Preserved

Original two-row shared rank allowance; per-ID stage state; five personal tickets;
Back discards pending results without refund; Decision alone applies rewards;
replacement consent; original DB comparison; Hover/X/passion/RC25/save integration.
Gacha weights, golden probability settings and reroll allowance are unchanged.
