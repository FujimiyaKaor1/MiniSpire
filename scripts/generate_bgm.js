// Generate simple WAV BGM files using pure Node.js
const fs = require('fs');
const path = require('path');

const OUT = path.resolve(__dirname, '..', 'assets', 'audio', 'bgm');
fs.mkdirSync(OUT, { recursive: true });

function writeWAV(filename, sampleRate, samples) {
    const numSamples = samples.length;
    const dataSize = numSamples * 2;
    const buf = Buffer.alloc(44 + dataSize);
    buf.write('RIFF', 0);
    buf.writeUInt32LE(36 + dataSize, 4);
    buf.write('WAVE', 8);
    buf.write('fmt ', 12);
    buf.writeUInt32LE(16, 16);
    buf.writeUInt16LE(1, 20);
    buf.writeUInt16LE(1, 22);
    buf.writeUInt32LE(sampleRate, 24);
    buf.writeUInt32LE(sampleRate * 2, 28);
    buf.writeUInt16LE(2, 32);
    buf.writeUInt16LE(16, 34);
    buf.write('data', 36);
    buf.writeUInt32LE(dataSize, 40);
    for (let i = 0; i < numSamples; i++) {
        const s = Math.max(-1, Math.min(1, samples[i]));
        buf.writeInt16LE(Math.round(s * 32767), 44 + i * 2);
    }
    fs.writeFileSync(path.join(OUT, filename), buf);
    console.log(`OK ${filename} (${(buf.length / 1024).toFixed(1)}KB, ${(numSamples / sampleRate).toFixed(1)}s)`);
}

const SR = 44100;

function pad(freq, t, vol) {
    return vol * (Math.sin(2*Math.PI*freq*t)*0.5 + Math.sin(2*Math.PI*freq*1.002*t)*0.3 + Math.sin(2*Math.PI*freq*0.5*t)*0.2);
}

function envelope(t, a, d, sus, r, dur) {
    if (t < a) return t/a;
    if (t < a+d) return 1-(1-sus)*(t-a)/d;
    if (t < dur-r) return sus;
    return sus*(dur-t)/r;
}

function makeBGM(name, dur, genFunc) {
    const n = SR * dur;
    const samples = new Float64Array(n);
    for (let i = 0; i < n; i++) samples[i] = genFunc(i / SR);
    writeWAV(name, SR, samples);
}

// main_menu.wav - dark atmospheric
makeBGM('main_menu.wav', 8, (t) => {
    const e = envelope(t, 1, 2, 0.6, 2, 8);
    let s = pad(73.42, t, 0.15) + pad(92.50, t, 0.12) + pad(110.00, t, 0.10);
    s += Math.sin(2*Math.PI*146.83*t) * 0.05 * (0.5+0.5*Math.sin(t*0.2));
    s += Math.sin(2*Math.PI*55*t) * 0.08;
    s += (Math.random() * 0.02 - 0.01) * (0.3+0.7*Math.sin(t*0.1));
    return s * e * 0.5;
});

// battle.wav - tense minor chord with rhythmic pulse
makeBGM('battle.wav', 8, (t) => {
    const e = envelope(t, 0.5, 1, 0.7, 1, 8);
    let s = pad(130.81, t, 0.15) + pad(155.56, t, 0.12) + pad(196.00, t, 0.10);
    s += Math.pow(Math.max(0, Math.sin(t*12.56)), 4) * Math.sin(2*Math.PI*65.41*t) * 0.15;
    s += Math.sin(2*Math.PI*466.16*t) * 0.04 * Math.sin(t*0.5);
    s += Math.sin(2*Math.PI*32.7*t) * 0.08 * (0.5+0.5*Math.sin(t*0.3));
    return s * e * 0.6;
});

