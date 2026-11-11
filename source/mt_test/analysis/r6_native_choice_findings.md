# R6 source and evidence notes

This is our R5-SafeExit_Confirm continuation. Other-chat videos are reference failure scenarios, not execution evidence for R6.

The preserved original renderer at 0x8a2fdc..0x8a3217 independently identifies the key R5 mistake: status 0 disables the button and radio entry; status 1 enables it and computes selected state from two original upper/lower counts. R5 interpreted 0 as an available action and required it to forward the original selection click. That could enable future choices, fail to register the current result and erase the native selection color.

The preserved 0x770080 handler reads officer ID through the clicked native row. Its map node at +0x20 stores upper in the low nibble and domestic in the high nibble. It increments a prefix on the next button, or truncates it on a selected button, then invokes 0x781810. R6 calls this handler rather than fabricating a separate rank/fee update. The original renderer uses CCmnButton vslot 0x208 for either row's selected appearance.

R6 reads counts through verified 0x8a2f87 -> 0x6dbdc0 and refreshes through verified 0x770236 -> 0x781810. SelectionConfig is appended to the private resolver. Before installing hooks, both exact callsites and in-image readable targets must verify. Existing hook count and common DLL files do not change.

The shared rank limit and left-prefix rule apply to both rows. The three upper GSlots record separate results; they are not a substitute for native upper+lower counts. Original auto-selection may select unrolled upper rewards; Decision cannot commit those until the corresponding draws exist. Deselected previews are excluded from scratch preparation. Domestic-only Decision after drawing/deselecting an upper reward still invokes the original native award chain.

The latest Back contract discards draws without refunding successful personal rerolls. Consent happens on a safe tick. R6 restores transient native map choices to earned counts, calls original refresh, detaches owned UI and invokes the original cancel trigger once. It never combines native Decision with native cancel. The original F_CLOSE guard (+0x178/+0x17c of the inner layout) is retained, not forcibly cleared. Native fade, busy duration and real Windows lifetime still require actual game verification.

Replacement consent is separate from Back consent. The scratch transaction lists officer, old trait and new trait for each actual replacement; every pair needs Yes at the same choice revision before Decision commits once. Any reroll/choice change invalidates consent.

The R6 tests execute the production PE through an MS ABI bridge. Captured original button, tick, reward-renderer and close instructions execute too. OS/Steam, status/getter/refresh, widgets, costs and fade endpoints are substituted. The status model follows the independent captured renderer; it is not evidence that the entire Windows game has run.

Replacement consent also hashes the exact old/new notice list before and after final preparation. Native auto-selection or external UI changes that bypass callback revisions cannot reuse consent for a different replacement list.
