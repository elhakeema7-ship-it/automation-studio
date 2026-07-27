# Automation Studio - C Engine

**High-performance file scanner and database manager for the Automation Studio ecosystem.**

## Features

✨ **Core Features:**
- 📄 **Multi-Format Support** - Reads any file type (code, markdown, text, binary)
- 🗄️ **SQLite Database** - Efficient storage with full-text indexing
- 🔐 **SHA256 Hashing** - Content integrity verification
- 📊 **JSON Export** - Complete database export for analysis
- ⚡ **High Performance** - Optimized for speed and memory efficiency
- 🔄 **Recursive Scanning** - Deep directory traversal
- 🎯 **Metadata Tracking** - File type, size, hash, timestamps

## Installation

### Prerequisites

- GCC or Clang compiler
- SQLite3 development libraries
- OpenSSL development libraries (for SHA256)

### Ubuntu/Debian

```bash
sudo apt-get install build-essential libsqlite3-dev libssl-dev
```

### macOS

```bash
brew install sqlite3 openssl
```

### Build

```bash
cd c-engine
make clean
make
```

Binary will be created at `bin/automation-engine`

## Usage

### Basic Scanning

```bash
./bin/automation-engine -s /path/to/project -d project.db
```

### Recursive Scan with Export

```bash
./bin/automation-engine -s /path/to/project -d project.db -o export.json -e -r
```

### Export Existing Database

```bash
./bin/automation-engine -d project.db -o export.json -e
```

### Clear Database

```bash
./bin/automation-engine -d project.db -c
```

### Command-line Options

```
-h, --help              Show help message
-s, --scan <dir>        Scan directory
-d, --database <path>   Database file (default: files.db)
-o, --output <path>     JSON output (default: database_export.json)
-e, --export            Export to JSON
-c, --clear             Clear database
-r, --recursive         Recursive scan
```

## Output Format

### Database Structure

**files table:**
```sql
id                INTEGER   PRIMARY KEY
filepath          TEXT      NOT NULL UNIQUE
filename          TEXT      NOT NULL
extension         TEXT
file_size         INTEGER
content_hash      TEXT      UNIQUE (SHA256)
content           BLOB
language          TEXT
created_at        TIMESTAMP
updated_at        TIMESTAMP
```

**file_metadata table:**
```sql
id                INTEGER   PRIMARY KEY
file_id           INTEGER   FOREIGN KEY
key               TEXT
value             TEXT
```

### JSON Export Example

```json
{
  "metadata": {
    "total_files": 42,
    "export_timestamp": "2024-generated",
    "database_path": "project.db"
  },
  "files": [
    {
      "id": 1,
      "filepath": "/path/to/file.md",
      "filename": "file.md",
      "extension": "md",
      "file_size": 1024,
      "content_hash": "abc123..."
    },
    ...
  ]
}
```

## Performance

- **Scanning Speed**: ~1000 files/second (depends on hardware)
- **Memory Usage**: ~10MB base + ~1KB per file
- **Database Size**: ~20KB overhead + file content
- **Hash Calculation**: SHA256 streamed (memory efficient)

## Architecture

### Components

1. **file_scanner** - Directory traversal and file detection
2. **database** - SQLite management and queries
3. **json_exporter** - Database serialization to JSON
4. **main** - CLI interface and orchestration

### Flow

```
CLI Input
   ↓
Arg Parsing
   ↓
Database Init
   ↓
File Scanning (if -s)
   ↓
Content Reading & Hashing
   ↓
Database Storage
   ↓
JSON Export (if -e)
   ↓
Output Report
```

## Examples

### Scan a project and export

```bash
./bin/automation-engine -s ~/projects/my-app -d myapp.db -o myapp-export.json -e -r
```

### Scan multiple times (append mode)

```bash
# First run
./bin/automation-engine -s ~/projects -d project.db

# Second run (updates existing files)
./bin/automation-engine -s ~/projects -d project.db

# Export
./bin/automation-engine -d project.db -o report.json -e
```

### Clear and rescan

```bash
./bin/automation-engine -d project.db -c -s ~/projects -o report.json -e -r
```

## Troubleshooting

### "Cannot open database" error

Check directory permissions:
```bash
ls -la
chmod 755 .
```

### "Too many files" error

Increase file descriptor limit:
```bash
ulimit -n 4096
```

### Build errors

Ensure dependencies are installed:
```bash
pkg-config --cflags --libs sqlite3
pkg-config --cflags --libs openssl
```

## Future Enhancements

- [ ] Multi-threading for faster scanning
- [ ] Incremental updates detection
- [ ] Compression support
- [ ] Full-text search indexing
- [ ] Vector storage for embeddings
- [ ] Graphql API interface

## License

MIT

## Author

elhakeema7 - Automation Studio
