// Asset downloader for MiniSpireSFML
// All sources are free/commercial-use friendly
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const BASE = path.join(__dirname, '..', 'assets');
const IMG = path.join(BASE, 'images');
const AUD = path.join(BASE, 'audio');

// Ensure directories exist
const dirs = [
  path.join(IMG, 'backgrounds'),
  path.join(AUD, 'bgm'),
  path.join(AUD, 'sfx'),
  path.join(IMG, 'ui'),
  path.join(IMG, 'enemies'),
];

dirs.forEach(d => { fs.mkdirSync(d, { recursive: true }); });
console.log('Directories ready.');

function download(url, dest) {
  return new Promise((resolve) => {
    if (fs.existsSync(dest) && fs.statSync(dest).size > 100) {
      console.log(`  SKIP [exists] ${path.basename(dest)}`);
      resolve(true);
      return;
    }
    const file = fs.createWriteStream(dest);
    const protocol = url.startsWith('https') ? https : http;

    const req = protocol.get(url, {
      headers: { 'User-Agent': 'Mozilla/5.0' },
      timeout: 30000,
    }, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        // Follow redirect
        file.close();
        fs.unlinkSync(dest);
        download(res.headers.location, dest).then(resolve);
        return;
      }
      if (res.statusCode !== 200) {
        file.close();
        try { fs.unlinkSync(dest); } catch(e) {}
        console.log(`  FAIL [HTTP ${res.statusCode}] ${path.basename(dest)}`);
        resolve(false);
        return;
      }
      res.pipe(file);
      file.on('finish', () => {
        file.close();
        const size = fs.statSync(dest).size;
        if (size > 100) {
          console.log(`  OK ${path.basename(dest)} (${(size/1024).toFixed(1)}KB)`);
          resolve(true);
        } else {
          try { fs.unlinkSync(dest); } catch(e) {}
          console.log(`  FAIL [empty] ${path.basename(dest)}`);
          resolve(false);
        }
      });
    });

    req.on('error', (e) => {
      file.close();
      try { fs.unlinkSync(dest); } catch(ex) {}
      console.log(`  FAIL [${e.message}] ${path.basename(dest)}`);
      resolve(false);
    });

    req.on('timeout', () => {
      req.destroy();
      file.close();
      try { fs.unlinkSync(dest); } catch(e) {}
      console.log(`  FAIL [timeout] ${path.basename(dest)}`);
      resolve(false);
    });
  });
}

async function main() {
  const GITHUB = 'https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618';

  console.log('\n=== UI Icons (OpenMoji CC BY-SA 4.0) ===');
  await download(`${GITHUB}/2764.png`, `${IMG}/ui/stat_hp.png`);           // Heart
  await download(`${GITHUB}/26A1.png`, `${IMG}/ui/stat_energy.png`);       // Lightning
  await download(`${GITHUB}/1F6E1.png`, `${IMG}/ui/stat_block.png`);       // Shield
  await download(`${GITHUB}/1F4AA.png`, `${IMG}/ui/stat_strength.png`);    // Flexed arm
  await download(`${GITHUB}/1F635.png`, `${IMG}/ui/stat_weak.png`);        // Dizzy
  await download(`${GITHUB}/2B50.png`, `${IMG}/ui/icon_star.png`);         // Star
  await download(`${GITHUB}/1F525.png`, `${IMG}/ui/icon_fire.png`);        // Fire
  await download(`${GITHUB}/1F480.png`, `${IMG}/ui/icon_skull.png`);       // Skull
  await download(`${GITHUB}/1F3C6.png`, `${IMG}/ui/icon_trophy.png`);      // Trophy
  await download(`${GITHUB}/1F3EA.png`, `${IMG}/ui/icon_shop.png`);        // Shop
  await download(`${GITHUB}/1F4DC.png`, `${IMG}/ui/icon_scroll.png`);      // Scroll

  console.log('\n=== Enemy Portraits (OpenMoji CC BY-SA 4.0) ===');
  await download(`${GITHUB}/1F4AA.png`, `${IMG}/enemies/enemy4.png`);      // Knight (swordsman)
  await download(`${GITHUB}/1F6E1.png`, `${IMG}/enemies/elite1.png`);      // Armor knight
  await download(`${GITHUB}/1F94A.png`, `${IMG}/enemies/elite2.png`);      // Blood fighter
  await download(`${GITHUB}/1F52E.png`, `${IMG}/enemies/elite3.png`);      // Curse priest

  console.log('\n=== Sound Effects (Mixkit Free License) ===');
  await download('https://assets.mixkit.co/active_storage/sfx/2570/2570-preview.mp3', `${AUD}/sfx/sword_attack.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2000/2000-preview.mp3', `${AUD}/sfx/player_hit.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/446/446-preview.mp3', `${AUD}/sfx/enemy_hit.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2567/2567-preview.mp3', `${AUD}/sfx/card_play.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2002/2000-preview.mp3', `${AUD}/sfx/block.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2556/2556-preview.mp3', `${AUD}/sfx/heal.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2869/2869-preview.mp3', `${AUD}/sfx/defeat.mp3`);
  await download('https://assets.mixkit.co/active_storage/sfx/2875/2875-preview.mp3', `${AUD}/sfx/victory.mp3`);

  console.log('\n=== Background Music (Mixkit Free License) ===');
  await download('https://assets.mixkit.co/music/preview/mixkit-deep-dark-ambient-cinematic-background-2410.mp3', `${AUD}/bgm/main_menu.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-epic-cinematic-orchestral-trailer-690.mp3', `${AUD}/bgm/battle.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-winding-clock-ticking-3060.mp3', `${AUD}/bgm/campfire.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-serenity-340.mp3', `${AUD}/bgm/shop.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-mystery-cinematic-trailer-784.mp3', `${AUD}/bgm/event.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-deep-dark-ominous-trailer-784.mp3', `${AUD}/bgm/defeat.mp3`);
  await download('https://assets.mixkit.co/music/preview/mixkit-achievement-celebration-melody-469.mp3', `${AUD}/bgm/victory.mp3`);

  console.log('\n=== Summary ===');
  const countFiles = (dir) => {
    let c = 0; try { fs.readdirSync(dir).forEach(f => { if(fs.statSync(path.join(dir,f)).isFile()) c++; }); } catch(e){}
    return c;
  };
  console.log(`UI icons: ${countFiles(path.join(IMG, 'ui'))}`);
  console.log(`Enemies: ${countFiles(path.join(IMG, 'enemies'))}`);
  console.log(`SFX: ${countFiles(path.join(AUD, 'sfx'))}`);
  console.log(`BGM: ${countFiles(path.join(AUD, 'bgm'))}`);
  console.log('\nDone. Background textures will be generated procedurally in code.');
}

main().catch(console.error);
