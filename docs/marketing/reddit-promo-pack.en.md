# Reddit Promo Pack

## Purpose
- Ready-to-post English copy for Reddit.
- Optimized for technical/product communities where authenticity matters more than polished ad language.

## Positioning Summary
- `MFCMouseEffect` is not just a mouse click effect toy.
- It is a cross-platform desktop input-feedback engine with:
  - cursor effects
  - input indicator
  - automation mapping
  - WASM plugin runtime
  - shared Web settings
- Current public positioning should stay honest:
  - macOS is the main active development line
  - Windows remains an important compatibility target
  - Linux is follow-up at compile/contract level

## Target Subreddit Angles
- `r/programming`
  - Angle: architecture + cross-platform desktop engine + WASM plugin boundary
- `r/cpp`
  - Angle: C++ desktop host with bounded WASM extensibility
- `r/macapps`
  - Angle: macOS input visualization / cursor effects / input indicator
- `r/opensource`
  - Angle: open-source cross-platform desktop tooling with plugin runtime
- `r/SideProject`
  - Angle: solo-built niche desktop tool that grew into an engine

## Title Bank
- I turned a mouse effects tool into a cross-platform input-feedback engine with a WASM plugin runtime
- Built an open-source desktop input visualization engine in C++ with cursor effects, automation, and plugins
- I have been building a cross-platform cursor effects and input indicator engine, and I would love feedback
- An open-source desktop input-feedback engine: click/trail/scroll/hold/hover plus WASM plugins
- I am experimenting with a bounded WASM plugin model for desktop input effects in a C++ host
- Show HN-style title for Reddit: cross-platform desktop input effects engine with shared settings and plugin lanes

## Short Post
Use this when the subreddit prefers brief, direct posts.

```text
I have been building an open-source project called MFCMouseEffect.

It started as a mouse effects idea, but it has gradually become a cross-platform desktop input-feedback engine with:
- click / trail / scroll / hold / hover effects
- cursor decoration
- input indicator for mouse + keyboard
- automation mapping for mouse actions / wheel / gestures
- a bounded WASM plugin runtime

The part I care about most is the architecture boundary: plugins can compute behavior, but the host still owns rendering, resources, fallback, and diagnostics.

macOS is my current mainline, Windows is kept regression-safe, and Linux is still more of a compile/contract target.

Repo:
https://github.com/sqmw/MFCMouseEffect

If you work on desktop UX, screen recording/tutorial tools, or C++ plugin systems, I would really like feedback on whether this direction feels useful or too niche.
```

## Medium Post
Use this when a subreddit allows a bit more engineering detail.

```text
I have been working on an open-source project called MFCMouseEffect:
https://github.com/sqmw/MFCMouseEffect

The original idea was simple mouse click effects, but over time I found the more interesting problem was broader desktop input feedback:

- cursor effects for click / trail / scroll / hold / hover
- a separate cursor-decoration lane
- an input indicator for mouse, wheel, and keyboard combos
- automation mapping for mouse actions and gestures
- a WASM plugin runtime for effect and indicator surfaces

One design choice I have been trying to keep strict is this:
the plugin does not own raw rendering or windowing.
The host owns rendering execution, fallback, diagnostics, and runtime policy.

That boundary has made the project slower to build, but much easier to evolve without turning it into an unstable plugin sandbox.

Current platform reality:
- macOS is the active mainline
- Windows is still important and should not regress
- Linux is currently compile-level / contract-level only

I am posting this because I want signal, not just promotion.
If you build desktop tools, teach with screen recordings, or care about C++/WASM host boundaries, I would love feedback on:
- which use case sounds most compelling
- whether the WASM boundary feels too restrictive or appropriately safe
- which part is most worth turning into polished demos first
```

## Demo-First Post
Use this together with a GIF or short screen recording.

```text
I have been building a desktop input-feedback engine and finally have enough pieces working together that it feels worth showing.

The project is called MFCMouseEffect:
https://github.com/sqmw/MFCMouseEffect

What is in the demo:
- cursor effects
- input indicator
- shared desktop settings UI
- plugin-ready effect lanes

The long-term goal is not just "prettier clicks".
I want this to be a reusable cross-platform base for:
- tutorials and screen recordings
- desktop UX experiments
- automation-triggered visual feedback
- bounded WASM extensions inside a native host

I would especially love feedback on what looks most promising vs what still feels unclear from the outside.
```

## Comment Templates
Use one as your first self-comment under the post.

### Comment: project link + quick framing
```text
Repo: https://github.com/sqmw/MFCMouseEffect

If you only want the quick summary:
- native desktop host in C++
- macOS mainline right now
- Windows kept regression-safe
- effects + indicator + automation + WASM plugin runtime

Happy to answer technical questions if anyone is curious about the architecture.
```

### Comment: ask for focused feedback
```text
The feedback I want most right now is actually pretty specific:

1. Does the project read more like a tool or like an engine?
2. Which part sounds most valuable from the outside: effects, indicator, automation, or plugin runtime?
3. If I polish one demo first, what should it be?
```

### Comment: for people asking "why WASM?"
```text
The short version is: I wanted extensibility without handing over rendering/window/resource ownership to plugins.

So the plugin computes commands and behavior, while the host keeps execution control, diagnostics, budget, and fallback.

It is more constrained than a fully open plugin model, but much easier to keep stable across platforms.
```

## Reply Templates
### Reply: "This seems niche"
```text
It is definitely niche compared with general productivity tools.

The bet is that tutorials, screen recording, desktop UX tooling, and input visualization all overlap enough that a reusable engine is more valuable than one-off effects.
```

### Reply: "Why not Electron/Tauri only?"
```text
The settings UI is web-based, but the core problem here is still native: input capture, overlays, platform behavior, and fallback paths.

So I kept the control plane web-friendly and the runtime host native.
```

### Reply: "Why C++?"
```text
Mostly because the core of the project lives in the native desktop/runtime layer rather than only the UI layer.

I also wanted the host/plugin boundary, rendering control, and cross-platform runtime seams to stay explicit.
```

### Reply: "What should I look at first?"
```text
If you care about product value, look at the effects + input indicator demos first.

If you care about architecture, the most interesting part is probably the bounded WASM plugin route and the shared settings/runtime diagnostics path.
```

## CTA Lines
- I am not mainly looking for stars here, I am looking for signal on which direction is actually worth polishing.
- If one part seems especially useful, I would love to know which one.
- If this feels over-engineered, I would honestly like to hear that too.
- If you know similar projects, I would appreciate links. I want the comparison.

## What To Avoid
- Do not say "the best" or "revolutionary".
- Do not claim Linux is a polished end-user platform yet.
- Do not imply full Windows/macOS feature parity if the current docs still describe staged parity.
- Do not paste a huge feature list without one clear story.

## Suggested Asset Pairing
- Best first post asset:
  - 15-30 second GIF showing click + trail + indicator in one flow
- Best second post asset:
  - screenshot of Web settings + one GIF of a plugin-driven effect
- Best engineering post asset:
  - one architecture diagram showing host-owned rendering and plugin-owned logic
