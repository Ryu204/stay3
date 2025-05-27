#!/usr/bin/env node
import { readdirSync, statSync, mkdirSync, writeFileSync } from 'fs'
import { resolve, join, dirname, sep } from 'path'

const args = process.argv.slice(2)
let excludeRegex, output, dir

// Parse command-line arguments
for (let i = 0; i < args.length;) {
  const arg = args[i]
  if (arg === '-I') excludeRegex = new RegExp(args[++i])
  else if (arg === '-o') output = args[++i]
  else dir = arg
  i++
}

if (!dir) {
  console.error('Missing directory')
  process.exit(1)
}

// Check if path should be excluded (including parent directories)
function isExcluded(entryPath) {
  return entryPath.split(sep).some(segment => excludeRegex?.test(segment))
}

// Process directory entries (non-recursive)
function listDirectory(currentDir) {
  return readdirSync(currentDir, { withFileTypes: true })
    .filter(e => !isExcluded(e.name))
    .sort((a, b) => a.isDirectory() - b.isDirectory() || a.name.localeCompare(b.name))
    .map(e => ({
      name: e.name,
      path: join(currentDir, e.name),
      isDirectory: e.isDirectory()
    }))
}

// Generate HTML for a single directory
function generateHTML(entries, targetDir) {
  return `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>Index of ${targetDir}</title></head>
<body>
  <h1>Index of ${targetDir}</h1>
  <ul>
    ${entries.map(e => `<li><a href="${encodeURIComponent(e.name)}${e.isDirectory ? '/' : ''}">${e.name}${e.isDirectory ? '/' : ''}</a></li>`).join('\n    ')}
  </ul>
</body></html>`
}

// Main execution
try {
  const entries = listDirectory(dir)
  const html = generateHTML(entries, dir)
  const outPath = resolve(output || join(dir, 'index.html'))
  mkdirSync(dirname(outPath), { recursive: true })
  writeFileSync(outPath, html)
  console.log(`Index created at ${outPath}`)
} catch (err) {
  console.error(`Error: ${err.message}`)
  process.exit(1)
}