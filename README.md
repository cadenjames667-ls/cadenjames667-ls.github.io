# cadenjames667-ls.github.io

Source for my personal portfolio site, hosted on GitHub Pages.

## Structure

- `index.html` — portfolio homepage: intro, skills, resume link, and project cards
- `resume.pdf` — downloadable resume, linked from the homepage
- `pantry/` — **The Pantry**, a real-time kitchen inventory manager (Firebase Firestore + Auth)
- `furnder/` — **Furnder**, a swipe-based furniture discovery app (Firestore-backed)
- `mycarremote/` — **MyCarRemote**, a project write-up + source for an ESP32-based web remote control for a mecanum-wheel smart car (embedded C++, not a hosted web app)
- `iam-rbac-demo/` — **Meridian Trust**, a project write-up for a role-based access control system with audit logging (Flask/PostgreSQL/Docker) — full source lives in its own [iam-rbac-demo](https://github.com/cadenjames667-ls/iam-rbac-demo) repo, not this one
- `firestore.rules` — prototype Firestore security rules shared by both Firebase-backed apps' projects

## Deploying

1. Push to the `main` branch.
2. GitHub Pages serves the repo root directly — no build step.
3. `.nojekyll` disables Jekyll processing so folders like `pantry/` and `furnder/` are served as-is.

## The Pantry (`pantry/`)

A real-time pantry and recipe-queue manager backed by Firebase.

**What works:**
- Live pantry sync and recipe URL queue via Firestore
- Email/password sign-in via Firebase Auth, with each account getting its own pantry space
- Share-by-link access to view another user's pantry
- A **Demo Mode** (button on the sign-in screen) that loads sample data entirely client-side — no account or Firestore writes required, nothing persists

**Important — this app is an unfinished test project:**
- The Firestore rules (`firestore.rules`) allow public read/write (`allow read, write: if true`). The sign-in screen keeps pantries logically separate but is **not** real data protection.
- Don't enter real personal information. Use Demo Mode or throwaway test credentials.

To point Pantry at your own Firebase project: create a Firebase project with Firestore + Email/Password Auth enabled, then paste your Web Config JSON into the app's "Sync" settings panel after signing in.

## Furnder (`furnder/`)

A swipe-based furniture discovery app. Paste a product link from Wayfair, IKEA, Amazon, etc., and it pulls the image/title automatically (via the Microlink API) for a Tinder-style swipe deck. Liked items are stored in Firestore.

## MyCarRemote (`mycarremote/`)

A custom web-based remote control for an Acebott QD001 ESP32 mecanum-wheel smart car. Unlike Pantry and Furnder, this isn't a hosted web app — the actual program (`MyCarRemote.ino`) runs on the ESP32 itself, which hosts its own WiFi access point and web server so you can drive the car from a browser with no phone app, router, or internet connection needed.

`mycarremote/index.html` is a write-up page (features, hardware pin mapping, setup steps, and the full source with syntax highlighting) for browsing on this site — it doesn't run the firmware. The real deployment target is the ESP32 board, flashed via the Arduino IDE.

## Meridian Trust (`iam-rbac-demo/`)

A role-based access control system with compliance-grade audit logging, modeled on enterprise and banking identity patterns — separating authentication, authorization, and audit into three enforced layers. Backend is Python/Flask + PostgreSQL, containerized with Docker Compose.

Like MyCarRemote, this is a write-up page rather than a hosted app (a Flask + Postgres stack can't run on GitHub Pages). The real, runnable source, setup instructions, and commit history live in the separate [iam-rbac-demo](https://github.com/cadenjames667-ls/iam-rbac-demo) repo.

## How these were built

Everything in this repo was built working alongside Claude (Anthropic's AI coding assistant) as a pair-programming partner — see the "How I Build" section on the homepage for more.
