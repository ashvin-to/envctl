# envctl

Local environment variable manager with profiles, inheritance, secrets, and a TUI.

## Install

```bash
# Dependencies
sudo apt install libsqlite3-dev libncursesw5-dev libssl-dev cmake g++

# Build
cd envctl
mkdir build && cd build
cmake .. && make -j$(nproc)

# Install (optional)
sudo make install
```

## Quick Start

```bash
cd /path/to/my-project
envctl init --name my-project          # Creates profiles: development, production, staging
envctl set DATABASE_URL=postgres://localhost/dev
envctl set API_KEY=sk-12345 --secret
envctl use production
eval $(envctl shell)                    # Loads active profile into your shell
```

## Commands

| Command | Description |
|---------|-------------|
| `envctl init [--name <name>]` | Initialize envctl for current directory |
| `envctl list [--json] [--resolved]` | List variables in active profile |
| `envctl set KEY=VALUE [--secret] [--description <text>]` | Set a variable |
| `envctl unset KEY` | Remove a variable |
| `envctl use <profile>` | Switch active profile |
| `envctl profiles` | List all profiles |
| `envctl create <name> [--parent <parent>]` | Create a new profile |
| `envctl diff <profile1> <profile2>` | Compare two profiles |
| `envctl shell [profile]` | Print `export` statements for shell eval |
| `envctl import <file> [--format <fmt>] [--overwrite]` | Import from .env, Docker, JSON |
| `envctl export [--format <fmt>] [--output <file>] [--resolved]` | Export to .env, JSON, shell, Docker |
| `envctl history [--key <key>] [--limit <n>]` | View change history |
| `envctl validate [--template .env.example]` | Validate variables (types, required) |
| `envctl mask [--json]` | List variables with secrets hidden (flagged secret **or** key names matching secret/token/password/key/cred patterns, plus any custom patterns) |
| `envctl patterns add <PATTERN>` | Add a custom secret-masking pattern for this project (substring, case-insensitive) |
| `envctl patterns remove <PATTERN>` | Remove a custom secret-masking pattern |
| `envctl patterns list` | List custom secret-masking patterns for this project |
| `envctl watch <file.env> [--no-reload]` | Watch .env file and auto-sync changes |
| `envctl sync [--push] [--pull]` | Sync profiles to git |
| `envctl tui` | Launch interactive terminal UI |

## Profiles

Profiles are named sets of variables. Each profile can optionally inherit from a parent, so child profiles only need to override what changes:

```
development          (base vars)
  staging            (inherits development, overrides some)
production           (independent)
  ci                 (inherits production)
```

```bash
envctl create staging --parent development
envctl create ci --parent production
```

`envctl list --resolved` walks the parent chain and merges variables, with child values taking precedence.

## Import / Export

```bash
envctl import .env                              # .env file
envctl import .env.production --overwrite       # Overwrite existing
envctl import docker-compose.yml                # Docker compose
envctl import config.json                       # JSON

envctl export                                   # .env format to stdout
envctl export --format json -o config.json      # JSON to file
envctl export --format shell -r                 # Resolved shell exports
envctl export --format docker -o env.yml        # Docker compose fragment
```

## Validation

Define a template `.env.example` with type hints in comments:

```env
DATABASE_URL=              # @type url
PORT=8080                  # @type port
DEBUG=true                 # @type bool
ADMIN_EMAIL=               # @type email
MAX_CONNECTIONS=100        # @type integer
LOG_PATH=/var/log/app      # @type path
```

```bash
envctl validate --template .env.example
```

Supported types: `url`, `email`, `int`/`integer`, `bool`/`boolean`, `port`, `path`.

Variables with a non-empty default in the template are **required**.

## Secrets

```bash
envctl set API_KEY=sk-abc123 --secret    # Mark as secret
envctl list                               # Shows **** for secrets
envctl mask                               # Explicit mask view
```

Secrets are never exported to git (`envctl sync` writes `# SECRET: KEY=****`). `envctl mask` also auto-hides any variable whose key looks secret (contains secret, token, password, key, or cred) even if it was not explicitly flagged.

### Custom secret patterns

The built-in patterns cover common cases, but you can add your own per-project patterns. A pattern is matched as a case-insensitive substring against the variable key:

```bash
envctl patterns add oauth           # Hide any key containing "oauth"
envctl patterns add _key            # Hide any key ending in "_key"
envctl patterns list                # Show custom patterns for this project
envctl patterns remove oauth        # Stop hiding "oauth" keys
```

Custom patterns are stored per project (in the `secret_patterns` table) and are **additive** to the built-ins — you cannot remove the built-in patterns. Once added, `envctl mask` and `envctl mask --json` hide matching variables automatically.

### JSON output

`envctl mask --json` emits an array of `{ "key", "value", "secret" }`. Masked secrets keep `"value": "****"` — values are never revealed, even in JSON:

```bash
envctl mask --json
# [
#   {"key": "API_KEY", "value": "****", "secret": true},
#   {"key": "PORT", "value": "8080", "secret": false}
# ]
```

## Shell Integration

```bash
eval $(envctl shell)                  # Load active profile
eval $(envctl shell production)       # Load specific profile
```

## Watch

```bash
envctl watch .env                     # Watches for changes via inotify, auto-imports
envctl watch .env --no-reload         # Report changes only, don't write to the db
```

Prints `+`, `~`, `-` for added, changed, removed variables. With `--no-reload`, changes are reported but not applied to the database.

## Sync

```bash
envctl sync                           # Export profiles to .envctl/profiles/
envctl sync --push                    # Export + git commit + push
envctl sync --pull                    # git pull + export
```

Profiles are exported as `.envctl/profiles/<name>.env` files (git-friendly).

## TUI

```bash
envctl tui
```

| Key | Action |
|-----|--------|
| Tab | Cycle panels (Profiles / Variables / History) |
| Up/Down | Navigate |
| Enter | Switch to selected profile |
| p/v/h | Jump to Profiles/Variables/History |
| ? | Toggle help |
| q | Quit |

## Diff

```bash
envctl diff development production
```

```
Diff: development vs production

  - DEBUG = true                     (only in development)
  + HTTPS = true                     (only in production)
  ~ DATABASE_URL: dev://... -> prod://...
```

## History

```bash
envctl history
envctl history --key DATABASE_URL
envctl history --limit 10
```

Every `set` and `unset` records old/new values with timestamp and username.

## Database

SQLite at `~/.config/envctl/envctl.db` (WAL mode, foreign keys enabled).

Schema: `projects`, `profiles`, `variables`, `history`, `templates`, `active_profile`, `secret_patterns`.

## Project Structure

```
envctl/
├── CMakeLists.txt
└── src/
    ├── main.cpp
    ├── cli/cli.{h,cpp}              # CLI dispatcher
    ├── core/
    │   ├── database.{h,cpp}         # SQLite CRUD + inheritance resolver
    │   └── context.{h,cpp}          # Project/profile resolution
    ├── schema/schema.h              # SQL DDL
    ├── file/
    │   ├── envfile.{h,cpp}          # .env parser
    │   └── importer.{h,cpp}         # Import from .env, Docker, JSON
    ├── crypto/encrypt.{h,cpp}       # AES-256-CBC encryption
    ├── commands/                     # command implementations (incl. mask, patterns)
    └── tui/tui.{h,cpp}              # ncurses terminal UI
```

## License

[MIT](LICENSE)
