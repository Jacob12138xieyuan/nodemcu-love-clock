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
.sty{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.sty div{border:2px solid var(--line);border-radius:10px;padding:4px;text-align:center;font-size:12px;color:var(--dim);cursor:pointer}
.sty div.on{border-color:var(--pink);color:var(--pink)}
.sty img{width:100%;display:block;border-radius:6px;image-rendering:pixelated}
@media (max-width:420px){.sty{grid-template-columns:repeat(2,1fr)}}
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
<header><button class="g" id="lang" style="float:right;padding:4px 12px;font-size:13px">EN</button><h1>💕 <span data-en="Love Clock">爱的小时钟</span></h1><div id="st"></div></header>
<nav id="tabs">
<button data-t="wifi">Wi-Fi</button><button data-t="basic" class="on" data-en="Basic">基本</button><button data-t="events" data-en="Events">纪念日</button>
<button data-t="quotes" data-en="Quotes">情话</button><button data-t="photos" data-en="Photos">照片</button><button data-t="display" data-en="Display">显示</button>
</nav>
<main>
<section id="wifi">
 <div class="hint" id="wst"></div>
 <label data-en="Saved Wi-Fi (up to 3, tried in most-recently-used order at boot)">已保存的 Wi-Fi（最多 3 个，开机时按最近使用的顺序尝试）</label>
 <div id="wsaved"></div>
 <button class="g" id="scan" style="margin-top:12px" data-en="Scan nearby Wi-Fi">扫描附近的 Wi-Fi</button>
 <div id="wlist"></div>
 <label data-en="Name (or type it in)">名称（也可以手动输入）</label><input id="ssid" autocapitalize="off" autocomplete="off">
 <label data-en="Password">密码</label><input id="wpass" type="password" autocomplete="off">
 <p class="row"><button class="b" id="wgo" data-en="Save &amp; connect">保存并连接</button><button class="g" id="wforget" data-en="Forget all Wi-Fi &amp; restart">清除全部 Wi-Fi 并重启</button></p>
 <div class="hint" id="wmsg"></div>
</section>
<section id="basic" class="on">
 <label data-en="Together since">在一起的日期</label><input type="date" id="start">
 <label data-en="City (for weather)">所在城市（用于天气）</label>
 <div class="row"><input id="city" placeholder="如：上海" data-enph="e.g. Shanghai"><button class="g fix" id="geo" data-en="Search">查找</button></div>
 <div id="geores"></div>
 <div class="row"><div><label data-en="Latitude">纬度</label><input id="lat" type="number" step="0.001"></div><div><label data-en="Longitude">经度</label><input id="lon" type="number" step="0.001"></div></div>
 <div class="hint" data-en="Search needs your phone to be online. You can also enter the coordinates directly.">查找需要手机能上网。也可以直接填经纬度。</div>
 <label data-en="Seconds per page">每页停留（秒）</label><input id="slide" type="number" min="3" max="60">
</section>
<section id="events">
 <div class="hint" data-en="Up to 10. If &quot;Celebrate all day&quot; is ticked, the screen shows the celebration page for the whole day. The &quot;Anniversary&quot; is added automatically from the date on the Basic tab.">最多 10 个。勾选"当天全屏庆祝"后，那天屏幕会一整天显示庆祝画面。「在一起纪念日」会根据基本页的日期自动加入，不用再填。</div>
 <div id="evlist"></div>
 <p><button class="g" id="evadd" data-en="+ Add event">＋ 添加纪念日</button></p>
</section>
<section id="quotes">
 <div class="hint" data-en="One per line, ideally under 70 characters each, up to about 50 lines. A random one is shown each time the quote page appears.">一行一句，建议每句不超过 40 个字，最多约 50 句。每次切到情话页会随机显示一句。</div>
 <textarea id="qtxt" placeholder="今天也很喜欢你&#10;记得多喝水" data-enph="Still crazy about you today&#10;Remember to drink water"></textarea>
</section>
<section id="photos">
 <div class="hint" data-en="Photos are cropped to a square and compressed, then uploaded right away. Up to 50 photos, about 20KB each.">选照片后会自动裁成正方形并压缩，选完立即上传保存。最多 50 张，每张约 20KB。</div>
 <input type="file" id="pfile" accept="image/*" multiple>
 <div class="grid" id="pgrid"></div>
</section>
<section id="display">
 <label data-en="Clock page style">时钟页样式</label>
 <div class="sty" id="sty"></div>
 <div class="hint" data-en="Tap one, then press Save. The default is the first (big digits).">点选一个，再按"保存设置"。默认是第一个（超大数字）。</div>
 <div class="row"><div><label><span data-en="Day brightness">白天亮度</span> <span id="bv"></span>%</label><input id="bright" type="range" min="5" max="100"></div>
 <div><label><span data-en="Night brightness">夜间亮度</span> <span id="nbv"></span>%</label><input id="nbright" type="range" min="0" max="100"></div></div>
 <div class="row"><div><label data-en="Night starts (hour)">夜间开始（点）</label><input id="ns" type="number" min="0" max="23"></div><div><label data-en="Night ends (hour)">夜间结束（点）</label><input id="ne" type="number" min="0" max="23"></div></div>
 <div class="hint" data-en="Same start and end means no dimming. Night brightness 0 turns the screen off.">开始和结束相同表示不调暗。夜间亮度设 0 就是熄屏。</div>
 <label data-en="Preview: pretend today is (to test the celebration page; clear it afterwards)">预览：假装今天是（测试庆祝页用，用完记得清空）</label><input type="date" id="test">
 <label data-en="Settings password (letters and digits only, up to 15)">设置页密码（仅限英文数字，最多 15 位）</label>
 <div class="row"><input id="npass" placeholder="" autocomplete="off"><button class="g fix" id="npclear" data-en="Remove password">取消密码</button></div>
 <div class="hint" id="pstate"></div>
</section>
</main>
<footer><button class="b" id="save" data-en="Save">保存设置</button></footer>
<div id="toast"></div>
<script>
const $=s=>document.querySelector(s);
function ls(k,v){try{if(v===undefined)return localStorage.getItem(k);localStorage.setItem(k,v)}catch(e){return null}}
let lang=ls('lang')||((navigator.language||'').toLowerCase().startsWith('zh')?'zh':'en');
const L=(z,e)=>lang==='en'?e:z;
function applyLang(){
  document.documentElement.lang=lang;document.title=L('爱的小时钟','Love Clock');
  document.querySelectorAll('[data-en]').forEach(el=>{if(el._z===undefined)el._z=el.innerHTML;el.innerHTML=lang==='en'?el.dataset.en:el._z});
  document.querySelectorAll('[data-enph]').forEach(el=>{if(el._p===undefined)el._p=el.placeholder;el.placeholder=lang==='en'?el.dataset.enph:el._p});
  $('#lang').textContent=lang==='en'?'中文':'EN';
}
let C={events:[],photos:[]},pass=sessionStorage.getItem('p')||'',newPass=null,scanning=false;
function toast(m){const t=$('#toast');t.textContent=m;t.classList.add('on');clearTimeout(toast.h);toast.h=setTimeout(()=>t.classList.remove('on'),2200)}
async function api(path,opt={}){
  opt.headers=Object.assign({'X-Pass':pass},opt.headers||{});
  const r=await fetch(path,opt);
  if(r.status===401){const p=prompt(L('请输入设置密码','Enter the settings password'));if(p===null)throw new Error('auth');pass=p;sessionStorage.setItem('p',p);return api(path,opt)}
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
    const w=S.sta?L(`已连接 ${S.ssid}（${S.ip}）`,`Connected to ${S.ssid} (${S.ip})`):(S.ap?L('未连接路由器，正在使用热点','Not connected, using hotspot'):L('未连接','Not connected'));
    const kb=Math.round(S.free/1024);
    $('#st').textContent=w+(S.time?` · ${S.time}`:'')+L(` · 剩余空间 ${kb}KB`,` · ${kb}KB free`);
    $('#wst').textContent=S.sta?L(`当前已连接：${S.ssid}，设备地址 ${S.ip}`,`Connected: ${S.ssid}, device address ${S.ip}`):L('当前未连接 Wi-Fi','Not connected to Wi-Fi');
  }catch(e){$('#st').textContent=L('无法连接设备','Cannot reach the device')}
}

