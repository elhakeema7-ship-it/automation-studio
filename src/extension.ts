import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';

export function activate(context: vscode.ExtensionContext) {
	let disposable = vscode.commands.registerCommand(
		'automationStudio.openPanel',
		() => {
			const panel = vscode.window.createWebviewPanel(
				'automationStudio',
				'Automation Studio',
				vscode.ViewColumn.Beside,
				{ enableScripts: true }
			);

			panel.webview.html = getWebviewContent();
			
			panel.webview.onDidReceiveMessage(async (message) => {
				if (message.command === 'loadMarkdownFiles') {
					const files = await findMarkdownFiles();
					panel.webview.postMessage({ command: 'filesLoaded', files });
				} else if (message.command === 'parseMarkdown') {
					const commands = await parseMarkdownFile(message.filePath);
					panel.webview.postMessage({ command: 'commandsParsed', commands });
				} else if (message.command === 'executeCommand') {
					executeCommand(message.command);
				}
			});
		}
	);

	context.subscriptions.push(disposable);

	// Create and show the webview automatically
	const panel = vscode.window.createWebviewPanel(
		'automationStudio',
		'Automation Studio',
		vscode.ViewColumn.Beside,
		{ enableScripts: true }
	);

	panel.webview.html = getWebviewContent();

	panel.webview.onDidReceiveMessage(async (message) => {
		if (message.command === 'loadMarkdownFiles') {
			const files = await findMarkdownFiles();
			panel.webview.postMessage({ command: 'filesLoaded', files });
		} else if (message.command === 'parseMarkdown') {
			const commands = await parseMarkdownFile(message.filePath);
			panel.webview.postMessage({ command: 'commandsParsed', commands });
		} else if (message.command === 'executeCommand') {
			await executeCommand(message.command, message.filePath);
		}
	});
}

async function findMarkdownFiles(): Promise<string[]> {
	const workspaceFolders = vscode.workspace.workspaceFolders;
	if (!workspaceFolders) {
		return [];
	}

	const files: string[] = [];
	for (const folder of workspaceFolders) {
		const fileUris = await vscode.workspace.findFiles(
			new vscode.RelativePattern(folder, '**/*.md'),
			'**/node_modules/**'
		);
		files.push(...fileUris.map(uri => uri.fsPath));
	}
	return files;
}

