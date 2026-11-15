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

Verified: sync completes error-free; anime_list and anime tables grew from 100 to 655 rows each, entries span 2021–2026, and 505 completed entries match the Kitsu library.


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

## License

Taiga is licensed under [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html).