// campfire.wav - warm peaceful
makeBGM('campfire.wav', 8, (t) => {
    const e = envelope(t, 1, 1.5, 0.6, 1.5, 8);
    let s = pad(174.61, t, 0.12) + pad(220.00, t, 0.10) + pad(261.63, t, 0.08);
    s += (Math.random() > 0.997 ? Math.random()*0.3 : 0);
    const m = [349.23, 392.00, 440.00, 392.00];
    s += Math.sin(2*Math.PI*m[Math.floor(t*0.5)%4]*t) * 0.03;
    return s * e * 0.5;
});

// shop.wav - cheerful
makeBGM('shop.wav', 8, (t) => {
    const e = envelope(t, 0.3, 0.5, 0.7, 0.5, 8);
    let s = pad(293.66, t, 0.10) + pad(369.99, t, 0.08) + pad(440.00, t, 0.07);
    s += (Math.pow(Math.max(0, Math.sin(t*18.84)), 6)*0.5+0.5) * Math.sin(2*Math.PI*146.83*t) * 0.08;
    return s * e * 0.5;
});

// event.wav - mysterious
makeBGM('event.wav', 8, (t) => {
    const e = envelope(t, 1, 2, 0.5, 2, 8);
    let s = pad(123.47, t, 0.10) + pad(146.83, t, 0.08) + pad(174.61, t, 0.06) + pad(207.65, t, 0.05);
    s *= 0.7 + 0.3*Math.sin(t*0.4);
    s += Math.sin(2*Math.PI*830.61*t) * 0.02 * (0.5+0.5*Math.sin(t*0.2));
    return s * e * 0.5;
});

// defeat.wav - somber
makeBGM('defeat.wav', 8, (t) => {
    const e = envelope(t, 0.8, 2, 0.3, 2.5, 8);
    const p = Math.min(1, t/4), r = 220*(1-p*0.1);
    let s = pad(r, t, 0.12) + pad(r*1.2, t, 0.08) + pad(r*1.5, t, 0.06);
    const n = [440, 392, 349.23, 329.63];
    s += Math.sin(2*Math.PI*n[Math.floor(t*0.4)%4]*t) * 0.04 * (0.5+0.5*Math.sin(t*0.3));
    s += Math.sin(2*Math.PI*55*t) * 0.1;
    return s * e * 0.5;
});

// victory.wav - cheerful celebration
makeBGM('victory.wav', 8, (t) => {
    const e = envelope(t, 0.2, 0.8, 0.75, 1.2, 8);
    let s = pad(261.63, t, 0.12) + pad(329.63, t, 0.10) + pad(392.00, t, 0.08);
    const melody = [392.00, 440.00, 493.88, 523.25, 493.88, 440.00, 392.00, 329.63];
    s += Math.sin(2*Math.PI*melody[Math.floor(t*2)%8]*t) * 0.10 * (0.6+0.4*Math.sin(t*4));
    s += Math.pow(Math.max(0, Math.sin(t*6.28)), 6) * Math.sin(2*Math.PI*783.99*t) * 0.06;
    s += Math.sin(2*Math.PI*130.81*t) * 0.06;
    return s * e * 0.55;
});

// victory_boss.wav - epic triumph for final boss victory
makeBGM('victory_boss.wav', 10, (t) => {
    const e = envelope(t, 0.3, 1, 0.8, 1.5, 10);
    let s = pad(261.63, t, 0.12) + pad(329.63, t, 0.10) + pad(392.00, t, 0.08);
    s += pad(523.25, t, 0.06) + pad(659.25, t, 0.04);
    const triumph = [523.25, 587.33, 659.25, 783.99];
    s += Math.sin(2*Math.PI*triumph[Math.floor(t*0.8)%4]*t) * 0.08 * (0.6+0.4*Math.sin(t*2));
    s += Math.pow(Math.max(0, Math.sin(t*6.28)), 8) * Math.sin(2*Math.PI*1046.5*t) * 0.05;
    s += Math.sin(2*Math.PI*130.81*t) * 0.08 * (0.5+0.5*Math.sin(t*0.5));
    return s * e * 0.55;
});

console.log('All BGM generated!');