async function parseMarkdownFile(filePath: string): Promise<any[]> {
	try {
		const content = fs.readFileSync(filePath, 'utf-8');
		const lines = content.split('\n');
		const commands: any[] = [];

		let inCommandSection = false;
		let currentCommand: any = null;

		for (let i = 0; i < lines.length; i++) {
			const line = lines[i];

			// Detect command sections (## Commands, ## Tasks, etc.)
			if (line.match(/^##\s+(Commands|Tasks|Actions|Workflows)/i)) {
				inCommandSection = true;
				continue;
			}

			// Parse command items (###, -, or bullet points)
			if (inCommandSection) {
				if (line.match(/^###\s+/)) {
					if (currentCommand) {
						commands.push(currentCommand);
					}
					currentCommand = {
						name: line.replace(/^###\s+/, '').trim(),
						description: '',
						section: ''
					};
				} else if (line.match(/^-\s+\*\*/)) {
					if (currentCommand) {
						commands.push(currentCommand);
					}
					const match = line.match(/^-\s+\*\*([^*]+)\*\*/);
					currentCommand = {
						name: match ? match[1] : line.trim(),
						description: line.replace(/^-\s+\*\*[^*]+\*\*:\s*/, '').trim(),
						section: ''
					};
				} else if (line.trim() === '' && currentCommand) {
					// Empty line might signal end of command description
					continue;
				} else if (currentCommand && line.trim() !== '') {
					currentCommand.description += ' ' + line.trim();
				}
			}
		}

		if (currentCommand) {
			commands.push(currentCommand);
		}

		return commands;
	} catch (error) {
		console.error('Error parsing markdown:', error);
		return [];
	}
}

async function executeCommand(command: string, filePath: string): Promise<void> {
	const terminal = vscode.window.createTerminal('Automation Studio');
	terminal.show();
	terminal.sendText(`echo "Executing: ${command}"`);
	terminal.sendText(`echo "File: ${filePath}"`);
}

function getWebviewContent(): string {
	return `<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Automation Studio</title>
	<style>
		* {
			margin: 0;
			padding: 0;
			box-sizing: border-box;
		}

		body {
			font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
			background-color: var(--vscode-editor-background);
			color: var(--vscode-editor-foreground);
			padding: 16px;
		}

		.container {
			display: flex;
			flex-direction: column;
			height: 100vh;
			gap: 16px;
		}

		.header {
			display: flex;
			align-items: center;
			gap: 8px;
			padding-bottom: 12px;
			border-bottom: 1px solid var(--vscode-panel-border);
		}

		.header h1 {
			font-size: 18px;
			font-weight: 600;
		}

		.header-icon {
			font-size: 24px;
		}

		.content {
			display: flex;
			gap: 16px;
			flex: 1;
			min-height: 0;
		}

		.panel {
			flex: 1;
			border: 1px solid var(--vscode-panel-border);
			border-radius: 4px;
			overflow: hidden;
			display: flex;
			flex-direction: column;
			background-color: var(--vscode-editor-background);
		}

		.panel-header {
			padding: 12px;
			background-color: var(--vscode-sideBar-background);
			border-bottom: 1px solid var(--vscode-panel-border);
			font-weight: 600;
			font-size: 14px;
		}

		.panel-content {
			flex: 1;
			overflow-y: auto;
			padding: 12px;
		}

		.file-item, .command-item {
			padding: 10px;
			margin-bottom: 8px;
			background-color: var(--vscode-sideBar-background);
			border: 1px solid var(--vscode-panel-border);
			border-radius: 4px;
			cursor: pointer;
			transition: all 0.2s ease;
		}

		.file-item:hover, .command-item:hover {
			background-color: var(--vscode-list-hoverBackground);
			transform: translateX(4px);
		}

		.file-item.active {
			background-color: var(--vscode-list-activeSelectionBackground);
			color: var(--vscode-list-activeSelectionForeground);
		}

		.file-name {
			font-weight: 600;
			font-size: 13px;
			margin-bottom: 4px;
		}

		.file-path {
			font-size: 11px;
			color: var(--vscode-descriptionForeground);
			word-break: break-all;
		}

		.command-name {
			font-weight: 600;
			font-size: 13px;
			margin-bottom: 4px;
		}

		.command-description {
			font-size: 12px;
			color: var(--vscode-descriptionForeground);
		}

		.command-item.active {
			background-color: var(--vscode-list-activeSelectionBackground);
		}

		.execute-btn {
			width: 100%;
			padding: 8px;
			margin-top: 8px;
			background-color: var(--vscode-button-background);
			color: var(--vscode-button-foreground);
			border: none;
			border-radius: 4px;
			cursor: pointer;
			font-size: 12px;
			font-weight: 600;
			transition: all 0.2s ease;
		}

		.execute-btn:hover {
			background-color: var(--vscode-button-hoverBackground);
		}

		.empty-state {
			display: flex;
			align-items: center;
			justify-content: center;
			height: 100%;
			color: var(--vscode-descriptionForeground);
			text-align: center;
		}

		.loading {
			display: flex;
			align-items: center;
			justify-content: center;
			height: 100%;
			gap: 8px;
		}

		.spinner {
			width: 16px;
			height: 16px;
			border: 2px solid var(--vscode-descriptionForeground);
			border-top-color: transparent;
			border-radius: 50%;
			animation: spin 0.8s linear infinite;
		}

		@keyframes spin {
			to { transform: rotate(360deg); }
		}
	</style>
</head>
<body>
	<div class="container">
		<div class="header">
			<div class="header-icon">⚙️</div>
			<h1>Automation Studio</h1>
		</div>

		<div class="content">
			<!-- Left Panel: Markdown Files -->
			<div class="panel" style="flex: 0 0 35%;">
				<div class="panel-header">📄 Markdown Files</div>
				<div class="panel-content" id="filesPanel">
					<div class="loading">
						<div class="spinner"></div>
						<span>Loading files...</span>
					</div>
				</div>
			</div>

			<!-- Right Panel: Commands -->
			<div class="panel" style="flex: 0 0 65%;">
				<div class="panel-header">⚡ Commands</div>
				<div class="panel-content" id="commandsPanel">
					<div class="empty-state">
						Select a Markdown file to view available commands
					</div>
				</div>
			</div>
		</div>
	</div>

	<script>
		const vscode = acquireVsCodeApi();
		let selectedFile = null;
		let currentCommands = [];

		// Load markdown files on startup
		function loadFiles() {
			vscode.postMessage({ command: 'loadMarkdownFiles' });
		}

		window.addEventListener('message', (event) => {
			const message = event.data;

			if (message.command === 'filesLoaded') {
				displayFiles(message.files);
			} else if (message.command === 'commandsParsed') {
				displayCommands(message.commands);
			}
		});

		function displayFiles(files) {
			const filesPanel = document.getElementById('filesPanel');
			filesPanel.innerHTML = '';

			if (files.length === 0) {
				filesPanel.innerHTML = '<div class="empty-state">No Markdown files found</div>';
				return;
			}

			files.forEach((file) => {
				const fileName = file.split('/').pop();
				const fileItem = document.createElement('div');
				fileItem.className = 'file-item';
				fileItem.innerHTML = \`
					<div class="file-name">📄 \${fileName}</div>
					<div class="file-path">\${file}</div>
				\`;

				fileItem.addEventListener('click', () => {
					selectFile(file, fileItem);
				});

				filesPanel.appendChild(fileItem);
			});
		}

		function selectFile(file, element) {
			// Remove active class from all file items
			document.querySelectorAll('.file-item').forEach(item => {
				item.classList.remove('active');
			});

			// Add active class to clicked item
			element.classList.add('active');
			selectedFile = file;

			// Load commands for this file
			vscode.postMessage({ command: 'parseMarkdown', filePath: file });
		}

		function displayCommands(commands) {
			const commandsPanel = document.getElementById('commandsPanel');
			commandsPanel.innerHTML = '';

			if (commands.length === 0) {
				commandsPanel.innerHTML = '<div class="empty-state">No commands found in this file</div>';
				return;
			}

			commands.forEach((command, index) => {
				const commandItem = document.createElement('div');
				commandItem.className = 'command-item';
				commandItem.innerHTML = \`
					<div class="command-name">⚡ \${command.name}</div>
					\${command.description ? \`<div class="command-description">\${command.description}</div>\` : ''}
					<button class="execute-btn">Execute</button>
				\`;

				commandItem.querySelector('.execute-btn').addEventListener('click', () => {
					vscode.postMessage({
						command: 'executeCommand',
						command: command.name,
						filePath: selectedFile
					});
				});

				commandsPanel.appendChild(commandItem);
			});
		}

		// Initial load
		loadFiles();
	</script>
</body>
</html>`;
}

export function deactivate() {}
