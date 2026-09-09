# cadenjames667-ls.github.io

Source for my personal portfolio site, hosted on GitHub Pages.

## Structure

- `index.html` — portfolio homepage: intro, skills, resume link, project cards, the visit-logging snippet, and a hidden easter egg (see below)
- `resume.pdf` — downloadable resume, linked from the homepage
- `assets/bad-apple.mp4` — local video for the homepage easter egg (see below)
- `admin/` — real-auth-gated dashboard for viewing the site's basic visit log
  - `admin/firestore.rules` — security rules for the visit-log Firebase project (public create, owner-only read)
- `pantry/` — **The Pantry**, a real-time kitchen inventory manager (Firebase Firestore + Auth)
  - `pantry/about/` — write-up page (how it's made, architecture, data model) — this is what the homepage card links to
  - `pantry/firestore.rules` — security rules for Pantry's Firebase project (currently wide open — see below)
- `furnder/` — **Furnder**, a swipe-based furniture discovery app (Firestore-backed)
  - `furnder/about/` — write-up page (how it's made, architecture, data model) — this is what the homepage card links to
- `mycarremote/` — **MyCarRemote**, a project write-up + source for an ESP32-based web remote control for a mecanum-wheel smart car (embedded C++, not a hosted web app)
- `iam-rbac-demo/` — **Meridian Trust**, a project write-up for a role-based access control system with audit logging (Flask/PostgreSQL/Docker) — full source lives in its own [iam-rbac-demo](https://github.com/cadenjames667-ls/iam-rbac-demo) repo, not this one
- `homelab/` — **Homelab**, a write-up for a self-hosted Linux server (no code to show — it's ops/infrastructure work, not an app)
- `this-site/` — **This Site**, a write-up about how the portfolio itself is built (site map, design-system tradeoffs)

## Deploying

1. Push to the `main` branch.
2. GitHub Pages serves the repo root directly — no build step.
3. `.nojekyll` disables Jekyll processing so folders like `pantry/` and `furnder/` are served as-is.

## The Pantry (`pantry/`)

A real-time pantry and recipe-queue manager backed by Firebase. The homepage links to `pantry/about/` first — a write-up page with a "Try It" button to the actual app at `pantry/` — rather than linking straight to the live app.

**What works:**
- Live pantry sync and recipe URL queue via Firestore
- Email/password sign-in via Firebase Auth, with each account getting its own pantry space
- Share-by-link access to view another user's pantry
- A **Demo Mode** (button on the sign-in screen) that loads sample data entirely client-side — no account or Firestore writes required, nothing persists

**Important — this app is an unfinished test project:**
- The Firestore rules (`pantry/firestore.rules`) allow public read/write (`allow read, write: if true`). The sign-in screen keeps pantries logically separate but is **not** real data protection.
- Don't enter real personal information. Use Demo Mode or throwaway test credentials.

To point Pantry at your own Firebase project: create a Firebase project with Firestore + Email/Password Auth enabled, then paste your Web Config JSON into the app's "Sync" settings panel after signing in.

## Furnder (`furnder/`)

A swipe-based furniture discovery app. Paste a product link from Wayfair, IKEA, Amazon, etc., and it pulls the image/title automatically (via the Microlink API) for a Tinder-style swipe deck. Liked items are stored in Firestore. Like Pantry, the homepage links to `furnder/about/` (write-up + "Try It" button) rather than straight to the live app at `furnder/`.

## MyCarRemote (`mycarremote/`)

A custom web-based remote control for an Acebott QD001 ESP32 mecanum-wheel smart car. Unlike Pantry and Furnder, this isn't a hosted web app — the actual program (`MyCarRemote.ino`) runs on the ESP32 itself, which hosts its own WiFi access point and web server so you can drive the car from a browser with no phone app, router, or internet connection needed.

`mycarremote/index.html` is a write-up page (features, hardware pin mapping, setup steps, and the full source with syntax highlighting) for browsing on this site — it doesn't run the firmware. The real deployment target is the ESP32 board, flashed via the Arduino IDE.

## Meridian Trust (`iam-rbac-demo/`)

A role-based access control system with compliance-grade audit logging, modeled on enterprise and banking identity patterns — separating authentication, authorization, and audit into three enforced layers. Backend is Python/Flask + PostgreSQL, containerized with Docker Compose.

Like MyCarRemote, this is a write-up page rather than a hosted app (a Flask + Postgres stack can't run on GitHub Pages). The real, runnable source, setup instructions, and commit history live in the separate [iam-rbac-demo](https://github.com/cadenjames667-ls/iam-rbac-demo) repo.

## Homelab (`homelab/`)

A self-hosted Linux server, provisioned and administered entirely over SSH, hosting several private Minecraft servers. Remote access runs over a private Tailscale mesh rather than public port-forwarding, with UFW/iptables enforcing a default-deny stance on top of that.

Unlike the other project pages, there's no source to show here — it's systems/ops work, not a codebase. `homelab/index.html` covers the architecture and practices instead.

## Admin (`admin/`)

A basic site visit log: the homepage logs a visit (timestamp, page path, referrer, browser/language — no IP or personal data) to a dedicated Firebase project on every load. `admin/index.html` is a dashboard for viewing that log, gated by real Firebase Auth sign-in rather than a client-side password check — `admin/firestore.rules` restricts reads to one specific account UID, enforced server-side, so signing in as anyone else (were that even possible, since there's no public sign-up UI) still couldn't read the log.

## Easter Egg

The nav bar's "Caden James" has one hidden interactive letter — the "a" in "Caden" (`#trigger-a` in `index.html`). Clicking it morphs the name into "Bad Apple" with a slide transition and swaps the page background for a fullscreen, looping local video of *Bad Apple!!* (`assets/bad-apple.mp4`) with audio, playing behind all UI elements — which stay fully visible and interactive on top. Clicking it again reverses both. Using a local video file (rather than a YouTube embed) means it also works offline.

## This Site (`this-site/`)

A write-up about the portfolio itself: a site map of how the homepage, project write-ups, live apps, and the three separate Firebase projects connect, plus the deliberate tradeoffs behind it — no build step, no shared component library (styles are repeated per-page on purpose), and each sub-app kept on its own isolated Firebase project.

## How these were built

Everything in this repo was built working alongside Claude (Anthropic's AI coding assistant) as a pair-programming partner — see the "How I Build" section on the homepage for more.
