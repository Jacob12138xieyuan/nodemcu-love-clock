#pragma once
// 设置网页（内嵌在固件里，不需要另外上传文件）
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="zh"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>爱的小时钟</title>
<style>
:root{--bg:#fff7f9;--card:#fff;--ink:#3a2a30;--dim:#8a7780;--pink:#e8567f;--line:#f0dde3}
@media (prefers-color-scheme:dark){:root{--bg:#1c1518;--card:#2a2024;--ink:#f3e6ea;--dim:#a8939b;--pink:#ff7aa0;--line:#3d2f35}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 -apple-system,"PingFang SC","Microsoft YaHei",sans-serif;padding-bottom:84px}
header{padding:16px 16px 8px}
h1{margin:0;font-size:20px}
#st{font-size:13px;color:var(--dim);margin-top:2px}
nav{display:flex;gap:6px;overflow-x:auto;padding:8px 16px;position:sticky;top:0;background:var(--bg);z-index:2}
nav button{flex:none;border:1px solid var(--line);background:var(--card);color:var(--ink);border-radius:18px;padding:6px 14px;font-size:15px}
nav button.on{background:var(--pink);border-color:var(--pink);color:#fff}
main{padding:0 16px}
section{display:none;background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-top:8px}
section.on{display:block}
label{display:block;font-size:14px;color:var(--dim);margin:12px 0 4px}
label:first-child{margin-top:0}
input,select,textarea{width:100%;font:inherit;color:var(--ink);background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:8px 10px}
textarea{min-height:200px}
input[type=checkbox]{width:auto}
input[type=range]{padding:0}
.row{display:flex;gap:8px;align-items:center}
.row>*{flex:1;min-width:0}
.row>.fix{flex:none}
button.b{border:0;background:var(--pink);color:#fff;border-radius:8px;padding:9px 16px;font:inherit}
button.g{border:1px solid var(--line);background:transparent;color:var(--ink);border-radius:8px;padding:8px 12px;font:inherit}
.hint{font-size:13px;color:var(--dim);margin:6px 0}
.ev{border:1px solid var(--line);border-radius:10px;padding:10px;margin-top:8px}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin-top:10px}
.ph{border:1px solid var(--line);border-radius:10px;padding:6px}
.ph img{width:100%;aspect-ratio:1;object-fit:cover;border-radius:6px;display:block;background:var(--line)}
.ph input{margin-top:6px;font-size:14px;padding:5px 8px}
.ph button{margin-top:6px;width:100%}
.wifi{display:flex;justify-content:space-between;padding:10px;border:1px solid var(--line);border-radius:8px;margin-top:6px;cursor:pointer}
.wifi.on{border-color:var(--pink);background:rgba(232,86,127,.1)}
footer{position:fixed;left:0;right:0;bottom:0;padding:12px 16px;background:var(--bg);border-top:1px solid var(--line)}
footer button{width:100%;font-size:17px;padding:12px}
footer button:disabled{opacity:.92}
footer button.ok{background:#2e9e5b}
footer button.bad{background:#c0392b}
.spin{display:inline-block;width:14px;height:14px;border:2px solid rgba(255,255,255,.4);border-top-color:#fff;border-radius:50%;animation:sp .7s linear infinite;vertical-align:-2px;margin-right:8px}
@keyframes sp{to{transform:rotate(360deg)}}
#toast{position:fixed;left:50%;top:70px;transform:translateX(-50%);background:#333;color:#fff;padding:8px 16px;border-radius:20px;font-size:14px;opacity:0;transition:opacity .3s;pointer-events:none;z-index:9;max-width:90%}
#toast.on{opacity:.95}
</style></head><body>
<header><h1>💕 爱的小时钟</h1><div id="st">连接中…</div></header>
<nav id="tabs">
<button data-t="wifi">Wi-Fi</button><button data-t="basic" class="on">基本</button><button data-t="events">纪念日</button>
<button data-t="quotes">情话</button><button data-t="photos">照片</button><button data-t="display">显示</button>
</nav>
<main>
<section id="wifi">
 <div class="hint" id="wst"></div>
 <button class="g" id="scan">扫描附近的 Wi-Fi</button>
 <div id="wlist"></div>
 <label>名称（也可以手动输入）</label><input id="ssid" autocapitalize="off" autocomplete="off">
 <label>密码</label><input id="wpass" type="password" autocomplete="off">
 <p class="row"><button class="b" id="wgo">保存并连接</button><button class="g" id="wforget">忘记 Wi-Fi 并重启</button></p>
 <div class="hint" id="wmsg"></div>
</section>
<section id="basic" class="on">
 <label>在一起的日期</label><input type="date" id="start">
 <label>所在城市（用于天气）</label>
 <div class="row"><input id="city" placeholder="如：上海"><button class="g fix" id="geo">查找</button></div>
 <div id="geores"></div>
 <div class="row"><div><label>纬度</label><input id="lat" type="number" step="0.001"></div><div><label>经度</label><input id="lon" type="number" step="0.001"></div></div>
 <div class="hint">查找需要手机能上网。也可以直接填经纬度。</div>
 <label>每页停留（秒）</label><input id="slide" type="number" min="3" max="60">
</section>
<section id="events">
 <div class="hint">最多 10 个。勾选"当天全屏庆祝"后，那天屏幕会一整天显示庆祝画面。「在一起纪念日」会根据基本页的日期自动加入，不用再填。</div>
 <div id="evlist"></div>
 <p><button class="g" id="evadd">＋ 添加纪念日</button></p>
</section>
<section id="quotes">
 <div class="hint">一行一句，建议每句不超过 40 个字，最多约 50 句。每次切到情话页会随机显示一句。</div>
 <textarea id="qtxt" placeholder="今天也很喜欢你&#10;记得多喝水"></textarea>
</section>
<section id="photos">
 <div class="hint">选照片后会自动裁成正方形并压缩，选完立即上传保存。最多 50 张，每张约 20KB。</div>
 <input type="file" id="pfile" accept="image/*" multiple>
 <div class="grid" id="pgrid"></div>
</section>
<section id="display">
 <div class="row"><div><label>白天亮度 <span id="bv"></span>%</label><input id="bright" type="range" min="5" max="100"></div>
 <div><label>夜间亮度 <span id="nbv"></span>%</label><input id="nbright" type="range" min="0" max="100"></div></div>
 <div class="row"><div><label>夜间开始（点）</label><input id="ns" type="number" min="0" max="23"></div><div><label>夜间结束（点）</label><input id="ne" type="number" min="0" max="23"></div></div>
 <div class="hint">开始和结束相同表示不调暗。夜间亮度设 0 就是熄屏。</div>
 <label>预览：假装今天是（测试庆祝页用，用完记得清空）</label><input type="date" id="test">
 <label>设置页密码（仅限英文数字，最多 15 位）</label>
 <div class="row"><input id="npass" placeholder="" autocomplete="off"><button class="g fix" id="npclear">取消密码</button></div>
 <div class="hint" id="pstate"></div>
</section>
</main>
<footer><button class="b" id="save">保存设置</button></footer>
<div id="toast"></div>
<script>
const $=s=>document.querySelector(s);
let C={events:[],photos:[]},pass=sessionStorage.getItem('p')||'',newPass=null,scanning=false;
function toast(m){const t=$('#toast');t.textContent=m;t.classList.add('on');clearTimeout(toast.h);toast.h=setTimeout(()=>t.classList.remove('on'),2200)}
async function api(path,opt={}){
  opt.headers=Object.assign({'X-Pass':pass},opt.headers||{});
  const r=await fetch(path,opt);
  if(r.status===401){const p=prompt('请输入设置密码');if(p===null)throw new Error('auth');pass=p;sessionStorage.setItem('p',p);return api(path,opt)}
  return r;
}
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));

document.querySelectorAll('#tabs button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('#tabs button').forEach(x=>x.classList.toggle('on',x===b));
  document.querySelectorAll('section').forEach(s=>s.classList.toggle('on',s.id===b.dataset.t));
  $('footer').style.display=b.dataset.t==='wifi'?'none':'';
});

// ---- 状态 ----
let S={};
async function status(){
  try{
    S=await (await fetch('/api/status')).json();
    const w=S.sta?`已连接 ${S.ssid}（${S.ip}）`:(S.ap?'未连接路由器，正在使用热点':'未连接');
    $('#st').textContent=w+(S.time?` · ${S.time}`:'')+` · 剩余空间 ${Math.round(S.free/1024)}KB`;
    $('#wst').textContent=S.sta?`当前已连接：${S.ssid}，设备地址 ${S.ip}`:'当前未连接 Wi-Fi';
  }catch(e){$('#st').textContent='无法连接设备'}
}

// ---- 加载与渲染 ----
async function load(){
  const r=await api('/api/config');C=await r.json();
  $('#start').value=C.start||'';$('#city').value=C.city||'';$('#lat').value=C.lat;$('#lon').value=C.lon;
  $('#slide').value=C.slide;$('#bright').value=C.bright;$('#nbright').value=C.nightBright;
  $('#ns').value=C.nightStart;$('#ne').value=C.nightEnd;$('#test').value=C.testDate||'';
  $('#bv').textContent=C.bright;$('#nbv').textContent=C.nightBright;
  $('#pstate').textContent=C.hasPass?'当前已设置密码':'当前没有密码';
  $('#npass').placeholder=C.hasPass?'输入新密码以修改':'输入密码以开启';
  const q=await api('/api/quotes');$('#qtxt').value=await q.text();
  renderEvents();renderPhotos();
}
$('#bright').oninput=e=>$('#bv').textContent=e.target.value;
$('#nbright').oninput=e=>$('#nbv').textContent=e.target.value;

function renderEvents(){
  $('#evlist').innerHTML=C.events.map((e,i)=>`<div class="ev">
   <input data-i="${i}" data-k="n" maxlength="10" placeholder="名称，如：她的生日" value="${esc(e.n)}">
   <div class="row" style="margin-top:8px"><input data-i="${i}" data-k="m" type="number" min="1" max="12" value="${e.m}"><span class="fix">月</span>
   <input data-i="${i}" data-k="d" type="number" min="1" max="31" value="${e.d}"><span class="fix">日</span></div>
   <div class="row" style="margin-top:8px"><label style="margin:0;color:var(--ink)"><input data-i="${i}" data-k="c" type="checkbox" ${e.c?'checked':''}> 当天全屏庆祝</label>
   <button class="g fix" data-del="${i}">删除</button></div></div>`).join('');
  document.querySelectorAll('#evlist input').forEach(el=>el.oninput=()=>{
    const e=C.events[+el.dataset.i],k=el.dataset.k;
    e[k]=k==='c'?el.checked:(k==='n'?el.value:+el.value)});
  document.querySelectorAll('#evlist [data-del]').forEach(b=>b.onclick=()=>{C.events.splice(+b.dataset.del,1);renderEvents()});
}
$('#evadd').onclick=()=>{if(C.events.length>=10)return toast('最多 10 个');C.events.push({n:'',m:1,d:1,c:true});renderEvents()};

function renderPhotos(){
  $('#pgrid').innerHTML=C.photos.map((p,i)=>`<div class="ph"><img loading="lazy" src="/ph?f=${p.f}">
   <input data-i="${i}" maxlength="15" placeholder="照片下方的小字" value="${esc(p.c)}">
   <button class="g" data-del="${i}">删除</button></div>`).join('');
  document.querySelectorAll('#pgrid input').forEach(el=>el.oninput=()=>C.photos[+el.dataset.i].c=el.value);
  document.querySelectorAll('#pgrid [data-del]').forEach(b=>b.onclick=async()=>{
    const p=C.photos[+b.dataset.del];if(!confirm('删除这张照片？'))return;
    await api('/api/photo/delete?f='+p.f,{method:'POST'});C.photos.splice(+b.dataset.del,1);renderPhotos();await save(true);status()});
}
async function crop(file){
  let bmp;try{bmp=await createImageBitmap(file,{imageOrientation:'from-image'})}catch(e){bmp=await createImageBitmap(file)}
  const s=Math.min(bmp.width,bmp.height),cv=document.createElement('canvas');cv.width=cv.height=240;
  cv.getContext('2d').drawImage(bmp,(bmp.width-s)/2,(bmp.height-s)/2,s,s,0,0,240,240);
  return new Promise(r=>cv.toBlob(r,'image/jpeg',0.82));
}
$('#pfile').onchange=async e=>{
  const files=[...e.target.files];e.target.value='';let n=0;
  for(const f of files){
    if(C.photos.length>=50){toast('最多 50 张');break}
    toast(`上传中 ${++n}/${files.length}…`);
    try{
      const blob=await crop(f),name='p'+(Date.now()%1e9)+'.jpg',fd=new FormData();
      fd.append('file',blob,name);
      const r=await api('/api/photo',{method:'POST',body:fd});
      if(!r.ok){toast('上传失败，空间可能已满');break}
      C.photos.push({f:name,c:''});renderPhotos();
    }catch(err){toast('上传失败');break}
  }
  await save(true);status();
};

// ---- 城市查找 ----
$('#geo').onclick=async()=>{
  const q=$('#city').value.trim();if(!q)return;
  try{
    const r=await fetch('https://geocoding-api.open-meteo.com/v1/search?count=5&language=zh&name='+encodeURIComponent(q));
    const j=await r.json();
    if(!j.results){$('#geores').innerHTML='<div class="hint">没找到，可以直接填经纬度</div>';return}
    $('#geores').innerHTML=j.results.map((x,i)=>`<div class="wifi" data-i="${i}">${esc(x.name)} <span class="hint">${esc([x.admin1,x.country].filter(Boolean).join(' '))}</span></div>`).join('');
    document.querySelectorAll('#geores .wifi').forEach(d=>d.onclick=()=>{
      const x=j.results[+d.dataset.i];$('#city').value=x.name;$('#lat').value=x.latitude.toFixed(3);$('#lon').value=x.longitude.toFixed(3);$('#geores').innerHTML=''});
  }catch(e){toast('查找失败，请确认手机能上网')}
};

// ---- 密码 ----
$('#npass').oninput=e=>{const v=e.target.value;if(/[^A-Za-z0-9]/.test(v)||v.length>15){e.target.value=v.replace(/[^A-Za-z0-9]/g,'').slice(0,15)}newPass=e.target.value||null;
  $('#pstate').textContent=newPass?'保存后生效':''};
$('#npclear').onclick=()=>{newPass='';$('#npass').value='';$('#pstate').textContent='保存后将取消密码'};

// ---- 保存 ----
function collect(){
  const o={start:$('#start').value,city:$('#city').value.trim(),lat:+$('#lat').value,lon:+$('#lon').value,
   slide:+$('#slide').value,bright:+$('#bright').value,nightBright:+$('#nbright').value,
   nightStart:+$('#ns').value,nightEnd:+$('#ne').value,testDate:$('#test').value,
   events:C.events.filter(e=>e.n.trim()),photos:C.photos};
  if(newPass!==null)o.newPass=newPass;
  return o;
}
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
let saving=false;
async function save(quiet){
  if(saving)return;
  saving=true;
  const btn=$('#save'),t0=Date.now();
  if(!quiet){btn.disabled=true;btn.innerHTML='<span class="spin"></span>保存中…'}
  let ok=true;
  try{
    const r=await api('/api/config',{method:'POST',body:JSON.stringify(collect())});
    if(!r.ok)throw 0;
    if(newPass!==null){pass=newPass;sessionStorage.setItem('p',pass);C.hasPass=!!newPass;newPass=null;$('#npass').value='';
      $('#pstate').textContent=C.hasPass?'当前已设置密码':'当前没有密码'}
    const q=await api('/api/quotes',{method:'POST',body:$('#qtxt').value.split('\n').map(s=>s.trim()).filter(Boolean).join('\n')});
    if(!q.ok)throw 0;
  }catch(e){ok=false}
  if(quiet){if(!ok)toast('保存失败');saving=false;return}
  const wait=700-(Date.now()-t0);if(wait>0)await sleep(wait);   // 保存很快，至少让"保存中"显示一会儿
  btn.innerHTML=ok?'已保存 ✓':'保存失败，请重试';
  btn.classList.add(ok?'ok':'bad');
  toast(ok?'已保存，屏幕马上更新':'保存失败，请检查网络后重试');
  await sleep(1600);
  btn.classList.remove('ok','bad');btn.disabled=false;btn.textContent='保存设置';
  saving=false;
}
$('#save').onclick=()=>save(false);

// ---- Wi-Fi ----
let chosen='';
$('#scan').onclick=async()=>{
  if(scanning)return;scanning=true;$('#scan').textContent='扫描中…';$('#wlist').innerHTML='';
  for(let i=0;i<12;i++){
    try{
      const j=await (await api('/api/wifi/scan')).json();
      if(!j.scanning){
        j.list.sort((a,b)=>b.r-a.r);
        $('#wlist').innerHTML=j.list.map(x=>`<div class="wifi" data-s="${esc(x.s)}"><span>${esc(x.s)}${x.e?' 🔒':''}</span><span class="hint">${x.r} dBm</span></div>`).join('')||'<div class="hint">没有扫描到</div>';
        document.querySelectorAll('#wlist .wifi').forEach(d=>d.onclick=()=>{
          document.querySelectorAll('#wlist .wifi').forEach(x=>x.classList.remove('on'));d.classList.add('on');$('#ssid').value=d.dataset.s});
        break;
      }
    }catch(e){break}
    await new Promise(r=>setTimeout(r,1000));
  }
  scanning=false;$('#scan').textContent='重新扫描';
};
$('#wgo').onclick=async()=>{
  const s=$('#ssid').value.trim();if(!s)return toast('请先选择或输入 Wi-Fi 名称');
  $('#wmsg').textContent='正在连接…';
  try{await api('/api/wifi',{method:'POST',body:JSON.stringify({s,p:$('#wpass').value})})}catch(e){return}
  for(let i=0;i<25;i++){
    await new Promise(r=>setTimeout(r,1000));
    try{await status();
      if(S.sta&&S.ssid===s){
        $('#wmsg').innerHTML=`连接成功！设备地址 <b>${S.ip}</b>。<br>请把手机切回家里的 Wi-Fi，再用 <b>http://love-clock.local</b> 或 <b>http://${S.ip}</b> 打开本页继续设置。`;return}
    }catch(e){}
  }
  $('#wmsg').textContent='连接失败，请检查密码后重试（设备会继续保持热点）';
};
$('#wforget').onclick=async()=>{
  if(!confirm('清除已保存的 Wi-Fi 并重启？重启后需要重新连接设备热点 LoveClock-Setup。'))return;
  await api('/api/wifi/forget',{method:'POST'});$('#wmsg').textContent='设备正在重启…';
};

setInterval(status,5000);
status().then(load).then(()=>{if(!S.sta&&S.ap)document.querySelector('#tabs button[data-t=wifi]').click()}).catch(()=>{});
</script></body></html>)rawliteral";
