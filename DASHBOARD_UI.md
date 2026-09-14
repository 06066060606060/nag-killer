# Dashboard UI — v3.5a1-SC

This revision applies the approved Single-CAN dashboard design to the existing Working-v3.1 + Mode-H firmware runtime.

## Theme

- Light mode remains a native bright theme.
- Dark mode follows `prefers-color-scheme: dark` and uses the approved neutral-charcoal palette.
- The browser `theme-color` follows the active OS/browser theme.

## Mode selector

- Mode A/B/C tiles are enlarged and visually balanced against Mode H.
- All modes receive an active selection state.
- Mode H additionally displays a `SELECTED` badge when active and hides the `RECOMMENDED` badge while selected.

No mock API code is included in the firmware dashboard. All controls use the real `/api/*` endpoints.
