# Copilot / Agent Instructions

## DOPE submodule — read-only

The `DOPE/` directory is a git submodule (a separate repo owned by a collaborator).
You have full read access and should use it for context when answering questions or
designing features in DOPE-ASS.

**Do NOT edit, create, or delete any file inside `DOPE/`.**

If you determine that a change to `DOPE/` is required to complete a task:
1. Stop before making the edit.
2. Clearly describe the exact change needed (file, location, what to add/change/remove).
3. Tell the user to make it in a separate VS Code window pointed at the DOPE repo.
4. Continue with any DOPE-ASS side changes that don't depend on DOPE being updated yet.

The user will handle DOPE changes themselves and let you know when done.
