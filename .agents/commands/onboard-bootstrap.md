---
name: onboard-bootstrap
description: Bootstrap agentic coding setup. Installs the onboard plugin from the team marketplace if not already installed, then runs /onboard. Use on a fresh machine or repo clone.
---

<!-- synced from onboard-skill; do not edit — customize via .agents/onboarding-team-prefs.yaml and onboard-repo -->

# Onboard Bootstrap

Ensures the `onboard` plugin family is installed, then runs the full wizard.

**Scope:** Step 2 installs only the **`onboard`** plugin (the wizard runner) into each agent's
**global** home config — not the full team marketplace. That is expected: developers with
both Claude Code and Cursor may get `onboard` in both. The remaining marketplace plugins
(superpowers, openspec, etc.) are installed later by `/onboard` → `onboard-plugins`.

## Step 1 — Read config

```bash
cat .agents/onboarding-team-prefs.yaml 2>/dev/null
```

If `.agents/onboarding-team-prefs.yaml` does not exist AND a bundled template exists in the
installed onboard plugin, copy it to `.agents/onboarding-team-prefs.yaml`:

```bash
TEMPLATE=$(ls ~/.claude/plugins/cache/{{ marketplace_name }}/onboard/*/onboarding-team-prefs.yaml \
              ~/.claude/plugins/cache/*/onboard/*/onboarding-team-prefs.yaml 2>/dev/null | head -1)
[ -n "$TEMPLATE" ] && cp "$TEMPLATE" .agents/onboarding-team-prefs.yaml
```

Then tell the user: *"Edit `.agents/onboarding-team-prefs.yaml` to set `marketplace_url` and other team options."*

Parse `marketplace_url`, `marketplace_name`, `onboard_plugin_repo`, and `plugin_install_scope` from config.
Default `onboard_plugin_repo`: `https://git.autodesk.com/shotgun/onboard-skill.git`
Default `plugin_install_scope`: `user`

Also check if the `marketplace_url` differs from the URL the plugin was installed from.
If they differ, prompt: *"The config's `marketplace_url` points to `{{ marketplace_url }}`
but this plugin came from a different marketplace. Update `marketplace_url`? (y/n)"*

## Step 2 — Install onboard plugin if needed

Check what's already installed:

```bash
HAS_CLAUDE=$(claude plugin list 2>/dev/null | grep -q "onboard@{{ marketplace_name }}" && echo yes || echo no)
HAS_CURSOR=$([ -f "$HOME/.cursor/skills/onboard/SKILL.md" ] && echo yes || echo no)
```

Track agents that need a reload after a **new** install this run.

### Claude Code (when `command -v claude` succeeds)

If `HAS_CLAUDE=yes` → skip.

Otherwise:

```bash
claude plugin marketplace add {{ marketplace_url }}
claude plugin marketplace update {{ marketplace_name }}
claude plugin install onboard@{{ marketplace_name }} --scope {{ plugin_install_scope }}
```

- On success → mark Claude for reload
- On failure → warn and continue (Cursor fallback below may still work)

### Cursor (when `$HOME/.cursor` exists)

Always sync `onboard` from a **persistent git cache** (pull + copy on every bootstrap run).
Reload Cursor only on **first** install (when `onboard/SKILL.md` was missing before this run).

```bash
CURSOR_WAS_NEW=$([ -f "$HOME/.cursor/skills/onboard/SKILL.md" ] && echo no || echo yes)
SYNC="$HOME/.cursor/skills/onboard-plugins/scripts/sync-cursor-plugin.sh"
if [ -f "$SYNC" ]; then
  sh "$SYNC" {{ marketplace_name }} onboard git {{ onboard_plugin_repo }}
else
  CACHE="$HOME/.cursor/plugins/cache/{{ marketplace_name }}/onboard/repo"
  if [ -d "$CACHE/.git" ]; then
    git -C "$CACHE" fetch --depth 1 origin && git -C "$CACHE" reset --hard FETCH_HEAD
  else
    mkdir -p "$(dirname "$CACHE")"
    git clone --depth 1 {{ onboard_plugin_repo }} "$CACHE"
  fi
  mkdir -p "$HOME/.cursor/skills" "$HOME/.cursor/commands"
  cp -R "$CACHE/skills/." "$HOME/.cursor/skills/"
  cp -R "$CACHE/commands/." "$HOME/.cursor/commands/"
fi
```

- If `CURSOR_WAS_NEW=yes` → mark Cursor for reload
- If `CURSOR_WAS_NEW=no` → synced in place; reload optional unless skills changed materially

Also used as fallback when Claude install failed.

### Sync repo copy (when `.agents/commands/` exists)

After global sync above, keep this repo's entry-point command aligned with the plugin cache.
`onboard-skill` is canonical — do not hand-edit `.agents/commands/onboard-bootstrap.md`;
customize via `.agents/onboarding-team-prefs.yaml` and `onboard-repo` instead.

Prefer the Cursor cache (just refreshed in Step 2). Fall back to Claude plugin cache.

```bash
REPO_BOOTSTRAP=".agents/commands/onboard-bootstrap.md"
if [ -d ".agents/commands" ]; then
  CANONICAL=""
  CURSOR_CACHE="$HOME/.cursor/plugins/cache/{{ marketplace_name }}/onboard/repo/commands/onboard-bootstrap.md"
  if [ -f "$CURSOR_CACHE" ]; then
    CANONICAL="$CURSOR_CACHE"
  else
    CANONICAL=$(ls ~/.claude/plugins/cache/{{ marketplace_name }}/onboard/*/commands/onboard-bootstrap.md \
                  ~/.claude/plugins/cache/*/onboard/*/commands/onboard-bootstrap.md 2>/dev/null | head -1)
  fi
  if [ -n "$CANONICAL" ]; then
    if ! cmp -s "$CANONICAL" "$REPO_BOOTSTRAP" 2>/dev/null; then
      cp "$CANONICAL" "$REPO_BOOTSTRAP"
      echo "Synced $REPO_BOOTSTRAP from onboard plugin cache"
    fi
  fi
fi
```

- If the file changed → mention it briefly (optional: `git diff .agents/commands/onboard-bootstrap.md`)
- If no cache yet (first run, install failed) → skip silently; repo seed file is used as-is

### Proceed or stop

If any agent was **newly** installed this run → tell the user which agent(s) need a reload,
then **stop here**. Reload Claude Code (`/plugin reload`) and/or Cursor, then re-run
`/onboard-bootstrap` or `/onboard`.

If onboard is unavailable on every agent (both installs failed and neither was pre-installed)
→ stop with an error.

Otherwise → Step 3.

## Step 3 — Run the full wizard

Use the Skill tool to invoke `onboard`.
