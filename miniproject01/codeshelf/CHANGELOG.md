# Changelog

## 2026-06-30 — Performance & UX

### Added
- `ScanWorker`: background folder scan on dedicated thread
- `loadSessionFromDb()`: restore last workspace from DB on startup
- `insertFileMetadata()`: store file metadata without reading full content
- Path normalization and second-level mtime comparison for incremental sync

### Changed
- App startup loads DB list only (no disk scan)
- Search list uses DB pagination (`LIMIT` / `OFFSET`) instead of loading 100k rows
- Folder scan runs asynchronously; UI stays responsive with progress in status bar

### Fixed
- Missing implementations for `initDatabase`, `onSearchRequested`, `filterByExt`, `onSearchExecuted`, `copyToClipboard`
- Existing files incorrectly treated as new files due to path format mismatch

### Known limitations
- Full-text content search is limited because scan no longer stores file content in DB
- Files deleted from disk may remain in DB until a cleanup step is added
- MySQL connection settings are still hard-coded in source