// ---- 加载与渲染 ----
const STY=[['超大数字','Big digits'],['上下分行','Stacked'],['圆润卡片','Cards'],['表盘','Dial'],['进度信息','Progress'],['复古数码','Retro LCD'],['爱心','Heart'],['极简','Minimal']];
const STYIMG=['data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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','data:image/png;base64,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'];
function renderSty(){
  $('#sty').innerHTML=STY.map((s,i)=>`<div data-i="${i}" class="${(C.cs|0)===i?'on':''}"><img src="${STYIMG[i]}" alt="">${'ABCDEFGH'[i]} ${L(s[0],s[1])}</div>`).join('');
  $('#sty').querySelectorAll('div').forEach(d=>d.onclick=()=>{C.cs=+d.dataset.i;renderSty()});
}
async function load(){
  const r=await api('/api/config');C=await r.json();
  $('#start').value=C.start||'';$('#city').value=C.city||'';$('#lat').value=C.lat;$('#lon').value=C.lon;
  $('#slide').value=C.slide;$('#bright').value=C.bright;$('#nbright').value=C.nightBright;
  $('#ns').value=C.nightStart;$('#ne').value=C.nightEnd;$('#test').value=C.testDate||'';
  $('#bv').textContent=C.bright;$('#nbv').textContent=C.nightBright;
  passUi();
  const q=await api('/api/quotes');$('#qtxt').value=await q.text();
  renderEvents();renderPhotos();renderSty();
  if(!ls('lang')){ls('lang',lang);if((lang==='en')!==(C.lang===1))api('/api/config',{method:'POST',body:JSON.stringify({lang:lang==='en'?1:0})}).catch(()=>{})}
}
function passUi(){
  $('#pstate').textContent=C.hasPass?L('当前已设置密码','Password is set'):L('当前没有密码','No password');
  $('#npass').placeholder=C.hasPass?L('输入新密码以修改','Enter a new password to change it'):L('输入密码以开启','Enter a password to enable it');
}
$('#lang').onclick=async()=>{
  lang=lang==='en'?'zh':'en';ls('lang',lang);applyLang();
  status();passUi();renderEvents();renderPhotos();wsaved();renderSty();
  try{await api('/api/config',{method:'POST',body:JSON.stringify({lang:lang==='en'?1:0})})}catch(e){}
};
$('#bright').oninput=e=>$('#bv').textContent=e.target.value;
$('#nbright').oninput=e=>$('#nbv').textContent=e.target.value;

