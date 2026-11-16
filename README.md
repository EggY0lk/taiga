# Taiga

[![](https://img.shields.io/github/license/erengy/taiga)](https://github.com/erengy/taiga/blob/master/LICENSE)
[![](https://img.shields.io/github/v/release/erengy/taiga)](https://taiga.moe/download.php)
[![](https://img.shields.io/discord/423475967051169813?logo=discord)](https://discord.gg/yeGNktZ)
[![](https://img.shields.io/github/sponsors/erengy?logo=github)](https://github.com/sponsors/erengy)

[Taiga](https://taiga.moe) is an open-source desktop application for Windows. It automatically detects the anime videos you watch on your computer and synchronizes your progress with [AniList](https://anilist.co), [Kitsu](https://kitsu.app) or [MyAnimeList](https://myanimelist.net). It helps you manage your anime library, discover new series, share watched episodes and download new ones.

## About this fork

This fork fixes Kitsu library synchronization (work done with Hermes Agent):

- Token POST/refresh now sends the User-Agent header — fixes Cloudflare returning 403 challenge so login succeeds and access/refresh tokens are stored.
- Pagination offset keys are decoded — Kitsu's page[offset] links are read correctly, so sync continues past page 1 instead of stopping.
- Page size raised to 100 with retry/backoff on Cloudflare 403/429 and timeouts, 150 ms pacing between pages, and a prune guard so an incomplete fetch cannot delete local library data.
- Network layer recognizes Cloudflare challenges for clearer errors.

Progress bar hover controls implemented with OpenCode.


## Links

- [Changelog](https://github.com/erengy/taiga/wiki/Changelog)
- [Contribution guidelines](https://github.com/erengy/taiga/wiki/Guidelines)
- [How to compile](https://github.com/erengy/taiga/wiki/How-to-Compile)

### Related projects

- [Anime relations](https://github.com/erengy/anime-relations) (episode redirections)
- [Anisthesia](https://github.com/erengy/anisthesia) (media detection library)
- [Anitomy](https://github.com/erengy/anitomy) (anime video filename parser)
- [taiga.moe](https://github.com/erengy/taiga-moe) (home page of Taiga)

## Progress bar controls

Hover over a progress bar cell in the anime list or cards view to reveal `+` / `-` icons. Clicking `+` increments watched episodes by 1; clicking `-` decrements by 1 (clamped to 0–episode count). The change is saved and reflected in the UI immediately. No icons are shown when not hovered.

## Development methodology

This fork applies **poteto-mode** from [Lauren Tan (poteto)](https://x.com/poteto) at [SpaceXSI](https://github.com/cursor/plugins/blob/main/pstack/skills/poteto-mode/SKILL.md), as presented in her talk "[All Code Is Slop (Except Mine)](https://youtu.be/O_zcc6Xl12g)".

Core principles from poteto-mode:

- **Deterministic guardrails over prose rules** — lint rules, type constraints, and directory structure replace markdown docs and comments as the source of truth for what agents (and humans) can do.
- **Verification-first** — every change ships with a verification artifact (tests, runtime proof) rather than relying on human line-by-line review.
- **Self-correction** — when a bug is found, fix the code *and* update the lint rule or test suite so that anti-pattern cannot be reintroduced.

See `AGENTS.md` for the C/C++ clang-tidy linting workflow derived from poteto-mode.

## License

Taiga is licensed under [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html).

## Credits

**Original app:** Taiga is developed by Eren Gökçe (erengy) and contributors. Source: https://github.com/erengy/taiga

**Fork fixes:** Kitsu library synchronization fixes and progress bar hover controls implemented directly with Hermes Agent (Nous Research), powered by **nvidia/nemotron-3.5-super-vl-120b-a12b** (provider: NVIDIA).

**Methodology:** The poteto-mode workflow and clang-tidy linting rules are based on Lauren Tan (poteto)'s "All Code Is Slop (Except Mine)" talk at SpaceXSI. Credit to Lauren Tan for poteto-mode, deterministic-guardrails-over-prose, and verification-first development approach. Official skill: https://github.com/cursor/plugins/blob/main/pstack/skills/poteto-mode/SKILL.md
