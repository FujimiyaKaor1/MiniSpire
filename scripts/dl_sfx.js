const https = require('https');
const fs = require('fs');
const path = require('path');
const AUD = path.join(__dirname, '..', 'assets', 'audio');
[AUD+'/sfx',AUD+'/bgm'].forEach(d => fs.mkdirSync(d,{recursive:true}));
function dl(url,dest){
  return new Promise(r=>{
    if(fs.existsSync(dest)&&fs.statSync(dest).size>100){console.log('SKIP '+path.basename(dest));r(1);return;}
    const f=fs.createWriteStream(dest);
    https.get(url,{headers:{'User-Agent':'Mozilla/5.0'},timeout:30000},res=>{
      if(res.statusCode>=300&&res.statusCode<400&&res.headers.location){f.close();fs.unlinkSync(dest);dl(res.headers.location,dest).then(r);return;}
      if(res.statusCode!==200){f.close();try{fs.unlinkSync(dest);}catch(e){}console.log('FAIL '+path.basename(dest)+' HTTP'+res.statusCode);r(0);return;}
      res.pipe(f);f.on('finish',()=>{f.close();const s=fs.statSync(dest).size;s>100?console.log('OK '+path.basename(dest)+' '+(s/1024).toFixed(0)+'KB'):(()=>{try{fs.unlinkSync(dest);}catch(e){}console.log('FAIL empty '+path.basename(dest));r(0);})();r(1);});
    }).on('error',e=>{f.close();try{fs.unlinkSync(dest);}catch(x){}console.log('FAIL '+path.basename(dest)+' '+e.message);r(0);});
  });
}
(async()=>{
  console.log('=== Sound Effects (Mixkit Free License) ===');
  await dl('https://assets.mixkit.co/active_storage/sfx/2570/2570-preview.mp3',AUD+'/sfx/sword_attack.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2000/2000-preview.mp3',AUD+'/sfx/player_hit.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/446/446-preview.mp3',AUD+'/sfx/enemy_hit.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2567/2567-preview.mp3',AUD+'/sfx/card_play.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2002/2000-preview.mp3',AUD+'/sfx/block.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2556/2556-preview.mp3',AUD+'/sfx/heal.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2869/2869-preview.mp3',AUD+'/sfx/defeat.mp3');
  await dl('https://assets.mixkit.co/active_storage/sfx/2875/2875-preview.mp3',AUD+'/sfx/victory.mp3');
  console.log('SFX done');
})();