function renderEvents(){
  $('#evlist').innerHTML=C.events.map((e,i)=>`<div class="ev">
   <input data-i="${i}" data-k="n" maxlength="${lang==='en'?28:10}" placeholder="${L('名称，如：她的生日','Name, e.g. Her birthday')}" value="${esc(e.n)}">
   <div class="row" style="margin-top:8px"><input data-i="${i}" data-k="m" type="number" min="1" max="12" value="${e.m}"><span class="fix">${L('月','month')}</span>
   <input data-i="${i}" data-k="d" type="number" min="1" max="31" value="${e.d}"><span class="fix">${L('日','day')}</span></div>
   <div class="row" style="margin-top:8px"><label style="margin:0;color:var(--ink)"><input data-i="${i}" data-k="c" type="checkbox" ${e.c?'checked':''}> ${L('当天全屏庆祝','Celebrate all day')}</label>
   <button class="g fix" data-del="${i}">${L('删除','Delete')}</button></div></div>`).join('');
  document.querySelectorAll('#evlist input').forEach(el=>el.oninput=()=>{
    const e=C.events[+el.dataset.i],k=el.dataset.k;
    e[k]=k==='c'?el.checked:(k==='n'?el.value:+el.value)});
  document.querySelectorAll('#evlist [data-del]').forEach(b=>b.onclick=()=>{C.events.splice(+b.dataset.del,1);renderEvents()});
}
$('#evadd').onclick=()=>{if(C.events.length>=10)return toast(L('最多 10 个','Up to 10'));C.events.push({n:'',m:1,d:1,c:true});renderEvents()};

