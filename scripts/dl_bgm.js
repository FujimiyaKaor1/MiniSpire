const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const BASE = path.resolve(__dirname, '..', 'assets');

function download(url, dest) {
    return new Promise((resolve, reject) => {
        const mod = url.startsWith('https') ? https : http;
        const file = fs.createWriteStream(dest);
        let redirected = false;
        
        function doReq(reqUrl) {
            mod.get(reqUrl, (res) => {
                if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                    if (redirected) { reject(new Error('Too many redirects')); return; }
                    redirected = true;
                    doReq(res.headers.location);
                    return;
                }
                if (res.statusCode !== 200) {
                    reject(new Error(`HTTP ${res.statusCode}`));
                    return;
                }
                res.pipe(file);
                file.on('finish', () => {
                    file.close();
                    resolve(fs.statSync(dest).size);
                });
            }).on('error', (e) => {
                fs.unlink(dest, () => {});
                reject(e);
            });
        }
        doReq(url);
    });
}

async function tryDownload(url, dest) {
    try {
        const size = await download(url, dest);
        if (size > 1000) return true;
        fs.unlinkSync(dest);
        return false;
    } catch {
        try { fs.unlinkSync(dest); } catch {}
        return false;
    }
}

async function main() {
    // Download remaining BGM from various free sources
    const bgmDir = path.join(BASE, 'audio', 'bgm');
    fs.mkdirSync(bgmDir, { recursive: true });

    const bgmFiles = [
        ['battle.mp3', 'https://cdn.pixabay.com/audio/2022/02/22/audio_d1718ab41b.mp3'],
        ['campfire.mp3', 'https://cdn.pixabay.com/audio/2022/01/18/audio_d0a13f69d2.mp3'],
        ['shop.mp3', 'https://cdn.pixabay.com/audio/2021/11/13/audio_cb4b57327e.mp3'],
        ['event.mp3', 'https://cdn.pixabay.com/audio/2022/10/25/audio_5499e4e7e6.mp3'],
        ['defeat.mp3', 'https://cdn.pixabay.com/audio/2024/11/04/audio_43e6e3b3d8.mp3'],
    ];

    for (const [name, url] of bgmFiles) {
        const dest = path.join(bgmDir, name);
        if (fs.existsSync(dest) && fs.statSync(dest).size > 1000) {
            console.log(`SKIP ${name} (exists, ${fs.statSync(dest).size} bytes)`);
            continue;
        }
        console.log(`Downloading ${name}...`);
        const ok = await tryDownload(url, dest);
        console.log(ok ? `OK ${name}` : `FAIL ${name}`);
    }

    console.log('Done.');
}

main().catch(console.error);
