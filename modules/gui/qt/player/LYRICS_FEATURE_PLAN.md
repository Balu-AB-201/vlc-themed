# Lyrics search and karaoke synchronization

## Scope

This fork's current desktop Qt player already contains a lyrics view in
`modules/gui/qt/player/qml/LyricsFlickable.qml`. It consumes
`Player.syltLyrics`, highlights the current line through
`Player.currentLyricIndex`, scrolls with playback, and lets the user seek by
tapping a line. The player view also checks `Player.hasLyrics` and
`MainCtx.lyricsMode`.

This is a useful presentation layer, but it does not by itself provide
automatic online lyrics lookup or prove that word-level timestamps are
available.

## Required behavior

- Automatic lyrics lookup is enabled by default when a track has no usable
  embedded/synchronized lyrics.
- Search candidates using normalized title and artist metadata. Let the user
  manually search and correct a match.
- Prefer cached or embedded lyrics before making network requests.
- Parse standard timestamped LRC for line synchronization.
- Support enhanced LRC / word-timestamped lyrics for word-by-word karaoke.
- If a source only supplies line timestamps, keep line highlighting rather
  than inventing word timings.
- Cache successful results and avoid repeated requests for the same track.
- Show a clear unavailable state and keep playback usable when lookup fails.
- Make the lyrics view keyboard/remote accessible as well as touch/mouse
  accessible; keep text readable at TV viewing distance.
- Do not bundle or redistribute copyrighted lyric text. Fetch only from a
  provider whose API and terms permit this use, and respect attribution,
  rate limits, and caching rules.

## Implementation sequence

1. Inspect the existing VLC player/metadata APIs and determine where lyrics
   are exposed to QML; do not duplicate the playback clock.
2. Add a small lyrics provider/parser layer with explicit result states
   (loading, found, not found, error), normalized track matching, and a cache.
3. Connect automatic lookup and a manual search/correction action to the
   existing player lyrics view.
4. Extend the presentation model to expose per-word timestamps only when
   present; highlight the active word from the existing playback position.
5. Add parser/model tests for malformed LRC, offsets, multiple lines at the
   same timestamp, enhanced LRC word timing, missing metadata, and provider
   failures.
6. Build and test the Qt player before merging. Android and Android TV require
   their own app source/integration; this Qt desktop tree alone does not update
   those apps.

## Acceptance criteria

- Existing embedded lyrics and line sync continue to work.
- A track without lyrics triggers lookup automatically by default.
- Manual search can correct an incorrect title/artist match.
- Word highlighting follows playback only when word timing exists.
- Missing network access or a provider error never interrupts playback.
- Existing player navigation and keyboard controls remain functional.