function renderPhotos(){
  $('#pgrid').innerHTML=C.photos.map((p,i)=>`<div class="ph"><img loading="lazy" src="/ph?f=${p.f}">
   <input data-i="${i}" maxlength="${lang==='en'?40:15}" placeholder="${L('照片下方的小字','Caption under the photo')}" value="${esc(p.c)}">
   <button class="g" data-del="${i}">${L('删除','Delete')}</button></div>`).join('');
  document.querySelectorAll('#pgrid input').forEach(el=>el.oninput=()=>C.photos[+el.dataset.i].c=el.value);
  document.querySelectorAll('#pgrid [data-del]').forEach(b=>b.onclick=async()=>{
    const p=C.photos[+b.dataset.del];if(!confirm(L('删除这张照片？','Delete this photo?')))return;
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
    if(C.photos.length>=50){toast(L('最多 50 张','Up to 50 photos'));break}
    toast(L(`上传中 ${++n}/${files.length}…`,`Uploading ${++n}/${files.length}...`));
    try{
      const blob=await crop(f),name='p'+(Date.now()%1e9)+'.jpg',fd=new FormData();
      fd.append('file',blob,name);
      const r=await api('/api/photo',{method:'POST',body:fd});
      if(!r.ok){toast(L('上传失败，空间可能已满','Upload failed, storage may be full'));break}
      C.photos.push({f:name,c:''});renderPhotos();
    }catch(err){toast(L('上传失败','Upload failed'));break}
  }
  await save(true);status();
};

// ---- 城市查找 ----
$('#geo').onclick=async()=>{
  const q=$('#city').value.trim();if(!q)return;
  try{
    const r=await fetch('https://geocoding-api.open-meteo.com/v1/search?count=5&language='+(lang==='en'?'en':'zh')+'&name='+encodeURIComponent(q));
    const j=await r.json();
    if(!j.results){$('#geores').innerHTML='<div class="hint">'+L('没找到，可以直接填经纬度','Not found. You can enter the coordinates directly.')+'</div>';return}
    $('#geores').innerHTML=j.results.map((x,i)=>`<div class="wifi" data-i="${i}">${esc(x.name)} <span class="hint">${esc([x.admin1,x.country].filter(Boolean).join(' '))}</span></div>`).join('');
    document.querySelectorAll('#geores .wifi').forEach(d=>d.onclick=()=>{
      const x=j.results[+d.dataset.i];$('#city').value=x.name;$('#lat').value=x.latitude.toFixed(3);$('#lon').value=x.longitude.toFixed(3);$('#geores').innerHTML=''});
  }catch(e){toast(L('查找失败，请确认手机能上网','Search failed. Make sure your phone is online.'))}
};

// ---- 密码 ----
$('#npass').oninput=e=>{const v=e.target.value;if(/[^A-Za-z0-9]/.test(v)||v.length>15){e.target.value=v.replace(/[^A-Za-z0-9]/g,'').slice(0,15)}newPass=e.target.value||null;
  $('#pstate').textContent=newPass?L('保存后生效','Takes effect after saving'):''};
$('#npclear').onclick=()=>{newPass='';$('#npass').value='';$('#pstate').textContent=L('保存后将取消密码','Password will be removed after saving')};

// ---- 保存 ----
function collect(){
  const o={start:$('#start').value,city:$('#city').value.trim(),lat:+$('#lat').value,lon:+$('#lon').value,
   slide:+$('#slide').value,bright:+$('#bright').value,nightBright:+$('#nbright').value,
   nightStart:+$('#ns').value,nightEnd:+$('#ne').value,testDate:$('#test').value,cs:C.cs|0,
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
  if(!quiet){btn.disabled=true;btn.innerHTML='<span class="spin"></span>'+L('保存中…','Saving...')}
  let ok=true;
  try{
    const r=await api('/api/config',{method:'POST',body:JSON.stringify(collect())});
    if(!r.ok)throw 0;
    if(newPass!==null){pass=newPass;sessionStorage.setItem('p',pass);C.hasPass=!!newPass;newPass=null;$('#npass').value='';
      passUi()}
    const q=await api('/api/quotes',{method:'POST',body:$('#qtxt').value.split('\n').map(s=>s.trim()).filter(Boolean).join('\n')});
    if(!q.ok)throw 0;
  }catch(e){ok=false}
  if(quiet){if(!ok)toast(L('保存失败','Save failed'));saving=false;return}
  const wait=700-(Date.now()-t0);if(wait>0)await sleep(wait);   // 保存很快，至少让"保存中"显示一会儿
  btn.innerHTML=ok?L('已保存 ✓','Saved ✓'):L('保存失败，请重试','Save failed, try again');
  btn.classList.add(ok?'ok':'bad');
  toast(ok?L('已保存，屏幕马上更新','Saved. The screen will update shortly.'):L('保存失败，请检查网络后重试','Save failed. Check the network and try again.'));
  await sleep(1600);
  btn.classList.remove('ok','bad');btn.disabled=false;btn.textContent=L('保存设置','Save');
  saving=false;
}
$('#save').onclick=()=>save(false);

// ---- Wi-Fi ----
let chosen='';
async function wsaved(){
  try{
    const j=await (await api('/api/wifi/list')).json();
    $('#wsaved').innerHTML=j.list.length?j.list.map((s,i)=>`<div class="wifi" style="align-items:center;cursor:default"><span>${esc(s)}</span><button class="g fix" data-i="${i}">${L('删除','Delete')}</button></div>`).join(''):'<div class="hint">'+L('还没有保存的 Wi-Fi','No saved Wi-Fi yet')+'</div>';
    document.querySelectorAll('#wsaved button').forEach(b=>b.onclick=async()=>{
      if(!confirm(L('删除这个 Wi-Fi？','Delete this Wi-Fi?')))return;
      try{await api('/api/wifi/del',{method:'POST',body:JSON.stringify({i:+b.dataset.i})})}catch(e){}
      wsaved();
    });
  }catch(e){}
}
$('#scan').onclick=async()=>{
  if(scanning)return;scanning=true;$('#scan').textContent=L('扫描中…','Scanning...');$('#wlist').innerHTML='';
  for(let i=0;i<12;i++){
    try{
      const j=await (await api('/api/wifi/scan')).json();
      if(!j.scanning){
        j.list.sort((a,b)=>b.r-a.r);
        $('#wlist').innerHTML=j.list.map(x=>`<div class="wifi" data-s="${esc(x.s)}"><span>${esc(x.s)}${x.e?' 🔒':''}</span><span class="hint">${x.r} dBm</span></div>`).join('')||'<div class="hint">'+L('没有扫描到','Nothing found')+'</div>';
        document.querySelectorAll('#wlist .wifi').forEach(d=>d.onclick=()=>{
          document.querySelectorAll('#wlist .wifi').forEach(x=>x.classList.remove('on'));d.classList.add('on');$('#ssid').value=d.dataset.s});
        break;
      }
    }catch(e){break}
    await new Promise(r=>setTimeout(r,1000));
  }
  scanning=false;$('#scan').textContent=L('重新扫描','Rescan');
};
$('#wgo').onclick=async()=>{
  const s=$('#ssid').value.trim();if(!s)return toast(L('请先选择或输入 Wi-Fi 名称','Pick or type a Wi-Fi name first'));
  $('#wmsg').textContent=L('正在连接…','Connecting...');
  try{await api('/api/wifi',{method:'POST',body:JSON.stringify({s,p:$('#wpass').value})})}catch(e){return}
  for(let i=0;i<25;i++){
    await new Promise(r=>setTimeout(r,1000));
    try{await status();
      if(S.sta&&S.ssid===s){
        wsaved();
        $('#wmsg').innerHTML=L(`连接成功！设备地址 <b>${S.ip}</b>。<br>请把手机切回家里的 Wi-Fi，再用 <b>http://love-clock.local</b> 或 <b>http://${S.ip}</b> 打开本页继续设置。`,`Connected! Device address <b>${S.ip}</b>.<br>Switch your phone back to your home Wi-Fi, then open <b>http://love-clock.local</b> or <b>http://${S.ip}</b> to continue.`);return}
    }catch(e){}
  }
  $('#wmsg').textContent=L('连接失败，请检查密码后重试（设备会继续保持热点）','Connection failed. Check the password and try again (the hotspot stays on).');
};
$('#wforget').onclick=async()=>{
  if(!confirm(L('清除全部已保存的 Wi-Fi 并重启？重启后需要重新连接设备热点 LoveClock-Setup。','Forget all saved Wi-Fi and restart? Afterwards you must connect to the LoveClock-Setup hotspot again.')))return;
  await api('/api/wifi/forget',{method:'POST'});$('#wmsg').textContent=L('设备正在重启…','Restarting...');
};

applyLang();
$('#st').textContent=L('连接中…','Connecting...');
setInterval(status,5000);
wsaved();
status().then(load).then(()=>{if(!S.sta&&S.ap)document.querySelector('#tabs button[data-t=wifi]').click()}).catch(()=>{});
</script></body></html>)rawliteral";
