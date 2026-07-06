# Security note (before public GitHub)

This project currently includes database credentials in:

- `codeshelf/DatabaseManager.h` (default `connectDB` parameters)
- `sql/database_create.sql`

Before making the repository **public**, consider:

1. Replace passwords with placeholders in committed files
2. Document local setup in README only (no real secrets)
3. Add `config.local.json` or environment variables and gitignore that file

Do not push real production passwords to a public repository.
