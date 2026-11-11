# iOS build 31: shared window and card canvas audit

Build 30 did not cover the actual common card dialog/progress/file-list compositors. Its legacy card-manager scope was not evidence that postrace Save Replay was handled.

The private corpus inventory scans all 7,377 generated functions and 27,356 direct call edges, including all display modules. It identifies 602 functions containing indirect calls; static reachability is not a claim that all screens have been visually tested.

## Changes

- Scope actual card dispatch (818B6B28), progress/status (818B8EDC), file browser (818BC310), and standalone card widgets.
- Inventory all callers of both original text-bounds collectors (28 ordinary, 11 framed). Each immediate and queued sprite submission checks the original engine's active window state. Window finish functions retain ownership after the collector is cleared, including generated panel geometry.
- Preserve each queue record's ownership AND UI family through sorting and eventual drawing, rather than relying on the enclosing caller at flush time.
- Separate gameplay-dialog and card/window families. Infer the original canvas from each family's actual ordinary glyph geometry; panels, icons, text and scissor rectangles use one family-wide affine map. Actual captures showed both original 4:3 and already-compensated 16:9 windows, so forcing every window to 4:3 was rejected before delivery.
- Preserve all 40 scene/live contexts independently of the two UI metadata bits. Zero remains untagged. The compact context has an exhaustive round-trip fixture over all ownership/family combinations.
- Private capture version 2 distinguishes the metadata encoding; both readers retain version 1 compatibility.
- Reject captured native glyphs whose resulting physical aspect differs from their actual texel geometry, in addition to unchanged-vertex/UV and protected-effect checks.

## Verification limits

All game function bodies are inventoried structurally; indirect dispatch, state-dependent reachability, and final Metal output require runtime or device evidence. Route names are intended input sequences, not proof of reached screens. The delivered report must separately state observed owners/functions and device verification limits. In particular, do not equate a card browser or a shared completion-message function with an observed complete postrace Save Replay sequence.
