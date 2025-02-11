import fs from "fs";
import path from "path";
import finalhandler from "finalhandler";
import { createServer } from "http";
import serveStatic from "serve-static";
import yargs from "yargs";
import { hideBin } from "yargs/helpers";

// Parse command-line arguments
const argv = yargs(hideBin(process.argv))
  //   .usage('Usage: $0 -d [dir] -i [index] -p [port]')
  .option("d", {
    alias: "dir",
    type: "string",
    description: "Directory to serve",
    demandOption: true,
  })
  .option("i", {
    alias: "index",
    type: "string",
    description: "Index file",
    default: "index.html",
  })
  .option("p", {
    alias: "port",
    type: "number",
    description: "Port to listen on",
    default: 3000,
  })
  .help().argv;

const fullPath = path.join(argv.dir, argv.index);
if (!fs.existsSync(argv.dir) || !fs.statSync(argv.dir).isDirectory()) {
  console.error(
    `Error: Directory "${argv.dir}" does not exist or is not a directory.`
  );
  process.exit(1);
}
if (!fs.existsSync(fullPath) || !fs.statSync(fullPath).isFile()) {
  console.error(
    `Error: Index file "${argv.index}" does not exist in directory "${argv.dir}".`
  );
  process.exit(1);
}

const serve = serveStatic(argv.dir, { index: [argv.index] });

// Create server
const server = createServer(function onRequest(req, res) {
  serve(req, res, finalhandler(req, res));
});

const PORT = argv.port;
// Listen
server.listen(PORT, () => console.log(`Serving at http://localhost:${PORT}`));
export default server;
