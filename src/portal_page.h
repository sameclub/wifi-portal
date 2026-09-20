// Terminal-style WiFi provisioning portal. Icon adapted from 78/esp-wifi-connect (MIT).
// See LICENSE in this directory.
#pragma once
#include <Arduino.h>

// Placeholders replaced at serve time:
//   __PORTAL_APP_NAME__   - display name (e.g. "DotMic", "Pip-Boy")
//   __PORTAL_ACCENT__     - CSS hex color (e.g. "#ff2900", "#20b845")
//   __PORTAL_TOKEN__      - CSRF token

static const char PORTAL_HTML[] PROGMEM = R"PORTAL(<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>__PORTAL_APP_NAME__ Wi-Fi</title><style>
:root{--a:__PORTAL_ACCENT__;--ad:rgba(0,0,0,.15);--ag:rgba(0,0,0,.4);--bg:#0a0a0a;--s:#111;--s2:#181818;--b:#222;--t:#c8c8c8;--td:#555;--tb:#fff;--m:'SF Mono','Cascadia Code','Consolas','Menlo',monospace;--r:2px}
@media(prefers-color-scheme:light){:root{--bg:#f0f0f0;--s:#fff;--s2:#e8e8e8;--b:#ccc;--t:#222;--td:#888;--tb:#000}}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:var(--m);background:var(--bg);color:var(--t);min-height:100vh;font-size:12px;line-height:1.5;-webkit-font-smoothing:antialiased}
.sl{pointer-events:none;position:fixed;inset:0;background:repeating-linear-gradient(0deg,transparent,transparent 2px,rgba(0,0,0,.03) 2px,rgba(0,0,0,.03) 4px);z-index:9999}
.pg{max-width:420px;margin:0 auto;padding:12px 14px 40px}
.hd{display:flex;align-items:center;gap:10px;padding:10px 0;margin-bottom:12px;border-bottom:1px solid var(--b)}
.hi{width:28px;height:28px;flex-shrink:0;border:1px solid var(--a);display:flex;align-items:center;justify-content:center;color:var(--a)}
.hi svg{width:16px;height:16px}
.hn{font-size:14px;font-weight:700;color:var(--a);text-transform:uppercase;letter-spacing:2px}
.hb{margin-left:auto;font-size:10px;color:var(--td);text-transform:uppercase;letter-spacing:1px}
.sc{background:var(--s);border:1px solid var(--b);margin-bottom:12px}
.sh{display:flex;align-items:center;gap:8px;padding:8px 12px;background:var(--s2);border-bottom:1px solid var(--b);font-size:10px;font-weight:700;color:var(--td);text-transform:uppercase;letter-spacing:1.5px}
.sh::before{content:'>';color:var(--a);font-weight:700}
.sb{padding:14px}
.fg{margin-bottom:14px}
.fl{display:flex;align-items:baseline;justify-content:space-between;margin-bottom:4px}
.fl label{font-size:10px;font-weight:600;color:var(--td);text-transform:uppercase;letter-spacing:1px}
.lk{font-size:10px;color:var(--a);cursor:pointer;background:none;border:none;font-family:var(--m);text-decoration:none}
.lk:hover{text-decoration:underline}
input[type=text],input[type=password]{width:100%;padding:10px;font-family:var(--m);font-size:16px;color:var(--tb);background:var(--bg);border:1px solid var(--b);border-radius:var(--r);outline:none;transition:border-color .15s,box-shadow .15s}
input:focus{border-color:var(--a);box-shadow:0 0 0 1px var(--a),0 0 12px var(--ag)}
input::placeholder{color:var(--td);font-size:11px}
.pw{position:relative}.pw input{padding-right:56px}
.tg{position:absolute;right:1px;top:1px;bottom:1px;padding:0 10px;background:var(--s2);border:none;border-left:1px solid var(--b);color:var(--td);font-family:var(--m);font-size:10px;cursor:pointer;text-transform:uppercase;letter-spacing:.5px}
.tg:hover{color:var(--a)}
.bt{width:100%;padding:12px;font-family:var(--m);font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:2px;border:1px solid var(--a);background:var(--ad);color:var(--a);cursor:pointer;transition:all .15s;border-radius:var(--r)}
.bt:hover{background:var(--a);color:var(--bg);box-shadow:0 0 20px var(--ag)}
.bt:active{transform:scale(.98)}.bt:disabled{opacity:.4;cursor:wait}
.st{font-size:11px;padding:8px 10px;margin-top:12px;border-left:2px solid var(--a);background:var(--ad);color:var(--a)}
.st:empty{display:none}.st.er{border-color:#ef4444;color:#ef4444}.st.ok{color:#22c55e;border-color:#22c55e}
.ns{border-top:1px solid var(--b);padding-top:12px;margin-top:16px}
.nh{display:flex;align-items:baseline;justify-content:space-between;margin-bottom:8px}
.nh span{font-size:10px;color:var(--td);text-transform:uppercase;letter-spacing:1px}
.nw{display:flex;align-items:center;justify-content:space-between;width:100%;text-align:left;padding:8px 10px;margin-top:4px;font-family:var(--m);font-size:12px;background:var(--bg);border:1px solid var(--b);border-radius:var(--r);color:var(--t);cursor:pointer;transition:all .1s}
.nw:hover,.nw[aria-pressed=true]{border-color:var(--a);background:var(--ad);color:var(--tb)}
.nn{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.nm{font-size:10px;color:var(--td);flex-shrink:0;margin-left:8px}
.ht{font-size:10px;color:var(--td);margin-top:10px}
.ft{text-align:center;margin-top:16px;font-size:10px;color:var(--td);line-height:1.8}
.ft .ac{color:var(--a)}
</style>
<script>
function initAccent(){
var c=getComputedStyle(document.documentElement).getPropertyValue('--a').trim();
if(!c||c==='__PORTAL_ACCENT__')c='#ff2900';
var m=c.match(/^#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i);
if(m){var r=parseInt(m[1],16),g=parseInt(m[2],16),b=parseInt(m[3],16);
document.documentElement.style.setProperty('--ad','rgba('+r+','+g+','+b+',.15)');
document.documentElement.style.setProperty('--ag','rgba('+r+','+g+','+b+',.4)');}
}
</script>
</head><body onload="initAccent()"><div class="sl"></div><main class="pg"><header class="hd"><span class="hi">
<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>
</span><span class="hn">__PORTAL_APP_NAME__</span><span class="hb">2.4G // Wi-Fi</span></header>
<section class="sc"><div class="sh">Network Config</div><div class="sb"><form action="/save" method="post">
<div class="fg"><div class="fl"><label>SSID</label><button class="lk" type="button" id="manual">manual input</button></div><input type="text" id="ssid" name="ssid" maxlength="32" required readonly placeholder="select from list below" autocomplete="off"></div>
<div class="fg"><div class="fl"><label>Password</label></div><div class="pw"><input type="password" id="password" name="password" maxlength="63" placeholder="enter password" autocomplete="new-password"><button class="tg" type="button" id="show-pw">show</button></div></div>
<input type="hidden" name="token" value="__PORTAL_TOKEN__"><button class="bt" id="connect" type="submit">Connect &amp; Save</button><p id="status" class="st" role="status" aria-live="polite"></p></form>
<div class="ns"><div class="nh"><span>Nearby Networks</span><button class="lk" id="refresh" type="button">refresh</button></div><div id="networks" aria-live="polite"><p class="ht">// scanning...</p></div><p class="ht">// select network to auto-fill. open networks need no password.</p></div></div></section>
<p class="ft">keep phone connected to device hotspot<br><span class="ac">NTP auto-sync after connection</span></p></main>
<script>
var list=document.getElementById('networks'),ssid=document.getElementById('ssid'),password=document.getElementById('password'),status=document.getElementById('status'),refresh=document.getElementById('refresh'),connect=document.getElementById('connect'),scanning=false,submitting=false;
function notify(t){status.textContent=t;status.className='st'+(t.indexOf('fail')>=0||t.indexOf('error')>=0?' er':t.indexOf('saved')>=0||t.indexOf('success')>=0?' ok':'');}
function selectNetwork(n){ssid.value=n.ssid;ssid.readOnly=true;password.value='';password.required=n.secure;password.minLength=n.secure?8:0;password.placeholder=n.secure?'enter password':'open network';var rows=list.querySelectorAll('button');for(var i=0;i<rows.length;i++)rows[i].setAttribute('aria-pressed',String(rows[i].dataset.ssid===n.ssid));if(n.secure)password.focus();}
document.getElementById('manual').onclick=function(){ssid.readOnly=false;ssid.value='';ssid.placeholder='enter hidden SSID';password.required=false;password.minLength=0;var rows=list.querySelectorAll('button');for(var i=0;i<rows.length;i++)rows[i].setAttribute('aria-pressed','false');ssid.focus();};
document.getElementById('show-pw').onclick=function(){var v=password.type==='password';password.type=v?'text':'password';this.textContent=v?'hide':'show';};
function empty(t){var p=document.createElement('p');p.className='ht';p.textContent='// '+t;list.replaceChildren(p);}
function scan(start){if(start&&scanning)return;scanning=true;refresh.disabled=true;refresh.textContent='scanning...';
fetch('/networks'+(start?'?scan=1':'')).then(function(r){return r.json()}).then(function(d){
if(d.busy){setTimeout(function(){scan(false)},1200);return;}
list.replaceChildren();var seen={};
for(var i=0;i<d.networks.length;i++){var n=d.networks[i];if(seen[n.ssid])continue;seen[n.ssid]=1;
var row=document.createElement('button'),nm=document.createElement('span'),mt=document.createElement('span');
row.type='button';row.className='nw';row.dataset.ssid=n.ssid;row.setAttribute('aria-pressed',String(ssid.value===n.ssid));
nm.className='nn';nm.textContent=n.ssid;mt.className='nm';mt.textContent=n.rssi+' dBm // '+(n.secure?'ENC':'OPEN');
row.append(nm,mt);(function(net){row.onclick=function(){selectNetwork(net)}})(n);list.append(row);}
if(!d.networks.length)empty(d.error||'no networks found');
scanning=false;refresh.disabled=false;refresh.textContent='refresh';
}).catch(function(){empty('scan failed');scanning=false;refresh.disabled=false;refresh.textContent='refresh';});}
refresh.onclick=function(){scan(true)};
document.querySelector('form').onsubmit=function(e){if(scanning||submitting){e.preventDefault();notify(scanning?'scanning, wait...':'connecting...');return;}if(!ssid.value){e.preventDefault();notify('select a network first');return;}submitting=true;connect.disabled=true;connect.textContent='connecting...';};
if(location.pathname==='/save'){notify('connecting...');empty('refresh list to re-select');}else scan(true);
setInterval(function(){fetch('/status').then(function(r){return r.text()}).then(function(t){if(t)notify(t)}).catch(function(){})},2000);
</script></body></html>)PORTAL";
