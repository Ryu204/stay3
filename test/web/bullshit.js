import { launch } from 'puppeteer';
import { writeFileSync } from 'fs';

(async () => {
    let browser;
    try {
        browser = await launch({
            headless: false,
            executablePath: process.env.CHROME_PATH,
            args: [
                "--no-sandbox",
                "--enable-unsafe-webgpu",
                "--enable-webgpu-developer-features",
                "--enable-features=WebAssemblyExperimentalJSPI,Vulkan,VulkanFromANGLE,DefaultANGLEVulkan",
            ],
            ignoreDefaultArgs: ["--headless=new"]
        });

        const page = await browser.newPage();

        // Navigate to chrome://gpu
        await page.goto('chrome://gpu', { waitUntil: 'networkidle0' });

        // Wait for the shadow DOM to load
        await page.waitForSelector('info-view');

        // Extract WebGPU status from shadow DOM
        const webgpuStatus = await page.evaluate(() => {
            const infoView = document.querySelector('info-view');
            const shadowRoot = infoView.shadowRoot;

            // Find the WebGPU status item
            const items = shadowRoot.querySelectorAll('#content li');
            for (const item of items) {
                if (item.textContent.includes('WebGPU:')) {
                    const statusElement = item.querySelector('.feature-green, .feature-yellow, .feature-red');
                    return statusElement ? statusElement.textContent.trim() : 'Status not found';
                }
            }
            return 'WebGPU entry not found';
        });

        console.log(`WebGPU Status: ${webgpuStatus}`);

        // Take screenshot of the relevant section
        await page.evaluate(() => {
            const infoView = document.querySelector('info-view');
            const shadowRoot = infoView.shadowRoot;
            const content = shadowRoot.querySelector('#content');
            content.style.border = '2px solid red'; // Highlight the section
        });

        await page.screenshot({
            path: 'gpu-status.png',
            fullPage: true
        });

        console.log('Screenshot saved as gpu-status.png');

        // Save the full GPU info to a text file
        const fullGpuInfo = await page.evaluate(() => {
            const infoView = document.querySelector('info-view');
            return infoView.shadowRoot.querySelector('#content').innerText;
        });

        writeFileSync('gpu-info.txt', fullGpuInfo);
        console.log('Full GPU info saved to gpu-info.txt');

        // Try to access adapter
        const gpu = await page.evaluate('(async () => { return (await navigator.gpu.requestAdapter()) != null; })()');
        console.log(`Adapter exists: ${gpu !== null}`);
    } catch (error) {
        console.error('Error:', error);
    } finally {
        if (browser) {
            await browser.close();
        }
    }
})();