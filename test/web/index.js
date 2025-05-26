import { launch } from "puppeteer";
import server from "./server.js";

async function main() {
  const browser = await launch({
    headless: "new",
    args: [
      "--enable-unsafe-webgpu",
      "--no-sandbox",
      "--enable-features=WebAssemblyExperimentalJSPI"
    ],
  });
  const page = await browser.newPage();

  page.on("console", (msg) => {
    console.log(`${msg.type().toUpperCase()}: ${msg.text()}`);
  });

  // Load localhost test shell file
  const url = `http://localhost:${server.address().port}`;
  await page.goto(url);

  // Wait for Emscripten exit
  await page.waitForFunction("window.testExitStatus !== undefined", {
    timeout: 30_000,
  });

  // Get exit status
  const exitStatus = await page.evaluate(() => window.testExitStatus);

  await browser.close();
  server.close();

  // Propagate exit code (0 for success, non-zero for failure)
  process.exit(exitStatus);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
