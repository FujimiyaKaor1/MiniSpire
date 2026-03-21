// Generate background PNGs using pure Node.js (no npm dependencies needed)
// Creates atmospheric gradient backgrounds for each game phase
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const W = 1280, H = 720;
const OUT = path.resolve(__dirname, '..', 'assets', 'images', 'backgrounds');
fs.mkdirSync(OUT, { recursive: true });

function clamp(v) { return Math.max(0, Math.min(255, Math.round(v))); }

function crc32(buf) {
    let crc = 0xFFFFFFFF;
    for (let i = 0; i < buf.length; i++) {
        crc ^= buf[i];
        for (let j = 0; j < 8; j++) {
            crc = (crc >>> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
        }
    }
    return (crc ^ 0xFFFFFFFF) >>> 0;
}

function createPNG(filename, pixelFunc) {
    const rawData = Buffer.alloc(W * H * 4);
    for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
            const [r, g, b, a] = pixelFunc(x, y);
            const i = (y * W + x) * 4;
            rawData[i] = clamp(r);
            rawData[i+1] = clamp(g);
            rawData[i+2] = clamp(b);
            rawData[i+3] = clamp(a || 255);
        }
    }
    const signature = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);
    function chunk(type, data) {
        const len = Buffer.alloc(4);
        len.writeUInt32BE(data.length);
        const typeB = Buffer.from(type);
        const crc = crc32(Buffer.concat([typeB, data]));
        const crcB = Buffer.alloc(4);
        crcB.writeUInt32BE(crc);
        return Buffer.concat([len, typeB, data, crcB]);
    }
    const ihdr = Buffer.alloc(13);
    ihdr.writeUInt32BE(W, 0);
    ihdr.writeUInt32BE(H, 4);
    ihdr[8] = 8; ihdr[9] = 6;
    const raw = Buffer.alloc(H * (1 + W * 4));
    for (let y = 0; y < H; y++) {
        raw[y * (1 + W * 4)] = 0;
        rawData.copy(raw, y * (1 + W * 4) + 1, y * W * 4, (y + 1) * W * 4);
    }
    const compressed = zlib.deflateSync(raw);
    const png = Buffer.concat([signature, chunk('IHDR', ihdr), chunk('IDAT', compressed), chunk('IEND', Buffer.alloc(0))]);
    fs.writeFileSync(path.join(OUT, filename), png);
    console.log(`OK ${filename} (${(png.length / 1024).toFixed(1)}KB)`);
}

function fbm(x, y, octaves) {
    let val = 0, amp = 1, freq = 1, max = 0;
    for (let o = 0; o < octaves; o++) {
        val += amp * (Math.sin(x * freq * 0.01 + y * freq * 0.007) * 0.5 + Math.cos(y * freq * 0.013 - x * freq * 0.005) * 0.5);
        max += amp; amp *= 0.5; freq *= 2.1;
    }
    return (val / max + 1) * 0.5;
}

// Main Menu
createPNG('main_menu.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x, y, 4);
    let r = 18 + n*20 + (1-ny)*15, g = 12 + n*12 + (1-ny)*8, b = 28 + n*25 + (1-ny)*10;
    const dx = (nx-0.5)*2, dy = (ny-0.35)*2;
    const glow = Math.exp(-(dx*dx+dy*dy)*1.8);
    r += glow*50; g += glow*30; b += glow*10;
    if (Math.abs(nx-0.5) < 0.02 && ny < 0.3) { r += 8; g += 5; b += 3; }
    const bottom = Math.pow(ny, 2);
    r -= bottom*15; g -= bottom*10; b -= bottom*8;
    return [r, g, b];
});

// Battle
createPNG('battle.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x+100, y+200, 5);
    let r = 22+n*18, g = 32+n*15, b = 42+n*20;
    const ty = (ny-0.2)*3;
    const t1 = Math.exp(-(((nx-0.85)*3)**2 + ty**2)*2);
    r += t1*60; g += t1*35; b += t1*10;
    const t2 = Math.exp(-(((nx-0.15)*3)**2 + ty**2)*2);
    r += t2*40; g += t2*25; b += t2*8;
    if (ny > 0.7) { const f = (ny-0.7)/0.3; r -= f*10; g -= f*8; b -= f*5; }
    if ((y%48) < 2 || ((x + (Math.floor(y/48)%2)*64) % 128) < 2) { r -= 5; g -= 5; b -= 4; }
    return [r, g, b];
});

