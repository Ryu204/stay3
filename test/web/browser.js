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

export default browser;