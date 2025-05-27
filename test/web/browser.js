import { launch } from "puppeteer";

const browser = await launch({
    headless: false,
    // This env var is set on CI. If it's null, puppeteer default to its own version of Chrome
    executablePath: process.env.CHROME_PATH,
    args: [
        "--no-sandbox",
        "--disable-gpu-sandbox",
        "--ignore-gpu-blocklist",
        "--enable-unsafe-webgpu",
        "--enable-webgpu-developer-features",
        "--enable-features=WebAssemblyExperimentalJSPI,Vulkan,VulkanFromANGLE,DefaultANGLEVulkan",
    ],
    ignoreDefaultArgs: [
        "--headless=new",
        "--disable-gpu",
    ],
});
export default browser;