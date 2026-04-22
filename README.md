# Pantry On GitHub Pages

This version is built to work as a static site on GitHub Pages.

## What works now

- Live pantry sync with Firebase Firestore
- Live recipe URL queue with Firebase Firestore
- H-E-B receipt import from receipt images or text files
- Browser-side Firebase config storage for quick setup
- Automatic import of the app's old localStorage pantry and recipe data into Firestore the first time you connect
- Lightweight pantry password screen that maps a password to a shared pantry space

## What is intentionally not live in this static version

- Recipe analysis against AI APIs
- Recipe suggestion generation against AI APIs

Those two features need a secure backend or serverless function because a GitHub Pages site should not expose private API keys in browser code.

## Firestore setup

1. Create a Firebase project.
2. Enable Firestore Database.
3. Create a Web App inside Firebase.
4. Copy the Firebase Web Config JSON from the Firebase console.
5. Open the deployed app, paste that JSON into the setup box, choose a pantry space name, and click `Save and Connect`.

## Pantry access flow

The current site includes a lightweight access gate before the app loads.

- `Open Pantry` only opens passwords that were already created
- `Create Pantry` registers a new password-to-pantry mapping in Firestore
- The lightweight built-in aliases live in `index.html` in `PANTRY_PASSWORD_MAP`
- Firestore-backed password mappings are stored in the `pantryAccess` collection

Current built-in examples:

- `demo` -> `demo-pantry`
- `family` -> `family-kitchen`
- `caden` -> `caden-pantry`

Important:

- This is not real authentication
- It is only a lightweight access pattern for shared static-site use
- Anyone with the password can access that pantry

## GitHub Pages deploy

1. Commit `index.html` and this README to your repo.
2. Push to GitHub.
3. In GitHub repo settings, enable GitHub Pages for your branch.
4. Open the published site and connect it to Firestore using your Firebase web config.

## Firestore rules

The included `firestore.rules` file is a prototype rule set for a public demo or personal project without auth.

Important:

- These rules allow public read and write access.
- That is okay for a quick prototype, but not for a production app with sensitive data.
- If you want, the next step can be adding Firebase Auth and tightening the rules.
