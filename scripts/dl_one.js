const https=require('https'),fs=require('fs'),path=require('path');
const n=process.argv[2],u=process.argv[3];
const A=path.join(__dirname,'..','assets','audio','bgm');
const d=path.join(A,n);
fs.mkdirSync(A,{recursive:true});
if(fs.existsSync(d)&&fs.statSync(d).size>5000){console.log('SKIP '+n);process.exit(0);}
const f=fs.createWriteStream(d);
https.get(u,{headers:{'User-Agent':'Mozilla/5.0'},timeout:90000},res=>{
  if(res.statusCode>=300&&res.statusCode<400&&res.headers.location){f.close();fs.unlinkSync(d);const https2=require('https');https2.get(res.headers.location,{headers:{'User-Agent':'Mozilla/5.0'},timeout:90000},r=>{r.pipe(f);f.on('finish',()=>{f.close();console.log(fs.statSync(d).size>5000?'OK':'SMALL');});}).on('error',e=>{f.close();console.log('ERR '+e.message);});return;}
  res.pipe(f);f.on('finish',()=>{f.close();console.log(fs.statSync(d).size>5000?'OK '+n:'SMALL '+n);});
}).on('error',e=>{f.close();console.log('ERR '+e.message);});
