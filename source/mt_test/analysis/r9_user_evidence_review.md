# R9: R8 follow-up evidence

Base: our shipped R8, feature SHA-256 f5b189e0de4488a61f9ebcf99fcced140a33f271051f14830e8a9c6667728e26.
The two latest videos and log ZIP 20260920-054806-878 are R8 execution evidence.
Log session 8827701747065937742 matches 용장/붕벽 and 감지/산전.
Back Yes/No are visible. Yes reaches the map then process exits. Decision highlights but no award appears.
Logs reach UI_UNREGISTER_END, then UI_UNREGISTER_DUPLICATE_IGNORED; this guard did NOT solve the crash.
No CONFIRM_BEGIN or rejection log exists in R8. R8 hook_confirm's early tree rejection was silent.

## Confirmed selection-owner defect

Preserved native constructor 0x6ef260 uses RSI as main merit layout. It passes RSI as row parent at
0x6ef430..442 and 0x6ef49c..4b0, storing rows at +0x1f8 and +0x200.
Native refresh 0x781810 reads the selection map from this same object's +0x198 at 0x78190e,
and passes its +0x1f8/+0x200 rows to refresh at 0x7819a3..9c0.
The constructor separately allocates 0x198 bytes for +0x1d8 at 0x6f0194..1c2 and attaches transition
participants at 0x6f01c8 onward. Original close 0x6be850 uses that transition object.
R8 passed +0x1d8 to choice_has_changes and choice_cancel_all. The former silently rejected Decision.
The latter read beyond the transition object's native size and could not restore the actual choices.
R5's note and fixture invented an inner selection-list owner at +0x1d8. R6-R8 inherited that error.
R9 uses main layout with primary-vtable validation. Preview.native_owner already correctly came from row+8.

## Confirmed Hover argument defect

The unchanged shipped Hover DLL SHA-256 is 2926c3ef05abd004bf0e5d9f7113321568800a6cf4d60b7e7e0bf25367b0f164.
Its release at +0x5ce0 saves the THIRD Windows x64 argument (R8 register) at +0x5ced,
and forwards it into popup vslot0x28 at +0x5d74..77 when destroy=1.
Our hook_row_destroy received native resource context but called shared_hover_release(row,1,0),
then correctly passed resources to the toolbar and original row. R9 forwards the same resources to Hover.
No CONTROLS_RELEASE_BEGIN appeared in R8 crash logs; Hover release occurs immediately before that marker.
This is consistent with a Hover cleanup failure but is NOT a crash-address/stack proof.

## Checks independent of the bad assumptions

Fixture now has MAIN layout as both row parent and selection owner; +0x1d8 is a separate 0x198-byte allocation.
The negative control loads the immutable R8 feature/resolver and current feature/resolver into the same fixture.
It observes R8 commit count0, failed choice restoration and actual Hover release receiving NULL.
Current results are commit count1, restored choice tree and preserved resource context.
The exact unchanged Hover +0x5ce0 machine code executes; only its native popup cleanup endpoint is substituted.
Latest seeds reproduce 11 용장, 8 붕벽, 189 감지 and 69 산전 with the supplied owned-trait/family inputs.
No probability tuning or unrelated source rewrite is included.

## Limits

No Windows game/GPU/allocator or original bottom physical event dispatch has run here.
R8 test_r8.py's synthetic code1 shared-event case remains a routing regression, NOT proof the actual Decision
button emits code1. Decision is tested at the preserved native state tick / hook entry with corrected ownership.
Full live crash cause or absence cannot be certified from the supplied log. New logs name confirm/cleanup stages;
read-only collector also includes bounded SAN14 Application Error metadata, not memory dumps or a stack trace.