// Campfire
createPNG('campfire.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x+50, y+50, 3);
    let r = 14+n*8, g = 18+n*6, b = 12+n*5;
    const dx = (nx-0.5)*2, dy = (ny-0.5)*2.5;
    const fire = Math.exp(-(dx*dx*1.5+dy*dy)*1.2);
    r += fire*80; g += fire*50; b += fire*15;
    const core = Math.exp(-(dx*dx*3+dy*dy*3)*1.5);
    r += core*40; g += core*30; b += core*5;
    return [r, g, b];
});

// Shop
createPNG('shop.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x+300, y+100, 3);
    let r = 35+n*15, g = 28+n*10, b = 18+n*8;
    r += Math.exp(-ny*4)*0.6*40; g += Math.exp(-ny*4)*0.6*30; b += Math.exp(-ny*4)*0.6*15;
    const shelf = Math.exp(-Math.abs(ny-0.35)*8);
    r += shelf*20; g += shelf*15; b += shelf*5;
    if (nx > 0.7 && ny > 0.4) { r += 8; g += 5; b += 2; }
    return [r, g, b];
});

// Event 1: 神殿祭坛 (Temple Altar) - 冷灰主调 + 暗金点缀
createPNG('event1.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x, y, 5);
    // 冷灰基调
    let r = 18+n*12, g = 20+n*10, b = 28+n*18;
    // 暗金点缀 - 中央祭坛光芒
    const dx = (nx-0.5)*2, dy = (ny-0.6)*2;
    const altar = Math.exp(-(dx*dx*0.8+dy*dy*1.2)*1.5);
    r += altar*60; g += altar*45; b += altar*15;
    // 神殿柱子暗影
    const pillar1 = Math.exp(-Math.abs(nx-0.25)*12) * (1 - ny);
    const pillar2 = Math.exp(-Math.abs(nx-0.75)*12) * (1 - ny);
    r -= (pillar1 + pillar2) * 10;
    g -= (pillar1 + pillar2) * 8;
    b -= (pillar1 + pillar2) * 5;
    // 顶部暗角
    const vignette = Math.pow(ny, 0.8) * 0.3;
    r *= (1 - vignette * 0.3);
    g *= (1 - vignette * 0.3);
    b *= (1 - vignette * 0.2);
    return [r, g, b];
});

// Event 2: 森林小路 (Forest Path) - 冷灰主调 + 幽暗绿光
createPNG('event2.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x+150, y+80, 6);
    // 冷灰森林基调
    let r = 12+n*10, g = 18+n*14, b = 16+n*12;
    // 幽暗绿光 - 从上方穿透树冠
    const canopy = Math.exp(-ny*3) * 0.5;
    r += canopy*15; g += canopy*35; b += canopy*20;
    // 路径光斑
    const path = Math.exp(-Math.abs(nx-0.5)*4) * Math.exp(-Math.abs(ny-0.7)*3);
    r += path*25; g += path*20; b += path*10;
    // 左右树木暗影
    const leftTree = Math.exp(-Math.abs(nx-0.15)*8);
    const rightTree = Math.exp(-Math.abs(nx-0.85)*8);
    r -= (leftTree + rightTree) * 8;
    g -= (leftTree + rightTree) * 6;
    b -= (leftTree + rightTree) * 4;
    // 雾气效果
    const fog = Math.pow(ny, 1.5) * 0.2;
    r += fog*20; g += fog*25; b += fog*30;
    return [r, g, b];
});

// Defeat
createPNG('defeat.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x+200, y, 3);
    let r = 18+n*12, g = 8+n*5, b = 10+n*6;
    const vig = Math.pow(Math.min(1, Math.max(Math.abs(nx-0.5), Math.abs(ny-0.5))*2), 1.2);
    r += vig*30; g += vig*3; b += vig*5;
    const center = Math.exp(-((nx-0.5)**2*4 + (ny-0.5)**2*4)*2);
    r -= center*10; g -= center*5; b -= center*5;
    return [r, g, b];
});

// Victory
createPNG('victory.png', (x, y) => {
    const ny = y/H, nx = x/W, n = fbm(x, y+400, 3);
    let r = 28+n*15, g = 22+n*12, b = 10+n*8;
    const dx = (nx-0.5)*2, dy = (ny-0.4)*2;
    const radiant = Math.exp(-(dx*dx*1.5+dy*dy)*1.0);
    r += radiant*60; g += radiant*45; b += radiant*15;
    r += Math.exp(-ny*3)*0.4*50; g += Math.exp(-ny*3)*0.4*40; b += Math.exp(-ny*3)*0.4*20;
    const sparkle = Math.pow(Math.max(0, Math.sin(nx*50+n*10)*Math.sin(ny*30)), 8);
    r += sparkle*60; g += sparkle*50; b += sparkle*20;
    return [r, g, b];
});

console.log('All backgrounds generated!');
