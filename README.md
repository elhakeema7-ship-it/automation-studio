# Automation Studio

**Automation Studio** is a VS Code Extension that helps you manage and execute automation workflows from Markdown files (Skills, Workflows, Tasks, etc.).

## Features

✨ **Key Features:**
- 📄 **Auto-detect Markdown Files** - Scans your workspace for `.md` files
- ⚡ **Parse Commands** - Extracts commands from Markdown files automatically
- 🎯 **Interactive UI** - Split panel view with file browser and command executor
- 🔧 **Execute Commands** - Run scripts/commands directly from the UI
- 🎨 **VS Code Theme Integration** - Matches your VS Code theme

## Installation

1. Clone the repository:
```bash
git clone https://github.com/elhakeema7-ship-it/automation-studio.git
cd automation-studio
```

2. Install dependencies:
```bash
npm install
```

3. Run the extension in development mode:
```bash
npm run esbuild-watch
```

4. Open VS Code and press `F5` to launch the extension

## Usage

1. **Open Automation Studio** - Look for the ⚙️ icon in the Activity Bar on the left
2. **Select a Markdown File** - Click on any `.md` file in the left panel
3. **View Commands** - Commands are automatically extracted from the file
4. **Execute Commands** - Click the "Execute" button to run a command

## Supported Markdown Formats

The extension recognizes commands in these formats:

### Format 1: Using ### headings
```markdown
## Commands

### Create File
Create a new file with Hello World

### Format Code
Automatically format the code
```

### Format 2: Using bullet points with **bold** names
```markdown
## Tasks

- **Create File**: Create a new file with Hello World
- **Format Code**: Automatically format the code
```

## File Structure

```
automation-studio/
├── package.json           # Extension configuration
├── tsconfig.json          # TypeScript configuration
├── src/
│   └── extension.ts       # Main extension code
├── resources/
│   └── automation-icon.svg # Extension icon
└── README.md              # This file
```

## Development

### Build
```bash
npm run compile
```

### Watch (development)
```bash
npm run esbuild-watch
```

### Package for production
```bash
npm run vscode:prepublish
```

## Requirements

- VS Code 1.85.0 or later
- Node.js 18.0 or later (for development)

## License

MIT

## Author

elhakeema7

---

Made with ❤️ for automation lovers
