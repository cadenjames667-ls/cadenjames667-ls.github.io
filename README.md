# Pantry On GitHub Pages

This version is built to work as a static site on GitHub Pages.

## What works now

- Live pantry sync with Firebase Firestore
- Live recipe URL queue with Firebase Firestore
- Email/password sign-in via Firebase Auth
- Automatic import of the app's old localStorage pantry and recipe data into Firestore the first time you connect
- Share-by-link access to view another user's pantry

## Account & access flow

The app is gated behind Firebase Auth email/password sign-in.

- `Sign In` authenticates an existing account
- `Create Account` registers a new Firebase Auth user
- Each signed-in user gets their own pantry space, keyed by their Firebase UID
- `Share Pantry` copies a link that lets another signed-in user view your pantry space

## Firestore setup

The app ships with a default Firebase project already wired up. To point it at your own Firebase project instead:

1. Create a Firebase project and enable Firestore Database + Authentication (Email/Password provider).
2. Create a Web App inside Firebase and copy the Firebase Web Config JSON.
3. Open the deployed app, sign in, then use the "Sync" settings panel to paste in your config and reconnect.

## GitHub Pages deploy

1. Commit `index.html` and this README to your repo.
2. Push to GitHub.
3. In GitHub repo settings, enable GitHub Pages for your branch.

## Firestore rules

The included `firestore.rules` file is a prototype rule set for a public demo or personal project without auth.

Important:

- These rules allow public read and write access.
- That is okay for a quick prototype, but not for a production app with sensitive data.
- If you want, the next step can be tightening these rules to require a matching Firebase Auth UID.
