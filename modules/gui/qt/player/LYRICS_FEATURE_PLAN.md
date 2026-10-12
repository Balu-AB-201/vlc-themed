# Lyrics search and karaoke synchronization

## Scope

The VLC Qt player already has a lyrics view in `modules/gui/qt/player/qml/LyricsFlickable.qml`. It consumes `Player.syltLyrics`, highlights the current line through `Player.currentLyricIndex`, scrolls with playback, and lets the user seek by tapping a line. The player view also checks `Player.hasLyrics` and `MainCtx.lyricsMode`.

This presentation layer does not itself provide automatic online lyrics lookup or guarantee word-level timestamps.

## Required behavior

- Automatic lyrics lookup is enabled by default when a track has no usable embedded/synchronized lyrics.
- Search candidates using normalized title and artist metadata; allow manual search and correction.
- Prefer embedded or cached lyrics before making network requests.
- Parse standard timestamped LRC for line synchronization.
- Support enhanced LRC / word-timestamped lyrics for word-by-word karaoke.
- If a source only supplies line timestamps, keep line highlighting rather than inventing word timings.
- Cache successful results and avoid repeated requests for the same track.
- Show clear unavailable/error states and keep playback usable when lookup fails.
- Keep the lyrics view keyboard/remote accessible and readable at TV viewing distance.
- Use only lyric providers whose API and terms permit the intended use; respect attribution, rate limits, and caching rules.

## Implementation sequence

1. Inspect the existing VLC player/metadata APIs and determine where lyrics are exposed to QML; do not duplicate the playback clock.
2. Add a small lyrics provider/parser layer with explicit result states (loading, found, not found, error), normalized track matching, and a cache.
3. Connect automatic lookup and manual search/correction to the existing player lyrics view.
4. Expose per-word timestamps only when present and highlight the active word from the existing playback position.
5. Add tests for malformed LRC, offsets, multiple lines at the same timestamp, enhanced LRC word timing, missing metadata, and provider failures.
6. Build and test the Qt player before considering the implementation complete. Android and Android TV require their own app source/integration; this Qt desktop tree alone does not update those apps.

## Acceptance criteria

- Existing embedded lyrics and line sync continue to work.
- A track without lyrics triggers lookup automatically by default.
- Manual search can correct an incorrect title/artist match.
- Word highlighting follows playback only when word timing exists.
- Missing network access or provider errors never interrupt playback.
- Existing player navigation and keyboard controls remain functional.
