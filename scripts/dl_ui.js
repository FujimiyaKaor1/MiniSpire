const https = require('https');
const fs = require('fs');
const path = require('path');
const IMG = path.join(__dirname, '..', 'assets', 'images');
const AUD = path.join(__dirname, '..', 'assets', 'audio');
[IMG+'/backgrounds', AUD+'/bgm', AUD+'/sfx'].forEach(d => fs.mkdirSync(d,{recursive:true}));

function dl(url,dest){
  return new Promise(r=>{
    if(fs.existsSync(dest)&&fs.statSync(dest).size>50){console.log('SKIP '+path.basename(dest));r(1);return;}
    const f=fs.createWriteStream(dest);
    https.get(url,{headers:{'User-Agent':'Mozilla/5.0'},timeout:20000},res=>{
      if(res.statusCode>=300&&res.statusCode<400&&res.headers.location){f.close();fs.unlinkSync(dest);dl(res.headers.location,dest).then(r);return;}
      if(res.statusCode!==200){f.close();try{fs.unlinkSync(dest);}catch(e){}console.log('FAIL '+path.basename(dest)+' HTTP'+res.statusCode);r(0);return;}
      res.pipe(f);f.on('finish',()=>{f.close();fs.statSync(dest).size>50?console.log('OK '+path.basename(dest)):(()=>{try{fs.unlinkSync(dest);}catch(e){}console.log('FAIL empty '+path.basename(dest));r(0);})();r(1);});
    }).on('error',e=>{f.close();try{fs.unlinkSync(dest);}catch(x){}console.log('FAIL '+path.basename(dest)+' '+e.code);r(0);});
  });
}
(async()=>{
  const G='https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618';
  await dl(G+'/2764.png',IMG+'/ui/stat_hp.png');
  await dl(G+'/26A1.png',IMG+'/ui/stat_energy.png');
  await dl(G+'/1F6E1.png',IMG+'/ui/stat_block.png');
  await dl(G+'/1F4AA.png',IMG+'/ui/stat_strength.png');
  await dl(G+'/1F635.png',IMG+'/ui/stat_weak.png');
  await dl(G+'/2B50.png',IMG+'/ui/icon_star.png');
  await dl(G+'/1F525.png',IMG+'/ui/icon_fire.png');
  await dl(G+'/1F480.png',IMG+'/ui/icon_skull.png');
  await dl(G+'/1F3C6.png',IMG+'/ui/icon_trophy.png');
  await dl(G+'/1F3EA.png',IMG+'/ui/icon_shop.png');
  await dl(G+'/1F4DC.png',IMG+'/ui/icon_scroll.png');
  await dl(G+'/1F4AA.png',IMG+'/enemies/enemy4.png');
  await dl(G+'/1F6E1.png',IMG+'/enemies/elite1.png');
  await dl(G+'/1F94A.png',IMG+'/enemies/elite2.png');
  await dl(G+'/1F52E.png',IMG+'/enemies/elite3.png');
  console.log('UI done');
})();
