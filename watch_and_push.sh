#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# watch_and_push.sh
# Watches assets/Paper2Slides.pdf for changes and auto-commits + pushes.
#
# Requirements (macOS):
#   brew install fswatch
#
# Usage:
#   chmod +x watch_and_push.sh
#   ./watch_and_push.sh
#   # Or run in background:  nohup ./watch_and_push.sh &
# ---------------------------------------------------------------------------

set -euo pipefail

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
WATCHED_FILE="assets/Paper2Slides.pdf"
BRANCH="gh-pages"

cd "$REPO_DIR"

# Verify fswatch is installed
if ! command -v fswatch &>/dev/null; then
  echo "ERROR: fswatch is not installed. Install it with: brew install fswatch"
  exit 1
fi

# Verify the file exists
if [[ ! -f "$WATCHED_FILE" ]]; then
  echo "WARNING: $WATCHED_FILE does not exist yet. Watching for it to be created..."
fi

echo "Watching $WATCHED_FILE for changes (branch: $BRANCH)..."
echo "Press Ctrl+C to stop."

fswatch -0 --event Created --event Updated --event Renamed \
  "$REPO_DIR/$WATCHED_FILE" | \
while IFS= read -r -d $'\0' changed_file; do
  echo ""
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Change detected: $changed_file"

  # Make sure we are on the right branch
  current_branch=$(git -C "$REPO_DIR" rev-parse --abbrev-ref HEAD)
  if [[ "$current_branch" != "$BRANCH" ]]; then
    echo "  ERROR: Not on branch '$BRANCH' (currently on '$current_branch'). Skipping push."
    continue
  fi

  # Stage the file
  git -C "$REPO_DIR" add "$WATCHED_FILE"

  # Only commit if there are staged changes (avoids empty commits)
  if git -C "$REPO_DIR" diff --cached --quiet; then
    echo "  No changes staged (file may not have actually changed). Skipping."
    continue
  fi

  COMMIT_MSG="chore: update Paper2Slides.pdf [$(date '+%Y-%m-%d %H:%M:%S')]"
  git -C "$REPO_DIR" commit -m "$COMMIT_MSG"
  echo "  Committed: $COMMIT_MSG"

  git -C "$REPO_DIR" push origin "$BRANCH"
  echo "  Pushed to origin/$BRANCH"
done
