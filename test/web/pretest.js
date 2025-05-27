import puppeteer from "puppeteer";
// import browser from "./browser.js";

async function main() {
    console.log(puppeteer.defaultArgs())
    const browser = await puppeteer.launch({
        args: ["--enable-unsafe-webgpu"],
        ignoreDefaultArgs: [
            // "--headless=new",
            // "--disable-gpu"
        ]
    });
    const page = await browser.newPage();
    await page
        .goto('chrome://gpu', { waitUntil: 'networkidle0', timeout: 20 * 60 * 1000 })
        .catch(e => console.log(e));
    await page.screenshot({
        path: 'gpu_stats.png'
    });
    await browser.close();
    // const page = await browser.newPage();

    // const result = await page.evaluate(async () => {
    //     if (!navigator.gpu) return "WebGPU not available: navigator.gpu is undefined";
    //     const adapter = await navigator.gpu.requestAdapter();
    //     return adapter ? "ok" : "WebGPU not available: requestAdapter returned null";
    // });

    // let exitStatus = 0;
    // if (result !== "ok") {
    //     console.error(result);
    //     exitStatus = 1;
    // }

    // await browser.close();
    // process.exit(exitStatus);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
