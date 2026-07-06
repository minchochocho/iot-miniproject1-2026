# Architecture

## Overview

CodeShelf is a desktop app that indexes local source files into MySQL and provides search and preview through a Qt Widgets UI.

## Layers

| Layer | Responsibility |
|-------|----------------|
| `CodeShelf` | UI, search state, pagination, session restore |
| `ScanWorker` | Background disk scan, metadata upsert |
| `DatabaseManager` | MySQL access, queries, path normalization |

## Runtime flows

### Startup (fast path)

1. Connect to MySQL
2. Restore last folder path from `QSettings`
3. Load file list from DB (first page only)
4. Render tags from extension aggregates

No disk scan runs on startup.

### Folder sync (background)

1. User selects a root folder
2. Main thread loads `filepath → last_modified` map from DB
3. `ScanWorker` walks the directory tree on a worker thread
4. Unchanged files are skipped via normalized path + epoch comparison
5. Changed/new files upsert metadata only (`content` left empty)
6. Main thread refreshes tags and search list on completion

### Preview

1. User clicks a list item or tree entry
2. `showDetail()` reads the file directly from disk
3. `CodeHighlighter` applies language-specific syntax coloring

## Database tables (in use)

- `storage_roots` — registered root folders
- `codes` — file metadata and optional content

Tables `tags` / `code_tags` exist in SQL scripts but are not used by the current app code.
