import { writeFileSync, mkdirSync, existsSync } from "fs";
import browser from "./browser.js";


async function main() {
    const page = await browser.newPage();
    // Extract the launch command
    await page.goto("chrome://version");
    const launchCommand = await page.evaluate(() => {
        const rows = Array.from(document.querySelectorAll("table tr"));
        for (const row of rows) {
            const label = row.querySelector("td:first-child")?.innerText?.trim();
            if (label === "Command Line") {
                return row.querySelector("td:last-child")?.innerText?.trim();
            }
        }
        return "Launch command not found";
    });
    // Save and log
    if (!existsSync("browser_data")) { mkdirSync("browser_data", {}); }
    writeFileSync("browser_data/launch-command.txt", launchCommand);
    console.log("Launch command saved to browser_data/launch-command.txt");
    // Navigate to chrome://gpu
    await page.goto("chrome://gpu");
    // Wait for the shadow DOM to load
    await page.waitForSelector("info-view");
    // Capture the whole page
    await page.evaluate(() => {
        const infoView = document.querySelector("info-view");
        const shadowRoot = infoView.shadowRoot;
        const content = shadowRoot.querySelector("#content");
        content.style.border = "2px solid red"; // Highlight the section
    });
    await page.screenshot({
        path: "browser_data/gpu-status.png",
        fullPage: true
    });
    console.log("Screenshot saved as browser_data/gpu-status.png");
    // Save the full GPU info to a text file
    const fullGpuInfo = await page.evaluate(() => {
        const infoView = document.querySelector("info-view");
        return infoView.shadowRoot.querySelector("#content").innerText;
    });
    writeFileSync("browser_data/gpu-info.txt", fullGpuInfo);
    console.log("Full GPU info saved to browser_data/gpu-info.txt");
    // Extract WebGPU status from shadow DOM
    const webgpuStatus = await page.evaluate(() => {
        const infoView = document.querySelector("info-view");
        const shadowRoot = infoView.shadowRoot;
        // Find the WebGPU status item
        const items = shadowRoot.querySelectorAll("#content li");
        for (const item of items) {
            if (item.textContent.includes("WebGPU:")) {
                const statusElement = item.querySelector(".feature-green, .feature-yellow, .feature-red");
                return statusElement ? statusElement.textContent.trim() : "Status not found";
            }
        }
        return "Not found";
    });
    console.log(`WebGPU Status: ${webgpuStatus}`);
    if (webgpuStatus == "Disabled" || webgpuStatus == "Not found") {
        console.log("Browser does not support WebGPU");
        process.exit(1);
    }
    // Try to access adapter
    const adapter_exists = await page.evaluate('(async () => { return (await navigator.gpu.requestAdapter()) != null; })()');
    console.log(`Adapter exists: ${adapter_exists}`);
    if (adapter_exists == false) {
        console.log("Failed to try get adapter");
        process.exit(1);
    }
    await browser.close();
    process.exit(0);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
