import { launch, defaultArgs } from "puppeteer";

console.log(defaultArgs());
const browser = await launch({
    headless: false,
    args: [
        "--no-sandbox",
        "--enable-unsafe-webgpu",
        "--enable-webgpu-developer-features",
        "--enable-features=WebAssemblyExperimentalJSPI,Vulkan,VulkanFromANGLE,DefaultANGLEVulkan",
    ],
    ignoreDefaultArgs: [
        "--headless=new"
    ]
});

const page = await browser.newPage();

// Navigate to chrome://version manually by setting the URL via evaluate or simply open a blank page
await page.goto('about:blank');

// Use Puppeteer to navigate the browser's address bar to chrome://version via evaluate
await page.goto("chrome://version")

// Wait for the page content to load
await page.waitForSelector('#command_line'); // The command line string is inside element with id "command_line"

// Extract the command line string
const commandLine = await page.$eval('#command_line', el => el.textContent);

console.log('Chrome command line:', commandLine);

await page.goto('chrome://gpu');

// Verify: log the WebGPU status or save the GPU report as PDF
const txt = await page.waitForSelector('text/WebGPU');
const status = await txt.evaluate(g => g.parentElement.textContent);
console.log(status);
await page.pdf({ path: './gpu.pdf' });

await browser.close();

export default browser;